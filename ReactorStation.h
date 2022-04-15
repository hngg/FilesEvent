#ifndef __reactorstation_h__
#define __reactorstation_h__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "event.h"

#include "EventActor.h"
#include "TaskBase.h"


class ReactorStation {
	public:
		ReactorStation( );
		~ReactorStation();

		EventGlobal& getEventArg();

		int startup();
		int isRunning();
		void shutdown();
		void releaseSessions();

		void setTimeout( int timeout );

	private:
		int start();
		int run();
		static void *eventLoop( void * arg );
		static void sigHandler( int, short, void * arg );

		EventGlobal mEveGlobal;
		int mIsShutdown;
		int mIsRunning;
		int mTimeout;
};

#endif

