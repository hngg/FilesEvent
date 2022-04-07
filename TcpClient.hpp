#ifndef __tcpclient_hpp__
#define __tcpclient_hpp__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "EventCall.hpp"

class Session;

// half-sync/half-async thread pool server
class TcpClient {
public:
	TcpClient( );
	virtual ~TcpClient();

	int connect(const char* destIp, unsigned short destPort);
	int connect(const char* destIp, unsigned short destPort, const char*filepath);
	int connect(const char* destIp, unsigned short destPort, void *surface); //real show
	int connect(const char* destIp, unsigned short destPort, const char*filepath, void *surface); //get h264 show
	int connect(const char* destIp, unsigned short destPort, const char*remoteFile, const char*saveFile);
	int disConnect();
	int registerEvent(const EventArg& evarg);

private:
	int 			mSockId;
	Sid_t 			mSid;
	Session			*mSession;
};


#endif
