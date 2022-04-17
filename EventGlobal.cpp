
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
	
	if(mEventBase) 
	{
		event_destroy();
		mEventBase = NULL;
	}
}

int EventGlobal ::Create() 
{
	if(mEventBase==NULL)
		mEventBase = (struct event_base*)event_init();

	if(mSessionManager==NULL)
		mSessionManager = new SessionManager();

	return 0;
}

int EventGlobal ::Destroy()
{
	//do clean
	if(mSessionManager!=NULL) 
	{
		log_warn("Manager session count:%d", mSessionManager->getCount());
		delete mSessionManager;
		mSessionManager = NULL;
	}

	if(mEventBase) 
	{
		event_destroy();
		mEventBase = NULL;
	}

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

