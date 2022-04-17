
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


#include "config.h"
#include "event.h"
#include "h264.h"
#include "basedef.h"
#include "protocol.h"
#include "net_utils.h"

#include "TaskFileSend.h"
#include "TaskFileRecv.h"
#include "Session.h"
#include "EventGlobal.h"


#define TIMEOUT_US 1000000


//----------------------------------session---------------------------------

/**
 * files receive session
 */
Session :: Session( Sockid_t sid, short type, char*remoteFile, char*saveFile)
			:mHeadLenConst(sizeof(NET_HEAD))
			,mSid(sid)
			,mTaskBase(NULL)
			,mArg(NULL)
			,mTotalDataLen(0)
			,mHasRecvLen(0)
			,mbRecvHead(true)
{
	mReadEvent  = (struct event*)malloc( sizeof( struct event ) );
	mWriteEvent = (struct event*)malloc( sizeof( struct event ) );
	mTimeEvent	= (struct event*)malloc( sizeof( struct event ) );

	mStatus  	= eNormal;
	mRunning 	= 0;
	mWriting 	= 0;
	mReading 	= 0;
	switch(type) 
	{
		case FILE_SEND_MSG:
			break;

		case FILE_RECV_MSG:
				mTaskBase = new TaskFileRecv( this, mSid, remoteFile, saveFile );
			break;
	}
}

Session :: ~Session()
{
	if(mReadEvent)
	{
		free( mReadEvent );
		mReadEvent = NULL;
	}

	if(mWriteEvent)
	{
		free( mWriteEvent );
		mWriteEvent = NULL;
	}

	if(mTimeEvent)
	{
		free( mTimeEvent );
		mTimeEvent = NULL;
	}

	if(mTaskBase!=NULL) 
	{
		delete mTaskBase;
		mTaskBase = NULL;
	}
}

int Session :: setHeartBeat() 
{
	return mTaskBase!=NULL?mTaskBase->setHeartCount():0;
}

int Session :: recvEx(char*pData, int len) 
{
	int recvCount = 0, recvRet = 0;
	do{
		recvRet = recv(mSid.sid, pData+recvCount, len-recvCount, 0);
		if(recvRet<=0)
			return recvRet;

		recvCount += recvRet;
	}while(recvCount<len);

	return recvCount;
}

int Session :: readBuffer() 
{
	int ret = 0;

	if(mTaskBase!=NULL) 
	{
		ret = mTaskBase->readBuffer();
	}
	else
	{
		if(mbRecvHead) 
		{
			ret = recv(mSid.sid, mReadBuff+mHasRecvLen, mHeadLenConst-mHasRecvLen, 0);
			if(ret>0) 
			{
				mHasRecvLen  += ret;
				if(mHasRecvLen==mHeadLenConst) 
				{

					LPNET_HEAD head = (LPNET_HEAD)mReadBuff;
					mTotalDataLen   = head->dwLength;
					mHasRecvLen = 0;
					mbRecvHead  = false;

					log_error("Session flag:%08x frameLen:%d ret:%d\n", head->dwFlag, mTotalDataLen, ret);
					//GLOGE("Session flag:%08x ret:%d data:%s", cmdbuf->dwFlag, ret, cmdbuf->lpData);
					ret = recvPackData();
				}
			}
		}
		else
			ret = recvPackData();

	}//mTaskBase==NULL

	return ret;
}

int Session :: writeBuffer() 
{
	int ret = 0;
	if(mTaskBase != NULL)
	{
		ret = mTaskBase->writeBuffer();
	}
	return ret;
}

