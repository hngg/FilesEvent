
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "config.h"   // from libevent, for event.h
#include "event.h"

#include "basedef.h"

#include "EventActor.h"
#include "Session.h"
#include "BufferCache.h"
#include "IOUtils.h"


#define TIMEOUT_US 1000000

EventGlobal :: EventGlobal()
		:mTimeout(0)
		,mEventBase(NULL)
		,mSessionManager(NULL) 
{

}

EventGlobal :: EventGlobal( int timeout )
			:mTimeout(timeout)
			,mEventBase(NULL)
			,mSessionManager(NULL) 
{

}

EventGlobal :: ~EventGlobal()
{
	//do clean
	if(mSessionManager) {
		delete mSessionManager;
		mSessionManager = NULL;
	}
	
	if(mEventBase) {
		event_destroy();
		mEventBase = NULL;
	}
}

int EventGlobal ::Create() 
{
	if(mEventBase==NULL)
		mEventBase = (struct event_base*)event_init();

	if(mSessionManager==NULL)
		mSessionManager = new SessionManager();

	return 0;
}

int EventGlobal ::Destroy()
{
	//do clean
	if(mSessionManager!=NULL) 
	{
		log_warn("Manager session count:%d", mSessionManager->getCount());
		delete mSessionManager;
		mSessionManager = NULL;
	}

	if(mEventBase) 
	{
		event_destroy();
		mEventBase = NULL;
	}

	return 0;
}

struct event_base * EventGlobal :: getEventBase() const 
{
	return mEventBase;
}

SessionManager * EventGlobal :: getSessionManager() const
{
	return mSessionManager;
}

void EventGlobal :: setTimeout( int timeout )
{
	mTimeout = timeout;
}

int EventGlobal :: getTimeout() const
{
	return mTimeout;
}

void EventGlobal ::setMaxConnections( int connection )
{
	mMaxConnections = connection;
}

int  EventGlobal ::getMaxConnections() const 
{
	return mMaxConnections;
}

//-------------------------------Event callback------------------------------------

void EventActor :: onAccept( int listenFd, short events, void * arg )
{
	int clientFD;

	struct sockaddr_in clientAddr;
	socklen_t clientLen = sizeof( clientAddr );
	
	EventGlobal * eventArg = (EventGlobal*)arg;
	if(!eventArg)
		return ;

	clientFD = accept( listenFd, (struct sockaddr *)&clientAddr, &clientLen );
	if( -1 == clientFD ) 
	{
		log_error("accept failed errno:%d.", errno);
		return;
	}

	log_warn( "accept client fd:%d\n", clientFD);

	//set nonblock
	if( IOUtils::setBlock( clientFD, 0 ) < 0 ) {
		log_error("failed to set client socket non-blocking\n" );
	}

	Sockid_t sid;
	sid.sid = clientFD;
	eventArg->getSessionManager()->get( sid.sid, &sid.seq );

	char clientIP[ 32 ] = { 0 };
	IOUtils::inetNtoa( &( clientAddr.sin_addr ), clientIP, sizeof( clientIP ) );
	log_warn( "clientIP: %s clientFD:%d",clientIP, clientFD);

	Session * session = new Session( sid );
	if( NULL != session ) 
	{
		eventArg->getSessionManager()->put( sid.sid, session, &sid.seq );
		session->setArg( eventArg );

		event_set( session->getReadEvent(),  clientFD, EV_READ,  onRead, session );
		event_set( session->getWriteEvent(), clientFD, EV_WRITE, onWrite, session );
		addEvent(  session, EV_READ, clientFD );

		//event_set( session->getTimeEvent(), clientFD, EV_TIMEOUT, onTimer, session );
		evtimer_set(session->getTimeEvent(), onTimer, session);	//useful
		addEvent(  session, EV_TIMEOUT, clientFD );

		struct timeval timeout;
		memset( &timeout, 0, sizeof( timeout ) );
		timeout.tv_sec 	= 0;
		timeout.tv_usec = TIMEOUT_US;
		evtimer_add( session->getTimeEvent(), &timeout );
		addEvent( session, EV_WRITE, clientFD );

		if( eventArg->getSessionManager()->getCount() > eventArg->getMaxConnections()
			/*|| eventArg->getInputResultQueue()->getLength() >= acceptArg->mReqQueueSize*/ ) {

			//syslog( LOG_WARNING, "System busy, session.count %d [%d], queue.length %d [%d]",
			//	eventArg->getSessionManager()->getCount(), acceptArg->mMaxConnections,
			//	eventArg->getInputResultQueue()->getLength(), acceptArg->mReqQueueSize );
/*
			SP_Message * msg = new SP_Message();
			msg->getMsg()->append( acceptArg->mRefusedMsg );
			msg->getMsg()->append( "\r\n" );
			session->getOutList()->append( msg );
*/
			session->setStatus( Session::eExit );

			addEvent( session, EV_WRITE, clientFD );
		} 
		else 
		{
			log_info("doStart.\n");
		}
	} 
	else 
	{
		close( clientFD );
		log_error("Out of memory, cannot allocate session object!\n" );
	}

}

