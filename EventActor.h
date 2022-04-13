

#ifndef __event_actor_h__
#define __event_actor_h__


#include "TaskBase.h"

struct event_base;

class Session;
class SessionManager;

class EventGlobal {
public:
	EventGlobal();
	EventGlobal( int timeout );
	~EventGlobal();
	int Create();
	int Destroy();

	struct event_base * getEventBase() const;

	SessionManager * getSessionManager() const;

	void setTimeout( int timeout );
	int  getTimeout() const;

private:
	struct event_base * mEventBase;
	SessionManager * mSessionManager;
	int mTimeout;
};

typedef struct tag_AcceptArg {
	EventGlobal * mEventArg;

	int mReqQueueSize;
	int mMaxConnections;
	char * mRefusedMsg;
} AcceptArg_t;


class EventActor {

	public:
		static void onAccept( int fd, short events, void * arg );
		static void onRead( int fd, short events, void * arg );
		static void onWrite( int fd, short events, void * arg );
		static void onTimer( int fd, short events, void * arg );
		static void addEvent( Session * session, short events, int fd );

		static void onResponse( void * queueData, void * arg );

	private:
		EventActor();
		~EventActor();
};

class EventHelper {

	public:
		static void doStart( Session * session );
		static void start( void * arg );

		static void doWork( Session * session );
		static void worker( void * arg );

		static void doError( Session * session );
		static void error( void * arg );

		static void doTimeout( Session * session );
		static void timeout( void * arg );

		//static void doCompletion( EventArg * eventArg, SP_Message * msg );

		static int isSystemSid( Sid_t * sid );

	private:
		EventHelper();
		~EventHelper();
};

#endif

