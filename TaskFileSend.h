#ifndef __TaskFileSend_h__
#define __TaskFileSend_h__


#include "basedef.h"
#include "GQueue.h"
#include "TaskBase.h"


class BufferCache;

class TaskFileSend :public TaskBase 
{
	public:
		TaskFileSend(Session* sess, Sockid_t& sid);
		~TaskFileSend();

		//abstract function
		int setFetchFile(const char* filepath);
		int setHeartCount();
		int readBuffer();
		int writeBuffer();

	private:
		int tcpSendData();
		int sendEx(char* data, int len);
		int recvPackData();
		int pushSendCmd(int iVal, int index=0);

		int sendVariedCmd(int iVal);

	private:
		struct tagCmdBuffer 		mRecvBuffer;
		struct tagFileProcBuffer 	mSendBuffer;
		BufferCache 		 		*mInBuffer;
		FILE						*mpFile;
		Session						*mSess;
		GQueue<int>					mMsgQueue;

		int     		mFrameCount;
		unsigned int	mFileLen;

		int 			mPackHeadLen;

		int  			mRecvDataLen;
		int  			mRecvHeadLen;
		unsigned int    mHasReadLen;
		bool			mbSendingData;
};


#endif
