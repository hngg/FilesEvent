
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


#include "TaskFileSend.h"
#include "TaskFileRecv.h"
#include "Session.h"


#include "config.h"
#include "event.h"

#include "h264.h"
#include "basedef.h"
#include "protocol.h"
#include "net_utils.h"

#include "EventActor.h"

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



//----------------------------------session---------------------------------

Session :: Session( Sockid_t sid )
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
}

Session :: Session( Sockid_t sid, short type)
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
		case VIDEO_RECV_MSG:
			// if(!mTaskBase)
			// 	mTaskBase = new TaskVideoRecv( this, mSid );
			break;

		case FILE_RECV_MSG:
				mTaskBase = new TaskFileRecv( this, mSid );
			break;
	}
}

Session :: Session( Sockid_t sid, short type, char*filepath)
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
		case VIDEO_RECV_MSG:
			// if(!mTaskBase)
			// 	mTaskBase = new TaskVideoRecv( this, mSid, filepath );
			break;

		case FILE_RECV_MSG:
				mTaskBase = new TaskFileRecv( this, mSid, filepath );
			break;
	}
}

Session :: Session( Sockid_t sid, short type, void*surface)
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
	switch(type) {
		case VIDEO_RECV_MSG:
			// if(!mTaskBase) {
			// 	mTaskBase = new TaskVideoRecv( this, mSid );
			// 	#ifdef __ANDROID__
			// 		TaskVideoRecv *temp = (TaskVideoRecv*)mTaskBase;
			// 		temp->setBase(new VideoDecoder(surface));
			// 	#endif
			// }
			break;

		case FILE_RECV_MSG:
				mTaskBase = new TaskFileRecv( this, mSid );
			break;
	}
}

Session :: Session( Sockid_t sid, short type, char*filepath, void*surface)
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
		case VIDEO_RECV_MSG:
			// if(!mTaskBase) {
			// 	mTaskBase = new TaskVideoRecv( this, mSid, filepath );
			// 	#ifdef __ANDROID__
			// 		TaskVideoRecv *temp = (TaskVideoRecv*)mTaskBase;
			// 		temp->setBase(new VideoDecoder(surface));
			// 	#endif
			// }
			break;

		case FILE_RECV_MSG:
				mTaskBase = new TaskFileRecv( this, mSid, filepath );
			break;
	}
}

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
		case VIDEO_RECV_MSG:
			// if(!mTaskBase) {
			// 	mTaskBase = new TaskVideoRecv( this, mSid, remoteFile );
			// 	#ifdef __ANDROID__
			// 		TaskVideoRecv *temp = (TaskVideoRecv*)mTaskBase;
			// 		//temp->setBase(new VideoDecoder(surface));
			// 	#endif
			// }
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

int Session :: setHeartBeat() {
	return mTaskBase!=NULL?mTaskBase->setHeartCount():0;
}

int Session :: recvEx(char*pData, int len) {
	int recvCount = 0, recvRet = 0;
	do{
		recvRet = recv(mSid.sid, pData+recvCount, len-recvCount, 0);
		if(recvRet<=0)
			return recvRet;

		recvCount += recvRet;
	}while(recvCount<len);
	return recvCount;
}

int Session :: readBuffer() {
	int ret = 0;

	if(mTaskBase!=NULL) {
		ret = mTaskBase->readBuffer();
	}
	else
	{
		if(mbRecvHead) {
			ret = recv(mSid.sid, mReadBuff+mHasRecvLen, mHeadLenConst-mHasRecvLen, 0);
			if(ret>0) {
				mHasRecvLen  += ret;
				if(mHasRecvLen==mHeadLenConst) {

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

int Session :: writeBuffer() {
	int ret = 0;
	if(mTaskBase!=NULL) {
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

void addEvent( Session * session, short events, int fd , void (*callback)(int, short, void *))
{
	EventGlobal * eventArg = (EventGlobal*)session->getArg();

	// if( ( events & EV_WRITE ) && (0 == session->getWriting()) ) 
	// {
	// 	session->setWriting( 1 );

	// 	struct event*pEvent=session->getWriteEvent();

	// 	if( fd < 0 ) 
	// 		fd = EVENT_FD( pEvent );

	// 	event_set( pEvent, fd, events, callback, session );
	// 	event_base_set( eventArg->getEventBase(), pEvent );

	// 	struct timeval timeout;
	// 	memset( &timeout, 0, sizeof( timeout ) );
	// 	timeout.tv_sec = eventArg->getTimeout();
	// 	event_add( pEvent, &timeout );
	// }

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

int Session :: addReadEvent(void (*callback)(int, short, void *))
{
	if(0 == getReading())
	{
		EventGlobal * eventArg = (EventGlobal*)getArg();
		//struct event* pEvent = getReadEvent();
		//event_set(mReadEvent, mSid.sid, EV_READ|EV_PERSIST, callback, this );
		//event_base_set( eventArg->getEventBase(), mReadEvent );

		addEvent(this, EV_READ, mSid.sid, onRead);

		//EventActor::addEvent( this, EV_READ, mSid.sid );
		// struct timeval timeout;
		// memset( &timeout, 0, sizeof( timeout ) );
		// timeout.tv_sec = eventArg->getTimeout();
		// event_add( pEvent, &timeout );

		setReading( 1 );

		log_warn("addReadEvent.");
	}
	//EventActor::addEvent( this, EV_READ, mSid.sid );
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
		if((0==ret)&&session)
		{
			log_warn("read zero:%d\n", ret);

			EventGlobal* eventArg = (EventGlobal*)session->getArg();
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

void Session :: setArg( void * arg )
{
	mArg = arg;
}

void * Session :: getArg()
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



