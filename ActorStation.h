#ifndef __actorstation_h__
#define __actorstation_h__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "event.h"

#include "EventCall.h"
#include "TaskBase.h"


class ActorStation {
	public:
		ActorStation(  );
		~ActorStation();

		const EventArg& getEventArg();

		int startup();
		int isRunning();
		void shutdown();
		void releaseSessions();

		void setTimeout( int timeout );

	private:
		static void * eventLoop( void * arg );
		int start();
		int run();
		static void sigHandler( int, short, void * arg );

		EventArg 	mEventArg;
		int mIsShutdown;
		int mIsRunning;
		int mTimeout;
};

#endif

