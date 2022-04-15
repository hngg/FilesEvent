
#ifndef __event_actor_h__
#define __event_actor_h__

#include "TaskBase.h"


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

	void setMaxConnections( int connection );
	int  getMaxConnections() const;

private:
	struct event_base * mEventBase;
	SessionManager * mSessionManager;

	int mTimeout;
	int mReqQueueSize;
	int mMaxConnections;
};


class EventActor {

	public:
		static void onAccept( int listenFd, short events, void * arg );
		static void onRead( int fd, short events, void * arg );
		static void onWrite( int fd, short events, void * arg );
		static void onTimer( int fd, short events, void * arg );
		static void addEvent( Session * session, short events, int fd );

	private:
		EventActor();
		~EventActor();
};

#endif

