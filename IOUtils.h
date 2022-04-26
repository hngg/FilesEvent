

#ifndef __ioutils_h__
#define __ioutils_h__

#include <netinet/in.h>

class IOUtils 
{
	public:
		static void inetNtoa( in_addr* addr, char* ip, int size );

		static int setBlock( int fd, int blocking);

		static int tcpListen( const char* ip, int bindPort, int* outListenId, int tcpdelay = 0 );
		
		static int tcpConnect(const char* destip, int destport, int* outConnectId, int tcpdelay = 0);
		//static int tcpSendPackage(int fd, char*data, int len);
		static int tcpSendData(int fd, char* data, int len);

	private:
		IOUtils();
};

#endif

