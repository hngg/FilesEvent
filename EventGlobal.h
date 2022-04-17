
#ifndef __event_global_h__
#define __event_global_h__

#include "TaskBase.h"


class Session;
class SessionManager;

class EventGlobal 
{
	public:
		EventGlobal();
		~EventGlobal();

		int Create();
		int Destroy();

		struct event_base* getEventBase() const;

		SessionManager* getSessionManager() const;

		void setTimeout( int timeout );
		int  getTimeout() const;

		void setMaxConnections( int connection );
		int  getMaxConnections() const;

	private:
		struct event_base* mEventBase;
		SessionManager* mSessionManager;

		int mTimeout;
		int mReqQueueSize;
		int mMaxConnections;
};

#endif

