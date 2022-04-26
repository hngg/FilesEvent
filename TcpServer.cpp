
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/tcp.h>

#include "TcpServer.h"
#include "Session.h"
#include "IOUtils.h"

#include "config.h"
#include "basedef.h"

//#include "event_msgqueue.h"
#define MAX_CONNECTIONS 256

TcpServer :: TcpServer( const char * bindIP, int port )
			:mListenFD(0)
			,mMaxConnections(MAX_CONNECTIONS)
{
	snprintf( mBindIP, sizeof( mBindIP ), "%s", bindIP );
	mPort = port;

	mEvAccept  = (struct event*)malloc( sizeof( struct event ) );
}

TcpServer :: ~TcpServer()
{
	if(mEvAccept)
	{
		free( mEvAccept );
		mEvAccept = NULL;
	}
}

void TcpServer :: setMaxConnections( int maxConnections )
{
	mMaxConnections = maxConnections > 0 ? maxConnections : mMaxConnections;
}

int TcpServer :: registerEvent(EventGlobal* evglobal) 
{
	int ret = 0;

	if(mEvAccept && evglobal)
	{
		ret = IOUtils::tcpListen( mBindIP, mPort, &mListenFD, 0 );
		log_warn("create listenid:%d ret:%d", mListenFD, ret);

		evglobal->setMaxConnections(mMaxConnections);

		event_set( mEvAccept, mListenFD, EV_READ|EV_PERSIST, TcpServer::onAccept, evglobal );
		event_base_set( evglobal->getEventBase(), mEvAccept );	//set event_base to event
		event_add( mEvAccept, NULL );

		log_warn("event_add accept event time:%d", evglobal->getTimeout());
	}

	return ret;
}

void TcpServer :: shutdown() 
{
	if(mEvAccept)
	{
		event_del( mEvAccept );
		free( mEvAccept );
		mEvAccept = NULL;
	}
	if(mListenFD>0) 
	{
		close(mListenFD);
		log_warn("close listenid:%d", mListenFD);
		mListenFD = -1;
	}
	log_warn("shutdown done.");
}

void TcpServer :: onAccept( int listenFd, short events, void * arg )
{
	int clientFD;

	struct sockaddr_in clientAddr;
	socklen_t clientLen = sizeof( clientAddr );
	
	EventGlobal* eventArg = (EventGlobal*)arg;
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
	if( IOUtils::setBlock( clientFD, 0 ) < 0 ) 
	{
		log_error("failed to set client socket non-blocking\n" );
	}

	Sockid_t sid;
	sid.sid = clientFD;
	eventArg->getSessionManager()->get( sid.sid, &sid.seq );

	char clientIP[ 32 ] = { 0 };
	IOUtils::inetNtoa( &( clientAddr.sin_addr ), clientIP, sizeof( clientIP ) );
	log_warn( "clientIP: %s clientFD:%d",clientIP, clientFD);

	Session* session = new Session(sid, FILE_SEND_MSG);
	if( NULL != session ) 
	{
		eventArg->getSessionManager()->put( sid.sid, session, &sid.seq );
		session->setGlobal( eventArg );

		session->addReadEvent();
		session->addWriteEvent();
		//TO-DO test timer
		
		if( eventArg->getSessionManager()->getCount() > eventArg->getMaxConnections()) 
		{
			session->setStatus( Session::eExit );
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

