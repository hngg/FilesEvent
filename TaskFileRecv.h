#ifndef __TaskFileRecv_h__
#define __TaskFileRecv_h__


#include <sys/time.h> //->gettimeofday define

#include "basedef.h"
#include "TaskBase.h"

class BufferCache;

class TaskFileRecv :public TaskBase 
{
	public:
		// TaskFileRecv( Session*sess, Sockid_t &sid );
		// TaskFileRecv( Session*sess, Sockid_t &sid, char*remoteFile );
		TaskFileRecv( Session*sess, Sockid_t &sid, char*remoteFile, char*saveFile );
		virtual ~TaskFileRecv();
		virtual int readBuffer();
		virtual int writeBuffer();

	private:
		int sendEx(void*data, int len);
		int SendCmd(int dwCmd, int dwIndex, void* lpData, int nLength);
		int recvPackData();

		struct tagFileProcBuffer 	mRecvBuffer;
		Session			*mSess;
		FILE			*mwFile;

		int 	mPackHeadLen;
		
		int  	mTotalLen;
		int  	mRecvDataLen;

		struct timeval mStartTime;
		struct timeval mEndTime;
};


#endif