int Session ::recvPackData() 
{
	int ret = recv(mSid.sid, mReadBuff+mHeadLenConst+mHasRecvLen, mTotalDataLen-mHasRecvLen, 0);
	if(ret>0) 
	{
		mHasRecvLen += ret;
		if(mHasRecvLen==mTotalDataLen) 
		{
		    int lValueLen, dataType=-1;
		    char acValue[256] = {0};	//new char[256];
		    memset(acValue,0, 256);
			LPNET_CMD pCmdbuf = (LPNET_CMD)mReadBuff;
		    PROTO_GetValueByName(mReadBuff, (char*)"play path", acValue, &lValueLen);
		    if(lValueLen>0)
		    	dataType 	= 0;
		    else 
			{
		    	PROTO_GetValueByName(mReadBuff, (char*)"get path", acValue, &lValueLen);
		    	if(lValueLen>0)
		    		dataType = 1;
		    	else
		    	{
			    	PROTO_GetValueByName(mReadBuff, (char*)"play real", acValue, &lValueLen);
			    	if(lValueLen>0)
			    		dataType = 2;
		    	}
		    }
		    log_info("filename:%s cmd:%d dataType:%d\n", acValue, pCmdbuf->dwCmd, dataType);

		    if(access(acValue, F_OK)!=0) 
			{
		    	log_error("filename %s is no exist.\n",acValue);
		    	return 0;
		    }

			switch(dataType) 
			{
				case 0:
					// mTaskBase = new TaskVideoSend( this, mSid, acValue );
					break;

				case 1:
					mTaskBase = new TaskFileSend( this, mSid, acValue );
					break;
				case 2:
					#ifdef __ANDROID__
					// mTaskBase = new TaskVideoRealSend( this, mSid, acValue );
					#endif
					break;
			}
			mTotalDataLen = 0;
			mHasRecvLen   = 0;
			mbRecvHead    = true;

			//SAFE_DELETE(acValue);
			//GLOGE("Session flag:%08x frameLen:%d ret:%d", pCmdbuf->dwFlag, pCmdbuf->dwLength, ret);
			//GLOGE("Session flag:%08x ret:%d data:%s", pCmdbuf->dwFlag, ret, pCmdbuf->lpData);
		}
	}
	return ret;
}

void Session :: addEvent( Session * session, short events, int fd)
{
	if(events & EV_WRITE)
	{
		addEvent( session, events, fd , onWrite);
	}else if(events & EV_READ)
	{
		addEvent( session, events, fd , onRead);
	}
}

void Session :: addEvent( Session * session, short events, int fd , void (*callback)(int, short, void *))
{
	EventGlobal * eventArg = (EventGlobal*)session->getGlobal();

	if( ( events & EV_WRITE ) && (0 == session->getWriting()) ) 
	{
		session->setWriting( 1 );

		struct event*pEvent=session->getWriteEvent();

		if( fd < 0 ) 
			fd = EVENT_FD( pEvent );

		event_set( pEvent, fd, events, callback, session );
		event_base_set( eventArg->getEventBase(), pEvent );

		struct timeval timeout;
		memset( &timeout, 0, sizeof( timeout ) );
		timeout.tv_sec = eventArg->getTimeout();
		event_add( pEvent, &timeout );
	}

	if( (events & EV_READ) && (0 == session->getReading()) )
	{
		session->setReading( 1 );

		if( fd < 0 ) 
			fd = EVENT_FD( session->getWriteEvent() );

		struct event*pEvent=session->getReadEvent();
		event_set( pEvent, fd, events, callback, session );
		event_base_set( eventArg->getEventBase(), pEvent );

		struct timeval timeout;
		memset( &timeout, 0, sizeof( timeout ) );
		timeout.tv_sec = eventArg->getTimeout();
		event_add( pEvent, &timeout );
	}
}

int Session :: addReadEvent()
{
	if(0 == getReading())
	{
		//EventGlobal * eventArg = (EventGlobal*)getArg();

		addEvent(this, EV_READ, mSid.sid, onRead);

		setReading( 1 );

		log_warn("addReadEvent.");
	}
	return 0;
}

int Session :: addWriteEvent()
{
	if(0 == getWriting())
	{
		//EventGlobal * eventArg = (EventGlobal*)getArg();

		addEvent(this, EV_WRITE, mSid.sid, onWrite);

		setWriting( 1 );

		log_warn("addWriteEvent.");
	}
	return 0;
}

int Session :: addTimerEvent()
{
	evtimer_set(getTimeEvent(), onTimer, this);	//useful
	//addEvent(  session, EV_TIMEOUT, clientFD );

	struct timeval timeout;
	memset( &timeout, 0, sizeof( timeout ) );
	timeout.tv_sec 	= 0;
	timeout.tv_usec = TIMEOUT_US;
	evtimer_add( getTimeEvent(), &timeout );
	return 0;
}

void Session :: onRead( int fd, short events, void * arg )
{
	Session* session = (Session*)arg;
	session->setReading( 0 );
	Sockid_t sid = session->getSid();
	int tempFd = fd;

	if( EV_READ & events ) 
	{
		int ret = session->readBuffer();
		if((0==ret) && session)
		{
			log_warn("read zero:%d\n", ret);

			EventGlobal* eventArg = (EventGlobal*)session->getGlobal();
			if(eventArg)
			{
				Session* sessRemoved = eventArg->getSessionManager()->remove( fd );
				event_del( session->getReadEvent() );
				event_del( session->getWriteEvent() );
				event_del( session->getTimeEvent() );

				if(sessRemoved==session) 
				{
					delete session;
					session = NULL;
					log_warn("session deleted socketid is:%d", tempFd);
				}
			}

			if(fd>0)
			{
				close( fd );
				fd = -1;
			}

			return;
		}
	}//if

	addEvent( session, EV_READ, fd ,onRead);
}

