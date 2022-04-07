#ifndef __TaskVideoSend_hpp__
#define __TaskVideoSend_hpp__


#include "h264.h"

#include "TaskBase.hpp"
#include "GQueue.h"


class TaskVideoSend :public TaskBase {

	public:
		TaskVideoSend( Session*sess, Sid_t& sid, char*filename );
		virtual ~TaskVideoSend();

		virtual int setHeartCount();
		virtual int readBuffer();
		virtual int writeBuffer();

	private:
		int tcpSendData();
		int sendEx(char*data,int len);
		int recvPackData();
		int pushSendCmd(int iVal, int index=0);

		int sendVariedCmd(int iVal);

	private:
		struct tagRecvBuffer 		mRecvBuffer;
		struct tagNALSendBuffer 	mSendBuffer;
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
