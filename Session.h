
#ifndef __session_h__
#define __session_h__

#include <unistd.h>
#include "TaskBase.h"


class Session 
{
	public:
		// Session( Sockid_t sid, short type, char*filepath, void*surface); //get h264 show
		Session( Sockid_t sid, short type, char*remoteFile, char*saveFile);
		virtual ~Session();

		int addReadEvent();
		int addWriteEvent();
		int addTimerEvent();
		struct event * getReadEvent();	
		struct event * getWriteEvent();
		struct event * getTimeEvent();

		void setGlobal( void * arg );
		void * getGlobal();

		Sockid_t getSid();

		enum { eNormal, eWouldExit, eExit };
		void setStatus( int status );
		int  getStatus();

		int setHeartBeat();

		int readBuffer();
		int writeBuffer();

		int getRunning();
		void setRunning( int running );

		int getReading();
		void setReading( int reading );

		int getWriting();
		void setWriting( int writing );

		static void onRead( int fd, short events, void * arg );
		static void onWrite( int fd, short events, void * arg );
		static void onTimer( int fd, short events, void * arg );

		static void addEvent( Session * session, short events, int fd);
		static void addEvent( Session * session, short events, int fd , void (*callback)(int, short, void *));
		
	private:
		Session( Session & );
		Session & operator=( Session & );
		int recvEx(char*pData, int len);
		int recvPackData();


		Sockid_t mSid;
		TaskBase *mTaskBase;

		struct event * mReadEvent;
		struct event * mWriteEvent;
		struct event * mTimeEvent;

		void * mArg;


		char mStatus;
		char mRunning;
		char mWriting;
		char mReading;

		int  mHeadLenConst;//const
		int  mTotalDataLen;
		int  mHasRecvLen;
		bool mbRecvHead;

		char mReadBuff[1500];
};

////////////////////////////////////////////////SessionManager//////////////////////////////////////////////////

typedef struct tagSessionEntry SessionEntry_t;

class SessionManager 
{
	public:
		SessionManager();
		~SessionManager();

		void setRealView(int sockId, void*surface);

		int getCount();
		void put( uint16_t key, Session * session, uint16_t * seq );
		Session * get( uint16_t key, uint16_t * seq );
		Session * remove( uint16_t key, uint16_t * seq = NULL );

	private:
		int mCount;
		SessionEntry_t * mArray[ 64 ];
};

#endif

