
#ifndef __tcpclient_h__
#define __tcpclient_h__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "EventGlobal.h"

//class Session;

// half-sync/half-async thread pool server
class TcpClient 
{
	public:
		TcpClient( );
		virtual ~TcpClient();

		// int connect(const char* destIp, unsigned short destPort, const char*filepath, void *surface); //get h264 show
		int connect(const char* destIp, unsigned short destPort, const char* remoteFile, const char* saveFile);
		int disConnect();
		int registerEvent(EventGlobal* evglobal);

		int fetchAndSaveFile(int key, const char* remoteFile, const char* saveFile);
		int cancelFetchingFile(int key);

	private:
		int 			mSockId;
		Session			*mSession;
};


#endif
