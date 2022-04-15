

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
}

TcpServer :: ~TcpServer()
{

}

void TcpServer :: setMaxConnections( int maxConnections )
{
	mMaxConnections = maxConnections > 0 ? maxConnections : mMaxConnections;
}

int TcpServer :: registerEvent(EventGlobal& evarg) 
{
	int ret = 0;

	ret = IOUtils::tcpListen( mBindIP, mPort, &mListenFD, 0 );
	log_warn("create listenid:%d ret:%d", mListenFD, ret);

	evarg.setMaxConnections(mMaxConnections);

	event_set( &mEvAccept, mListenFD, EV_READ|EV_PERSIST, EventActor::onAccept, &evarg );
	event_base_set( evarg.getEventBase(), &mEvAccept );
	event_add( &mEvAccept, NULL );

	return ret;
}

void TcpServer :: shutdown() 
{
	event_del( &mEvAccept);
	if(mListenFD>0) 
	{
		close(mListenFD);
		log_warn("close listenid:%d", mListenFD);
		mListenFD = -1;
	}
}


