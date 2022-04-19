
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "TaskFileRecv.h"
#include "BufferCache.h"

#include "event.h"
#include "basedef.h"

#include "BaseUtils.h"


enum FILE_RECV_STATUS{
	RECV_TELL_TOTAL_LENGTH = 1,	//file total length
	RECV_TELL_READ_LENGTH,		//current recv length
	RECV_TELL_END,				//file recv end
	RECV_TELL_SAVE_DONE,		//file save done
	RECV_TELL_USE_TIME			//file recv use time
};


#ifdef 	__ANDROID__
#define	FILE_PATH	"/sdcard/w.h264"
#else
#define	FILE_PATH	"recv.mp4"
#endif


	// TaskFileRecv::TaskFileRecv( Session*sess, Sockid_t &sid )
	// 			:mPackHeadLen(sizeof(NET_CMD))
	// 			,TaskBase(sid)
	// 			,mSess(sess)
	// 			,mRecvDataLen(0)
	// 			,mTotalLen(0)
	// {
	// 	//mCmdBuffer.reset();
	// 	mRecvBuffer.reset();
	// 	mRecvBuffer.createMem(FILE_MEMORY_LEN);

	// 	mwFile = fopen(FILE_PATH, "w");

	// 	char lpData[2048];
	// 	int nLength = sprintf(lpData, "<get path=\"%s\"/>", "/h264/tmp.mp4");


	// 	if(SendCmd(MODULE_MSG_LOGIN, 0, lpData, nLength)<0)
	// 		log_error("send CMD err:%s", lpData);
	// }


	// TaskFileRecv::TaskFileRecv( Session*sess, Sockid_t &sid, char*remoteFile )
	// 			:mPackHeadLen(sizeof(NET_CMD))
	// 			,TaskBase(sid)
	// 			,mSess(sess)
	// 			,mRecvDataLen(0)
	// 			,mTotalLen(0)
	// {
	// 	//mCmdBuffer.reset();
	// 	mRecvBuffer.reset();
	// 	mRecvBuffer.createMem(FILE_MEMORY_LEN);

	// 	mwFile = fopen(FILE_PATH, "w");

	// 	char lpData[2048];
	// 	int nLength = sprintf(lpData, "<get path=\"%s\"/>", remoteFile);


	// 	if(SendCmd(MODULE_MSG_LOGIN, 0, lpData, nLength)<0)
	// 		log_error("send CMD err:%s", lpData);
	// }

	TaskFileRecv::TaskFileRecv( Session*sess, Sockid_t &sid, char*remoteFile, char*saveFile )
				:mPackHeadLen(sizeof(NET_CMD))
				,TaskBase(sid)
				,mSess(sess)
				,mRecvDataLen(0)
				,mTotalLen(0)
	{
		mRecvBuffer.reset();
		mRecvBuffer.createMem(FILE_MEMORY_LEN+sizeof(NET_CMD));

		mwFile = fopen(saveFile, "w");

		char lpData[2048];
		int nLength = sprintf(lpData, "<get path=\"%s\"/>", remoteFile);

		if(SendCmd(MODULE_MSG_LOGIN, 0, lpData, nLength)<0)
			log_error("send CMD err:%s", lpData);
	}

	TaskFileRecv::~TaskFileRecv() 
	{
		log_info("file seek:%ld", ftell(mwFile));

		if(mwFile != NULL)
			fclose(mwFile);

		mRecvBuffer.releaseMem();
	}

	int TaskFileRecv::sendEx(void*data, int len) 
	{
		int leftLen = len, iRet = 0;

		struct timeval timeout;
		int sockId = mSid.sid;
		do {
			iRet = send(sockId, (char*)data+len-leftLen, leftLen, 0);

			if(iRet<0) 
			{
				//GLOGE("send data errno:%d ret:%d.", errno, iRet);
				switch(errno) 
				{
					case EAGAIN:	//11 Resource temporarily unavailable,try again
						usleep(2000);
						continue;

					case EPIPE:		//32 Broken pipe
						break;
				}
				return iRet;
			}

			leftLen -= iRet;

		}while(leftLen>0);

		return len - leftLen;
	}

	int TaskFileRecv::SendCmd(int dwCmd, int dwIndex, void* lpData, int nLength)
	{
		int iRet = -1;
		NET_CMD nc;
		memset(&nc,0,sizeof(nc));
		nc.dwFlag = NET_FLAG;
		nc.dwCmd = dwCmd;
		nc.dwIndex = dwIndex;
		nc.dwLength = nLength;
		if ((iRet = sendEx(&nc, sizeof(nc)))<0)
		{
			log_error("send cmd err len = %d ", nLength);
			return iRet;
		}

		log_info("send command head len:%d ", (int)sizeof(nc));
		if (nLength == 0)
		{
			return 0;
		}
		if ((iRet = sendEx(lpData, nLength))<0)
		{
			log_error("send lpdata err len = %d ",nLength);
			return iRet;
		}
		log_info("send command data len:%d lpData:%s", nLength, (char*)lpData);
		return iRet;
	}

	//read event callback function
	int TaskFileRecv::readBuffer() 
	{
		int ret = -1;
		int &hasRecvLen = mRecvBuffer.hasProcLen;
		if(mRecvBuffer.bProcCmmd) 
		{
			ret = recv(mSid.sid, mRecvBuffer.data+hasRecvLen, mPackHeadLen-hasRecvLen, 0);

			if(ret>0) 
			{
				hasRecvLen+=ret;
				if(hasRecvLen==mPackHeadLen) 
				{
					LPNET_CMD head = (LPNET_CMD)mRecvBuffer.data;
					mRecvBuffer.totalLen  = head->dwLength;//package data length
					mRecvBuffer.bProcCmmd = false;
					hasRecvLen = 0;

					log_info("playback flag:%08x cmd:%d read event need recv totalLen:%d ret:%d", 
								head->dwFlag, head->dwCmd, mRecvBuffer.totalLen, ret);

					if(head->dwLength>0) 
					{
						ret = recvPackData();
					}
					else
					{
						uint64_t tsdiff;
						switch(head->dwCmd) 
						{
							case MODULE_MSG_DATAEND:	//69
								gettimeofday(&mEndTime, NULL);
								tsdiff = (mEndTime.tv_sec-mStartTime.tv_sec)*1000000 + (mEndTime.tv_usec-mStartTime.tv_usec);
								#ifdef __ANDROID__
									FileRecvCallback(mSid.sid, RECV_TELL_END, mRecvDataLen);
									FileRecvCallback(mSid.sid, RECV_TELL_USE_TIME, tsdiff);
								#endif
								log_info("MODULE_MSG_DATAEND use time:%lld ms", tsdiff/1000);
							break;

							case MODULE_MSG_SECTION_END://70
//								char lpData[2048];
//								int nLength = sprintf(lpData, "<control name=\"start\" tmstart=\"10485760\" tmend=\"20485760\" />");
//								if(SendCmd(MODULE_MSG_CONTROL_PLAY, 0, lpData, nLength)<0)
//									GLOGE("send CMD err!");
							break;
						}//switch
					}
				}//==
			}//else return 0
			else
			{
				#ifdef 	__ANDROID__
					FileRecvCallback(mSid.sid, RECV_TELL_SAVE_DONE, 0);
				#endif
			}
		}//bProcCmmd
		else
		{
			ret = recvPackData();
		}

		//EventCall::addEvent( mSess, EV_READ, -1 );
		return ret;
	}

	//receive file data,head length is 16byte,other is data
	int TaskFileRecv::recvPackData() 
	{
		int &hasRecvLen = mRecvBuffer.hasProcLen;
		int ret = recv(mSid.sid, mRecvBuffer.data+mPackHeadLen+hasRecvLen, mRecvBuffer.totalLen-hasRecvLen, 0);
		log_info("------recvPackData ret:%d hasRecvLne:%d totalLen:%d", ret, hasRecvLen, mRecvBuffer.totalLen);
		if(ret>0) 
		{
			hasRecvLen += ret;
			
			if(hasRecvLen==mRecvBuffer.totalLen) 
			{
				int lValueLen;
			    char acValue[256] = {0};	//new char[256];
			    memset(acValue, 0, 256);
				LPNET_CMD pCmdbuf = (LPNET_CMD)mRecvBuffer.data;
				LPLOGIN_RET lpRet;
				LPFILE_GET  lpFrame;
				switch(pCmdbuf->dwCmd) {

					case MODULE_MSG_CONTROL_PLAY: //68
						break;

					case MODULE_MSG_FILES:	//25
						lpFrame 		= (LPFILE_GET)(pCmdbuf->lpData);
						fwrite(lpFrame->lpData , 1 , lpFrame->nLength , mwFile);
						fflush(mwFile);
						log_info("frame len:%d", lpFrame->nLength);
						mRecvDataLen += lpFrame->nLength;
						#ifdef 	__ANDROID__
							FileRecvCallback(mSid.sid, RECV_TELL_READ_LENGTH, mRecvDataLen);
						#endif
						break;

					case MODULE_MSG_LOGINRET: //2
						LPLOGIN_RET lpRet = (LPLOGIN_RET)(mRecvBuffer.data+mPackHeadLen);
						LPFILE_INFO lpInfo= (LPFILE_INFO)lpRet->lpData;
						mTotalLen = lpInfo->tmEnd;

						log_info("The file to recv TotalLen:%d", mTotalLen);
						gettimeofday(&mStartTime, NULL);

						char szCmd[100];
						int len = sprintf(szCmd, "<control name=\"start\" tmstart=\"%d\" tmend=\"%d\" />", 0, mTotalLen);
						SendCmd(MODULE_MSG_CONTROL_PLAY, 0, szCmd,len + 1);

						#ifdef __ANDROID__
							FileRecvCallback(mSid.sid, RECV_TELL_TOTAL_LENGTH, mTotalLen);
						#endif
						break;
				}

			    //GLOGE("recv total:%s", mRecvBuffer.buff);

			    mRecvBuffer.reset();
			}
			else if(hasRecvLen>mRecvBuffer.totalLen)
			{
				log_error("hasRecvLen:%d bigger than totalLen:%d maybe memory out.", hasRecvLen, mRecvBuffer.totalLen);
			}
		}
		else if(ret == 0) {

		}

		return ret;
	}

	int TaskFileRecv::writeBuffer() 
	{

		return 0;
	}


