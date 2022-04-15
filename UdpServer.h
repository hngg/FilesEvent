

#ifndef __udpserver_h__
#define __udpserver_h__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "event.h"

#include "EventActor.h"
#include "TaskBase.h"

// half-sync/half-async thread pool server
class UdpServer {
public:
	UdpServer( const char * bindIP, int port );
	~UdpServer();

	int registerEvent(EventGlobal& evarg);
	void shutdown();

	void setMaxConnections( int maxConnections );
	void setMaxThreads( int maxThreads );
	void setReqQueueSize( int reqQueueSize, const char * refusedMsg );

/*	const EventArg& getEventArg();

	void shutdown();
	int isRunning();
	int run();
	void runForever();*/

private:

	char mBindIP[ 64 ];
	int mPort;
	int mIsShutdown;
	int mIsRunning;
	int mListenFD;


	int mMaxThreads;
	int mMaxConnections;
	int mReqQueueSize;

	struct event mEvAccept;

//	static void * eventLoop( void * arg );
//	int start();
//	static void sigHandler( int, short, void * arg );
//	static void outputCompleted( void * arg );
};

#endif

