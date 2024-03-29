
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "event.h"
#include "protocol.h"
#include "basedef.h"

#include "BaseUtils.h"
#include "Session.h"
#include "TaskFileSend.h"
#include "BufferCache.h"


	TaskFileSend::TaskFileSend(Session* sess, Sockid_t& sid)
				:mPackHeadLen(sizeof(NET_CMD))
				,TaskBase(sess, sid)
				,mSess(sess)
				,mpFile(NULL)
				,mRecvDataLen(0)
				,mRecvHeadLen(0)
				,mFileLen(0)
				,mHasReadLen(0)
				,mFrameCount(0)
				,mbSendingData(true)
	{
		mRecvBuffer.reset();
		mSendBuffer.reset();

		mInBuffer = new BufferCache();
	}

	TaskFileSend::~TaskFileSend() 
	{
		if(mInBuffer)
		{
			delete mInBuffer;
			mInBuffer = NULL;
		}

		if(mpFile) 
		{
			fclose(mpFile);
			mpFile = NULL;
		}
			
		mMsgQueue.clearQueue();
		mSendBuffer.releaseMem();
	}

	int TaskFileSend::setFetchFile(const char* filepath)
	{
		int rest = -1;
		if(mpFile)
			return rest;

		mpFile = fopen(filepath, "rb");
	    struct stat buf;
	    stat(filepath, &buf);
	    mFileLen = buf.st_size;
//		fseek( mpFile, 0, SEEK_END );
//		mFileLen = ftell( mpFile );
//		fseek( mpFile, 0, SEEK_END );
//		rewind( mpFile );

		char *lpRet   = mSendBuffer.cmmd;
		LPNET_CMD cmd = (LPNET_CMD)lpRet;
		cmd->dwFlag   = NET_FLAG;
		cmd->dwCmd    = MODULE_MSG_LOGINRET;
		cmd->dwIndex  = 0;
		cmd->dwLength = sizeof(LOGIN_RET);

		LOGIN_RET loginRet 	= {0};
		FILE_INFO info 		= {0};
		info.tmEnd 			= mFileLen;
		loginRet.nLength 	= sizeof(FILE_INFO);
		loginRet.lRet 		= ERR_NOERROR;
		memcpy(loginRet.lpData, &info, sizeof(FILE_INFO));
		memcpy(cmd->lpData, &loginRet, sizeof(LOGIN_RET));

		mSendBuffer.totalLen 	= sizeof(NET_CMD) + sizeof(LOGIN_RET);
		mSendBuffer.bProcCmmd 	= true;
		int ret = tcpSendData();

		log_warn("filepath %s len:%u", filepath, mFileLen);		//get_filesize(filename)

		return 0;
	}

	int TaskFileSend::sendVariedCmd(int iVal) 
	{
		LPNET_CMD  pCmd = (LPNET_CMD)mSendBuffer.cmmd;
		pCmd->dwFlag 	= NET_FLAG;
		pCmd->dwCmd 	= iVal;
		pCmd->dwIndex 	= 0;
		pCmd->dwLength 	= 0;
		mSendBuffer.totalLen = sizeof(NET_CMD);
		mSendBuffer.bProcCmmd = true;
		int ret = tcpSendData();
		//GLOGE("-------------------sendVariedCmd:%d", iVal);
		return ret;
	}

	//Here further processing is needed.
	int TaskFileSend::writeBuffer() 
	{
		int ret = 0;

		if(mSendBuffer.totalLen==0) //take new data and send,totalLen is cmd len first
		{
			if(mHasReadLen<mFileLen) 
			{
				if(mMsgQueue.getSize()>0) 
				{
					int val = 0;
					mMsgQueue.try_pop(val);
					return sendVariedCmd(val);
				}

				if(!mbSendingData)
					return 0;

				unsigned int iLeftLen 	= mFileLen - mHasReadLen;
				unsigned int iBuffLen 	= (iLeftLen > FILE_MEMORY_LEN)?FILE_MEMORY_LEN:iLeftLen;
				mSendBuffer.totalLen 	= sizeof(NET_CMD) + sizeof(FILE_GET);
				mSendBuffer.bProcCmmd 	= true;
				LPNET_CMD	 cmd 		= (LPNET_CMD)mSendBuffer.cmmd;
				LPFILE_GET frame 		= (LPFILE_GET)(cmd->lpData);
				cmd->dwFlag 			= NET_FLAG;
				cmd->dwCmd 				= MODULE_MSG_FILES;
				cmd->dwIndex 			= 0;

				mSendBuffer.createMem(iBuffLen);
				memset(mSendBuffer.data, 0, iBuffLen);
				frame->dwPos 			= ftell(mpFile);
				mSendBuffer.dataLen  	= fread(mSendBuffer.data, 1, iBuffLen, mpFile);

				cmd->dwLength 			= mSendBuffer.dataLen+sizeof(FILE_GET); 	//cmd incidental length
				frame->nLength  		= mSendBuffer.dataLen;

				mHasReadLen 			+= mSendBuffer.dataLen;

				mFrameCount++;
				log_info("fread data len:%d mHasReadLen:%d cmd mFrameCount:%d", mSendBuffer.dataLen, mHasReadLen, mFrameCount);
			}
			else
			{
				#ifdef __ANDROID__
					FileRecvCallback(0, SEND_TELL_FILE_END, mFileLen);
				#endif
				ret = pushSendCmd(MODULE_MSG_DATAEND);
				return OWN_SOCK_EXIT;
			}
		}

		ret = tcpSendData();

		return ret;
	}

	int TaskFileSend::setHeartCount() 
	{
//			mMsgQueue.push(MODULE_MSG_PING);
//
//			if( !mbSendingData)
//				EventCall::addEvent( mSess, EV_WRITE, -1 );
//
//			//GLOGE("setHeart----------------\n");
//
//		return mMsgQueue.getSize();
		return 0;
	}

	int TaskFileSend::sendEx(char* data, int len) 
	{
		int leftLen = len, iRet = 0;

		struct timeval timeout;
		int sockId = mSid.sid;
		do {

			iRet = send(sockId, data+len-leftLen, leftLen, 0);
			if(iRet<0) 
			{
				//GLOGE("send data errno:%d ret:%d.", errno, iRet);
				switch(errno) 
				{
					case EAGAIN:
						usleep(2000);
						continue;

					case EPIPE:
						break;
				}
				return iRet;
			}

			leftLen -= iRet;

		}while(leftLen>0);

		return len - leftLen;
	}

	int TaskFileSend::tcpSendData()
	{
		int ret = 0;
		if(mSendBuffer.bProcCmmd) 
		{
			ret = sendEx(mSendBuffer.cmmd+mSendBuffer.hasProcLen, mSendBuffer.totalLen-mSendBuffer.hasProcLen);
			if(ret>0)
				mSendBuffer.hasProcLen += ret;
			else
				log_error("tcpSendData cmd errno:%d ret:%d.", errno, ret);

			if(mSendBuffer.hasProcLen == mSendBuffer.totalLen) 
			{
				mSendBuffer.setToVideo();
			}
		}
		else//data
		{
			ret = sendEx((char*)mSendBuffer.data+mSendBuffer.hasProcLen, mSendBuffer.totalLen-mSendBuffer.hasProcLen);
			if(ret>0)
				mSendBuffer.hasProcLen += ret;
			else
				log_error("tcpSendData dta errno:%d ret:%d .", errno, ret);

			if(mSendBuffer.hasProcLen == mSendBuffer.totalLen) 
			{
				mSendBuffer.reset();
			}
		}
		return ret;
	}

	int TaskFileSend::pushSendCmd(int iVal, int index) 
	{
		int ret = 0;
		LPNET_CMD	pCmd = (LPNET_CMD)mSendBuffer.cmmd;
		switch(iVal) 
		{
			case MODULE_MSG_DATAEND:
			case MODULE_MSG_SECTION_END:
				pCmd->dwFlag 	= NET_FLAG;
				pCmd->dwCmd 	= iVal;
				pCmd->dwIndex 	= index;
				pCmd->dwLength 	= 0;

				mSendBuffer.totalLen = sizeof(NET_CMD);
				mSendBuffer.bProcCmmd = true;
				ret = tcpSendData();
				break;

			case MODULE_MSG_PING:
				if(mMsgQueue.getSize() < 10)
					mMsgQueue.push(MODULE_MSG_PING);
				break;
		}
		log_info("pushSendCmd value:%d ret:%d.", iVal, ret);

		return ret;
	}

	int TaskFileSend::readBuffer() 
	{
		int ret = -1;
		int &hasRecvLen = mRecvBuffer.hasProcLen;
		if(mRecvBuffer.bProcCmmd) 
		{
			ret = recv(mSid.sid, mRecvBuffer.cmmd+hasRecvLen, mPackHeadLen-hasRecvLen, 0);
			if(ret>0) 
			{
				hasRecvLen+=ret;
				if(hasRecvLen==mPackHeadLen) 
				{
					LPNET_CMD head = (LPNET_CMD)mRecvBuffer.cmmd;
					mRecvBuffer.totalLen  = head->dwLength;
					mRecvBuffer.bProcCmmd = false;
					hasRecvLen = 0;

					log_info("sendfile flag:%08x totalLen:%d ret:%d", head->dwFlag, mRecvBuffer.totalLen, ret);
					ret = recvPackData();
				}
			}
		}//
		else
		{
			ret = recvPackData();
		}
		return ret;
	}

	int TaskFileSend::recvPackData() 
	{
		int &hasRecvLen = mRecvBuffer.hasProcLen;
		int ret = recv(mSid.sid, mRecvBuffer.cmmd+mPackHeadLen+hasRecvLen, mRecvBuffer.totalLen-hasRecvLen, 0);
		//GLOGE("-------------------recvPackData ret:%d\n",ret);
		if(ret>0) 
		{
			hasRecvLen += ret;
			if(hasRecvLen==mRecvBuffer.totalLen) 
			{
				int lValueLen;
			    char acValue[256] = {0};	//new char[256];
			    memset(acValue, 0, 256);
				LPNET_CMD pCmdbuf = (LPNET_CMD)mRecvBuffer.cmmd;
				if(pCmdbuf->dwCmd == MODULE_MSG_CONTROL_PLAY) 
				{
					PROTO_GetValueByName(mRecvBuffer.cmmd, (char*)"name", acValue, &lValueLen);
					log_info("recv control commond acValue:%s",acValue);

					if (strcmp(acValue, "start") == 0) 
					{
						memset(acValue, 0, 256);
						PROTO_GetValueByName(mRecvBuffer.cmmd, (char*)"tmstart", acValue, &lValueLen);
						log_info("tmstart:%d",atoi(acValue));

						memset(acValue, 0, 256);
						PROTO_GetValueByName(mRecvBuffer.cmmd, (char*)"tmend", acValue, &lValueLen);
						log_info("tmend:%d",atoi(acValue));
						//Session::addEvent( mSess, EV_WRITE, -1 );
						mSess->addWriteEvent();
					}
					else if(strcmp(acValue, "setpause") == 0) 
					{
						memset(acValue, 0, 256);
						PROTO_GetValueByName(mRecvBuffer.cmmd, (char*)"value", acValue, &lValueLen);
						int value = atoi(acValue);
						log_info("control setpause value:%d", value);
					}
					else if(strcmp(acValue, "send") == 0) 
					{
						mbSendingData = true;
						//EventActor::addEvent( mSess, EV_WRITE, -1 );
						mSess->addWriteEvent();
					}
				}
			    //GLOGE("recv total:%s", mRecvBuffer.buff);

			    mRecvBuffer.reset();
			}
		}
		else if(ret == 0) 
		{

		}

		return ret;
	}



