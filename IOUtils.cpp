
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>

#include "basedef.h"
#include "IOUtils.h"

#define  MAX_MTU 1400

void IOUtils :: inetNtoa( in_addr* addr, char* ip, int size )
{
	#if defined (linux) || defined (__ANDROID__) || defined (__sgi) || defined (__hpux) || defined (__FreeBSD__)
		const unsigned char *p = ( const unsigned char *) addr;
		snprintf( ip, size, "%i.%i.%i.%i", p[0], p[1], p[2], p[3] );
	#else
		snprintf( ip, size, "%i.%i.%i.%i", addr->s_net, addr->s_host, addr->s_lh, addr->s_impno );
	#endif
}

/**
 * blocking:
 * 1->block
 * 0->nonblock
 */
int IOUtils :: setBlock( int fd, int blocking)
{
	int flags;

	flags = fcntl( fd, F_GETFL );
	if( flags < 0 ) 
		return flags;

	if(blocking)
		flags &= ~O_NONBLOCK;
	else
		flags |= O_NONBLOCK;
		
	if( fcntl( fd, F_SETFL, flags ) < 0 ) 
		return -1;

    // if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0)|O_NONBLOCK) == -1)
    // {
    //  	return -1;
    // }

	return 0;
}

/**
 * 0 nonblock, 1 block
 * return success: bindPort, failed: <0
 */
int IOUtils :: tcpListen( const char* ip, int bindPort, int* outListenId, int tcpdelay )
{
	int listenFd = socket(AF_INET, SOCK_STREAM, 0);
	if( listenFd < 0 ) 
	{
		log_error("listen failed, errno %d, %s", errno, strerror( errno ) );
		return -errno;
	}

	//0 is set socket nonblock
	if( setBlock( listenFd, 0) < 0 ) 
	{
		log_error( "failed to set socket to non-blocking" );
	}
	
	int flags = 1;
	if( setsockopt( listenFd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof( flags ) ) < 0 ) 
	{
		log_error( "failed to set setsock to reuseaddr" );
	}

	if( 0 == tcpdelay ) 
	{
		int flags = 1;
		if( setsockopt( listenFd, IPPROTO_TCP, TCP_NODELAY, (char*)&flags, sizeof(flags) ) < 0 ) 
		{
			log_error("failed to set socket to nodelay" );
		}
	}

	struct sockaddr_in addr;

	memset(&addr, 0, sizeof( addr ));
	addr.sin_family 		= AF_INET;
	addr.sin_port 			= htons( bindPort );
	addr.sin_addr.s_addr 	= inet_addr(ip);//INADDR_ANY;

	if( bind( listenFd, (struct sockaddr*)&addr, sizeof( addr ) ) < 0 ) 
	{
		log_error("bind failed, errno %d, %s", errno, strerror( errno ) );
		return -errno;
	}

	// if( '\0' != *ip ) {
	// 	if( 0 != inet_aton( ip, &addr.sin_addr ) ) {
	// 		log_error("failed to convert %s to inet_addr", ip );
	// 		ret = -1;
	// 	}
	// }


	if( ::listen( listenFd, 5 ) < 0 ) 
	{
		close( listenFd );
		log_error("listen failed, errno %d, %s", errno, strerror( errno ) );
		return -errno;
	}

	*outListenId = listenFd;
	log_info("Listen on port [%d]", bindPort );

	return bindPort;
}

int IOUtils :: tcpConnect(const char* destip, int destport, int* outConnectId, int tcpdelay)
{
	int ret = 0;
	int sockid = socket(AF_INET, SOCK_STREAM, 0);
	if(sockid < 0) 
	{
		log_error("socket() failure val:%d", sockid);
		return -1;
	}

	if( 0 == tcpdelay ) 
	{
		int flags = 1;
		if( setsockopt( sockid, IPPROTO_TCP, TCP_NODELAY, (char*)&flags, sizeof(flags) ) < 0 ) 
		{
			log_error("failed to set socket to nodelay" );
			ret = -1;
		}
	}

	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port   = htons(destport);
	addr.sin_addr.s_addr = inet_addr(destip);

	if((ret = connect(sockid, (struct sockaddr*)&addr, sizeof(addr)) ) < 0) 
	{
		close( sockid );
		log_error("bind failed, ret:%d errno %d, %s ", ret, errno, strerror( errno ) );
		return -errno;
	}

	*outConnectId = sockid;
	log_info("connect %s %d success socketid:%d", destip, destport, sockid);

	return ret;
}

int IOUtils :: tcpSendData(int fd, char* data, int len)
{
	int sendLen = len, iRet = 0;
	if(len<= MAX_MTU)
		iRet += send(fd, data, len,0);
	else 
	{
		while(sendLen>0)
		{
			if(sendLen<=MAX_MTU)
		     		iRet += send(fd, data+len-sendLen, sendLen,0);
			else  	
				iRet += send(fd, data+len-sendLen, MAX_MTU,0);

			sendLen -= MAX_MTU;
		}
	}
	return iRet;
}

void setPortReuse(int listener) 
{
	  /*设置socket属性，端口可以重用*/
   int opt=SO_REUSEADDR;
   setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}


