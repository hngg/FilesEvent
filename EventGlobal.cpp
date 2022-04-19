
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "config.h"   // from libevent, for event.h
#include "event.h"

#include "basedef.h"

#include "EventGlobal.h"
#include "Session.h"
#include "BufferCache.h"
#include "IOUtils.h"


#define TIMEOUT_US 1000000

EventGlobal :: EventGlobal()
		:mTimeout(0)
		,mEventBase(NULL)
		,mSessionManager(NULL) 
{

}

EventGlobal :: ~EventGlobal()
{
	//do clean
	if(mSessionManager) 
	{
		delete mSessionManager;
		mSessionManager = NULL;
	}
}

int EventGlobal ::Create() 
{
	int step = 0;
	if(mEventBase==NULL)
	{
		mEventBase = (struct event_base*)event_init();
		step++;
	}

	if(mSessionManager==NULL)
	{
		mSessionManager = new SessionManager();
		step++;
	}
		
	log_warn("global create step:%d", step);
	return 0;
}

int EventGlobal ::Destroy()
{
	int step = 0;
	//do clean
	if(mSessionManager!=NULL) 
	{
		log_warn("Manager session count:%d", mSessionManager->getCount());
		delete mSessionManager;
		mSessionManager = NULL;
		step++;
	}

	if(mEventBase) 
	{
		event_destroy();
		mEventBase = NULL;
		step++;
	}

	log_warn("global destroy step:%d", step);

	return 0;
}

struct event_base* EventGlobal :: getEventBase() const 
{
	return mEventBase;
}

SessionManager* EventGlobal :: getSessionManager() const
{
	return mSessionManager;
}

void EventGlobal :: setTimeout( int timeout )
{
	mTimeout = timeout;
}

int EventGlobal :: getTimeout() const
{
	return mTimeout;
}

void EventGlobal ::setMaxConnections( int connection )
{
	mMaxConnections = connection;
}

int  EventGlobal ::getMaxConnections() const 
{
	return mMaxConnections;
}

