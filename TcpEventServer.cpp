/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>

#include "TcpServer.h"
#include "ReactorStation.h"


int main( int argc, char * argv[] )
{
	int port = TEST_PORT, maxThreads = 10;
	//const char * serverType = "hahs";

	extern char *optarg ;
	int c ;

	while( ( c = getopt ( argc, argv, "p:t:s:v" )) != EOF ) {
		switch ( c ) {
			case 'p' :
				port = atoi( optarg );
				break;

			case 't' :
				maxThreads = atoi( optarg );
				break;

			case 's' :
				//serverType = optarg;
				break;

			case '?' :
			case 'v' :
				printf( "Usage: %s [-p <port>] [-t <threads>] [-s <hahs|lf>]\n", argv[0] );
				exit( 0 );
				break;
		}
	}

	int count = 0;
	while(1) {
		ReactorStation station;
		station.startup();

		TcpServer server( "127.0.0.1", port );
		server.setMaxThreads( maxThreads );
		server.setReqQueueSize( 100, "HTTP/1.1 500 Sorry, server is busy now!\r\n" );
		server.registerEvent(station.getEventArg());

		getchar();

		server.shutdown();
		station.shutdown();
		printf( "______shutdown done %d\n", ++count);
	}


	return 0;
}



