

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/tcp.h>

#include "UdpServer.h"
#include "Session.h"
#include "IOUtils.h"

#include "config.h"
#include "basedef.h"

//#include "event_msgqueue.h"

UdpServer :: UdpServer( const char * bindIP, int port )
{
	snprintf( mBindIP, sizeof( mBindIP ), "%s", bindIP );
	mPort = port;
	mIsShutdown = 0;
	mIsRunning = 0;
	mListenFD = 0;


	mMaxThreads 	= 64;
	mReqQueueSize 	= 128;
	mMaxConnections = 256;
}

UdpServer :: ~UdpServer()
{

}

void UdpServer :: setMaxThreads( int maxThreads )
{
	mMaxThreads = maxThreads > 0 ? maxThreads : mMaxThreads;
}

void UdpServer :: setMaxConnections( int maxConnections )
{
	mMaxConnections = maxConnections > 0 ? maxConnections : mMaxConnections;
}

void UdpServer :: setReqQueueSize( int reqQueueSize, const char * refusedMsg )
{
	mReqQueueSize = reqQueueSize > 0 ? reqQueueSize : mReqQueueSize;
}

int UdpServer :: registerEvent(EventGlobal& evarg) {
	int ret = 0;

	ret = IOUtils::tcpListen( mBindIP, mPort, &mListenFD, 0 );
	GLOGW("create listenid:%d ret:%d\n", mListenFD, ret);

	evarg.setMaxConnections(mMaxConnections);

	event_set( &mEvAccept, mListenFD, EV_READ|EV_PERSIST, EventActor::onAccept, &evarg );
	event_base_set( evarg.getEventBase(), &mEvAccept );
	event_add( &mEvAccept, NULL );

	return ret;
}

void UdpServer :: shutdown() {
	event_del( &mEvAccept);
	if(mListenFD>0) {
		close(mListenFD);
		mListenFD = 0;
		GLOGW("close listenid:%d\n", mListenFD);
	}
}


