

#ifndef __tcpserver_h__
#define __tcpserver_h__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "EventGlobal.h"
#include "TaskBase.h"

#include "event.h"

#define TEST_PORT 31005

// half-sync/half-async thread pool server
class TcpServer 
{
	public:
		TcpServer( const char * bindIP, int port );
		~TcpServer();

		int registerEvent(EventGlobal* evglobal);
		void shutdown();
		void setMaxConnections( int maxConnections );

		static void onAccept( int listenFd, short events, void * arg );

	private:
		char mBindIP[ 64 ];
		int mPort;
		int mListenFD;

		int mMaxConnections;

		struct event *mEvAccept;
};

#endif

