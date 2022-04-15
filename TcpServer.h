

#ifndef __tcpserver_h__
#define __tcpserver_h__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "EventActor.h"
#include "TaskBase.h"

#include "event.h"

#define TEST_PORT 31005

// half-sync/half-async thread pool server
class TcpServer {
public:
	TcpServer( const char * bindIP, int port );
	~TcpServer();

	int registerEvent(EventGlobal& evarg);
	void shutdown();

	void setMaxConnections( int maxConnections );

private:

	char mBindIP[ 64 ];
	int mPort;
	int mListenFD;

	int mMaxConnections;

	struct event mEvAccept;
};

#endif