void EventActor :: onRead( int fd, short events, void * arg )
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

	addEvent( session, EV_READ, fd );
}

void EventActor :: onWrite( int fd, short events, void * arg )
{
	int ret = 0;
	Session * session = (Session*)arg;

	EventGlobal * eventArg = (EventGlobal*)session->getArg();

	session->setWriting( 0 );

	Sockid_t sid = session->getSid();
	//GLOGW("onWrite fd:%d sid:%d",fd,session->getSid().mKey);

	if( EV_WRITE & events ) 
	{
		ret = session->writeBuffer();
	}

	//>0 need listen,<=0 not do that
	if(ret==OWN_SOCK_EXIT) {
//		EventArg * eventArg = (EventArg*)session->getArg();
//		eventArg->getSessionManager()->remove( fd );
//		event_del( session->getReadEvent() );
//		event_del( session->getWriteEvent() );
//		event_del( session->getTimeEvent() );
//
//		delete session;
//		session = NULL;
//		close( fd );
	}
	else if(ret != 0)
		addEvent( session, EV_WRITE, fd );


/*
	if( EV_WRITE & events ) {
		int ret = 0;

		if( session->getOutList()->getCount() > 0 ) {
			int len = session->getIOChannel()->transmit( session );

			if( len > 0 ) {
				if( session->getOutList()->getCount() > 0 ) {
					// left for next write event
					addEvent( session, EV_WRITE, -1 );
				}
			} else {
				if( EINTR != errno && EAGAIN != errno ) {
					ret = -1;
					if( 0 == session->getRunning() ) {
						syslog( LOG_NOTICE, "session(%d.%d) write error", sid.mKey, sid.mSeq );
						EventHelper::doError( session );
					} else {
						addEvent( session, EV_WRITE, -1 );
						syslog( LOG_NOTICE, "session(%d.%d) busy, process session error later, errno [%d]",
								sid.mKey, sid.mSeq, errno );
					}
				}
			}
		}

		if( 0 == ret && session->getOutList()->getCount() <= 0 ) {
			if( Session::eExit == session->getStatus() ) {
				ret = -1;
				if( 0 == session->getRunning() ) {
					syslog( LOG_NOTICE, "session(%d.%d) close, exit", sid.mKey, sid.mSeq );

					eventArg->getSessionManager()->remove( fd );
					event_del( session->getReadEvent() );
					handler->close();
					close( fd );
					delete session;
				} else {
					addEvent( session, EV_WRITE, -1 );
					syslog( LOG_NOTICE, "session(%d.%d) busy, terminate session later",
							sid.mKey, sid.mSeq );
				}
			}
		}

		if( 0 == ret ) {
			if( 0 == session->getRunning() ) {
				SP_MsgDecoder * decoder = session->getRequest()->getMsgDecoder();
				if( SP_MsgDecoder::eOK == decoder->decode( session->getInBuffer() ) ) {
					EventHelper::doWork( session );
				}
			} else {
				// If this session is running, then onResponse will add write event for this session.
				// So no need to add write event here.
			}
		}
	} else {
		if( 0 == session->getRunning() ) {
			EventHelper::doTimeout( session );
		} else {
			addEvent( session, EV_WRITE, -1 );
			syslog( LOG_NOTICE, "session(%d.%d) busy, process session timeout later",
					sid.mKey, sid.mSeq );
		}
	}
*/
}

void EventActor :: onTimer( int fd, short events, void * arg )
{
	Session * session = (Session*)arg;
	int count = session->setHeartBeat();

	struct timeval timeout;
	memset( &timeout, 0, sizeof( timeout ) );
	timeout.tv_sec 	= 0;
	timeout.tv_usec = TIMEOUT_US;
	evtimer_add(session->getTimeEvent(), &timeout);
}


void EventActor :: addEvent( Session * session, short events, int fd )
{
	EventGlobal * eventArg = (EventGlobal*)session->getArg();

	if( ( events & EV_WRITE ) && (0 == session->getWriting()) ) 
	{
		session->setWriting( 1 );

		struct event*pEvent=session->getWriteEvent();

		if( fd < 0 ) 
			fd = EVENT_FD( pEvent );

		event_set( pEvent, fd, events, onWrite, session );
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
		event_set( pEvent, fd, events, onRead, session );
		event_base_set( eventArg->getEventBase(), pEvent );

		struct timeval timeout;
		memset( &timeout, 0, sizeof( timeout ) );
		timeout.tv_sec = eventArg->getTimeout();
		event_add( pEvent, &timeout );
	}
}