void Session :: onWrite( int fd, short events, void * arg )
{
	int ret = 0;
	Session * session = (Session*)arg;
	session->setWriting( 0 );

	Sockid_t sid = session->getSid();
	//GLOGW("onWrite fd:%d sid:%d",fd,session->getSid().mKey);

	if( EV_WRITE & events ) 
	{
		ret = session->writeBuffer();
	}

	//>0 need listen,<=0 not do that
	if(ret==OWN_SOCK_EXIT) {
	}
	else if(ret != 0)
		addEvent( session, EV_WRITE, fd , onWrite);
}

void Session :: onTimer( int fd, short events, void * arg )
{
	Session * session = (Session*)arg;
	int count = session->setHeartBeat();

	struct timeval timeout;
	memset( &timeout, 0, sizeof( timeout ) );
	timeout.tv_sec 	= 0;
	timeout.tv_usec = TIMEOUT_US;
	evtimer_add(session->getTimeEvent(), &timeout);
}

struct event * Session :: getReadEvent()
{
	return mReadEvent;
}

struct event * Session :: getWriteEvent()
{
	return mWriteEvent;
}

struct event * Session :: getTimeEvent()
{
	return mTimeEvent;
}

void Session :: setGlobal( void * arg )
{
	mArg = arg;
}

void * Session :: getGlobal()
{
	return mArg;
}

Sockid_t Session :: getSid()
{
	return mSid;
}

void Session :: setStatus( int status )
{
	mStatus = status;
}

int Session :: getStatus()
{
	return mStatus;
}

int Session :: getRunning()
{
	return mRunning;
}

void Session :: setRunning( int running )
{
	mRunning = running;
}

int Session :: getWriting()
{
	return mWriting;
}

void Session :: setWriting( int writing )
{
	mWriting = writing;
}

int Session :: getReading()
{
	return mReading;
}

void Session :: setReading( int reading )
{
	mReading = reading;
}


//---------------------------sessionManager----------------------------------------

typedef struct tagSessionEntry 
{
	uint16_t mSeq;
	Session  *mSession;
} SessionEntry;

SessionManager :: SessionManager()
{
	mCount = 0;
	memset( mArray, 0, sizeof( mArray ) );
}

SessionManager :: ~SessionManager()
{
	int arrLen = (int)( sizeof( mArray ) / sizeof( mArray[0] ));
	for( int i = 0; i < arrLen; i++ ) 
	{
		SessionEntry_t * node = mArray[i];
		if( NULL != node ) 
		{
			SessionEntry_t * iter = node;
			for( int i = 0; i < 1024; i++, iter++ ) 
			{
				if( NULL != iter->mSession ) 
				{
					delete iter->mSession;
					iter->mSession = NULL;
				}
			}
			free( node );
			node = NULL;
		}
	}

	memset( mArray, 0, sizeof( mArray ) );
}

void SessionManager :: setRealView(int sockId, void*surface) {

	uint16_t seq  = 0;
	Session* sess = get( sockId, &seq );
}

int SessionManager :: getCount()
{
	return mCount;
}

void SessionManager :: put( uint16_t sid, Session * session, uint16_t * seq )
{
	int row = sid / 1024, col = sid % 1024;

	if( NULL == mArray[ row ] ) 
	{
		mArray[ row ] = ( SessionEntry_t * )calloc(1024, sizeof( SessionEntry_t ));
	}

	SessionEntry_t * node = mArray[ row ];
	node[ col ].mSession = session;
	*seq = node[ col ].mSeq;

	mCount++;
}

Session * SessionManager :: get( uint16_t sid, uint16_t * seq )
{
	int row = sid / 1024, col = sid % 1024;

	Session * sess = NULL;

	SessionEntry_t * node = mArray[row];
	if( NULL != node ) 
	{
		sess = node[col].mSession;
		*seq = node[col].mSeq;
	} 
	else 
	{
		*seq = 0;
	}

	return sess;
}

Session * SessionManager :: remove( uint16_t sid, uint16_t * seq )
{
	int row = sid / 1024, col = sid % 1024;

	Session * sessRemoved = NULL;

	SessionEntry_t * node = mArray[ row ];
	if( NULL != node ) 
	{
		sessRemoved = node[ col ].mSession;
		if( NULL != seq ) 
			*seq  = node[ col ].mSeq;

		node[ col ].mSession = NULL;
		node[ col ].mSeq++;

		mCount--;
	}

	return sessRemoved;
}
