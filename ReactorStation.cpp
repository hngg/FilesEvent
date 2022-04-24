
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "ReactorStation.h"
#include "basedef.h"

#define EVENT_TIMEOUT 1000 //ms

	ReactorStation :: ReactorStation( )
					:mIsStartup(0)
					,mTimeout(EVENT_TIMEOUT)
					,mEveGlobal(NULL)
	{
	}

	ReactorStation :: ~ReactorStation()
	{
		log_warn("~ReactorStation Destroy.");
	}

	EventGlobal* ReactorStation :: getEventGlobal()
	{
		return mEveGlobal;
	}

	/**
	 * @brief 
	 * 
	 * @return int 1 is success, 0 is failed
	 */
	int ReactorStation :: startup()
	{
		log_warn("ReactorStation startup global value is %s", (NULL==mEveGlobal)?"null":"no null");
		if(NULL == mEveGlobal) 
		{
			mEveGlobal = new EventGlobal();
			mEveGlobal->Create();
			mEveGlobal->setTimeout(mTimeout);

			pthread_mutex_init(&mExitMutex, NULL);

			mIsStartup = 1;
			run();

			return 1;
		}
		return 0;
	}

	/**
	 * @brief 
	 * 
	 * @return int 1 is success, 0 is failed
	 */
	int ReactorStation :: shutdown()
	{
		if(mEveGlobal)
		{
			mIsStartup = 0;

			struct timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec= 10;
			int rest = event_base_loopexit(mEveGlobal->getEventBase(), &tv);

			pthread_mutex_lock(&mExitMutex);
			mEveGlobal->Destroy();
			delete mEveGlobal;
			mEveGlobal = NULL;
			pthread_mutex_unlock(&mExitMutex);

			pthread_mutex_destroy(&mExitMutex);

			log_warn("ReactorStation shutdown mIsStartup is:%d exit rest:%d", mIsStartup, rest);
			
			return 1;
		}
		else
			log_error("shutdown mEveGlobal is null.");

		return 0;
	}

	int ReactorStation :: isStartup()
	{
		return mIsStartup;
	}

	int ReactorStation :: run()
	{
		int ret = -1;

		pthread_attr_t attr;
		pthread_attr_init( &attr );
		assert( pthread_attr_setstacksize( &attr, 1024 * 1024 ) == 0 );
		pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

		pthread_t thread = 0;
		ret = pthread_create( &thread, &attr, reinterpret_cast<void*(*)(void*)>(eventLoop), this );
		pthread_attr_destroy( &attr );
		if( 0 == ret ) 
		{
			log_warn( "Thread #%ld has been created to loop.", thread );
		} 
		else 
		{
			log_error( "Unable to create a thread for TCP server, %s", strerror( errno ) ) ;
		}

		log_warn("ActorStation run ret:%d", ret);

		return ret;
	}

	void* ReactorStation :: eventLoop( void * arg ) 
	{
		ReactorStation * station = (ReactorStation*)arg;

		pthread_mutex_lock(&station->mExitMutex);
		station->startThread(arg);
		pthread_mutex_unlock(&station->mExitMutex);
		
		log_warn("eventLoop end___$");

		return NULL;
	}

	int ReactorStation :: startThread(void * arg) 
	{
		/* Don't die with SIGPIPE on remote read shutdown. That's dumb. */
		signal( SIGPIPE, SIG_IGN );

		ReactorStation* station 	= (ReactorStation*)arg;
		EventGlobal* eveGlobal 		= station->getEventGlobal();

		// Clean close on SIGINT or SIGTERM.
		struct event evSigInt, evSigTerm;
		signal_set( &evSigInt, SIGINT,  sigHandler, this );
		event_base_set( eveGlobal->getEventBase(), &evSigInt );
		signal_add( &evSigInt, NULL);

		signal_set( &evSigTerm, SIGTERM, sigHandler, this );
		event_base_set( eveGlobal->getEventBase(), &evSigTerm );
		signal_add( &evSigTerm, NULL);

		/* Start the event loop. */
		while( 1 == station->isStartup() )
		{
			//log_warn("___________mIsShutdown:%d", mIsShutdown);
			event_base_loop( eveGlobal->getEventBase(), EVLOOP_NONBLOCK );
		}

		//eveGlobal->Destroy();
		log_warn("ReactorStation loop___ exit.");

		signal_del( &evSigTerm );
		signal_del( &evSigInt );
		
		return 0;
	}

	void ReactorStation :: sigHandler( int fd, short event, void * arg ) 
	{
		ReactorStation * station = (ReactorStation*)arg;
		station->shutdown();
		
		log_error("sigHandler fd:%d event:%d.", fd, event);
	}

	// void ReactorStation :: setTimeout( int timeout )
	// {
	// 	mTimeout = timeout > 0 ? timeout : mTimeout;
	// }