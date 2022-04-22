#ifndef __reactorstation_h__
#define __reactorstation_h__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "event.h"

#include "EventGlobal.h"
#include "TaskBase.h"


class ReactorStation 
{
	public:
		ReactorStation( );
		~ReactorStation();

		EventGlobal* getEventGlobal();

		int startup();
		int shutdown();

		int isStartup();

		//void setTimeout( int timeout );
		pthread_mutex_t	 mExitMutex;

	private:
		int startThread(void * arg);
		int run();
		static void *eventLoop( void * arg );
		static void sigHandler( int, short, void * arg );

		EventGlobal* mEveGlobal;
		int mIsStartup;
		int mTimeout;

		
};

#endif

