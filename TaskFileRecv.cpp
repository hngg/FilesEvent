
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "TaskFileRecv.h"
#include "BufferCache.h"
#include "EventCall.h"

#include "event.h"
#include "h264.h"

//#define TAG "TaskFileRecv"
#include "basedef.h"


enum FILE_RECV_STATUS{
	RECV_TELL_TOTAL_LENGTH = 1,	//file total length
	RECV_TELL_READ_LENGTH,		//current recv length
	RECV_TELL_END,				//file recv end
	RECV_TELL_SAVE_DONE			//file save done
};


#ifdef 	__ANDROID__
#define	FILE_PATH	"/sdcard/w.h264"
#else
#define	FILE_PATH	"recv.mp4"
#endif

#ifdef 	__ANDROID__

#include <jni.h>

extern JavaVM*	g_javaVM;
extern jclass   g_mClass;

int FileRecvCallback( int sockId, int command, int fileLen ) {
	jint result = -1;
	JNIEnv*		menv;
	jobject		mobj;
	jclass		tmpClass;

	if(NULL == g_mClass) {
		GLOGE("g_mClass is null.");
		return result;
	}

	if(g_javaVM)
	{
		result = g_javaVM->AttachCurrentThread( &menv, NULL);
		if(NULL == menv) {
			GLOGE("function: %s, line: %d, GetEnv failed!", __FUNCTION__, __LINE__);
			return g_javaVM->DetachCurrentThread();
		}
	}
	else
	{
		GLOGE("function: %s, line: %d, JavaVM is null!", __FUNCTION__, __LINE__);
		return result;
	}

	if(NULL != g_mClass)
	{
		tmpClass = menv->GetObjectClass(g_mClass);
		if(tmpClass == NULL) {
			GLOGE("function: %s, line: %d, find class error", __FUNCTION__, __LINE__);
			return g_javaVM->DetachCurrentThread();
		}
		mobj = menv->AllocObject(tmpClass);
		if(NULL == mobj) {
			GLOGE("function: %s, line: %d, find jobj error!", __FUNCTION__, __LINE__);
			return g_javaVM->DetachCurrentThread();
		}

		jmethodID methodID_func = menv->GetMethodID(tmpClass, "onFileState", "(III)V");// sockid command length
		if(methodID_func == NULL) {
			GLOGE("function: %s, line: %d,find method error!", __FUNCTION__, __LINE__);
			return g_javaVM->DetachCurrentThread();
		}
		else {
			menv->CallVoidMethod(mobj, methodID_func, sockId, command, fileLen);
		}
	}

	return g_javaVM->DetachCurrentThread();
}

#endif

	TaskFileRecv::TaskFileRecv( Session*sess, Sid_t &sid )
				:mPackHeadLen(sizeof(NET_CMD))
				,TaskBase(sid)
				,mSess(sess)
				,mRecvDataLen(0)
				,mRecvHeadLen(0)
				,mTotalLen(0)
	{
		//mCmdBuffer.reset();
		mRecvBuffer.reset();
		mRecvBuffer.createMem(FILE_MEMORY_LEN);

		mwFile = fopen(FILE_PATH, "w");

		char lpData[2048];
		int nLength = sprintf(lpData, "<get path=\"%s\"/>", "/h264/tmp.mp4");


		if(SendCmd(MODULE_MSG_LOGIN, 0, lpData, nLength)<0)
			GLOGE("send CMD err:%s", lpData);

	}


	TaskFileRecv::TaskFileRecv( Session*sess, Sid_t &sid, char*remoteFile )
				:mPackHeadLen(sizeof(NET_CMD))
				,TaskBase(sid)
				,mSess(sess)
				,mRecvDataLen(0)
				,mRecvHeadLen(0)
				,mTotalLen(0)
	{
		//mCmdBuffer.reset();
		mRecvBuffer.reset();
		mRecvBuffer.createMem(FILE_MEMORY_LEN);

		mwFile = fopen(FILE_PATH, "w");

		char lpData[2048];
		int nLength = sprintf(lpData, "<get path=\"%s\"/>", remoteFile);


		if(SendCmd(MODULE_MSG_LOGIN, 0, lpData, nLength)<0)
			GLOGE("send CMD err:%s", lpData);

	}

	TaskFileRecv::TaskFileRecv( Session*sess, Sid_t &sid, char*remoteFile, char*saveFile )
				:mPackHeadLen(sizeof(NET_CMD))
				,TaskBase(sid)
				,mSess(sess)
				,mRecvDataLen(0)
				,mRecvHeadLen(0)
				,mTotalLen(0)
	{
		//mCmdBuffer.reset();
		mRecvBuffer.reset();
		mRecvBuffer.createMem(FILE_MEMORY_LEN+sizeof(NET_CMD));

		mwFile = fopen(saveFile, "w");

		char lpData[2048];
		int nLength = sprintf(lpData, "<get path=\"%s\"/>", remoteFile);


		if(SendCmd(MODULE_MSG_LOGIN, 0, lpData, nLength)<0)
			GLOGE("send CMD err:%s", lpData);
	}

	TaskFileRecv::~TaskFileRecv() {

		GLOGW("file seek:%ld", ftell(mwFile));

		if(mwFile != NULL)
			fclose(mwFile);

		mRecvBuffer.releaseMem();
	}

	int TaskFileRecv::sendEx(void*data, int len) {
		int leftLen = len, iRet = 0;

		struct timeval timeout;
		int sockId = mSid.mKey;
		do {
			iRet = send(sockId, (char*)data+len-leftLen, leftLen, 0);

			if(iRet<0) {
				//GLOGE("send data errno:%d ret:%d.", errno, iRet);
				switch(errno) {
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
			GLOGE("send cmd err len = %d ", nLength);
			return iRet;
		}

		GLOGW("send head len:%d ", (int)sizeof(nc));
		if (nLength == 0)
		{
			return 0;
		}
		if ((iRet = sendEx(lpData, nLength))<0)
		{
			GLOGE("send lpdata err len = %d ",nLength);
			return iRet;
		}
		GLOGW("send data len:%d lpData:%s", nLength, (char*)lpData);
		return iRet;
	}

	//read event callback function
	int TaskFileRecv::readBuffer() {
		int ret = -1;
		int &hasRecvLen = mRecvBuffer.hasProcLen;
		if(mRecvBuffer.bProcCmmd) {
			ret = recv(mSid.mKey, mRecvBuffer.data+hasRecvLen, mPackHeadLen-hasRecvLen, 0);

			if(ret>0) {
				hasRecvLen+=ret;
				if(hasRecvLen==mPackHeadLen) {
					LPNET_CMD head = (LPNET_CMD)mRecvBuffer.data;
					mRecvBuffer.totalLen  = head->dwLength;//package data length
					mRecvBuffer.bProcCmmd = false;
					hasRecvLen = 0;

					GLOGE("playback flag:%08x cmd:%d read event need recv totalLen:%d ret:%d", head->dwFlag, head->dwCmd, mRecvBuffer.totalLen, ret);

					if(head->dwLength>0) {
						ret = recvPackData();
					}
					else
					{
						switch(head->dwCmd) {
							case MODULE_MSG_DATAEND:	//69
								#ifdef 	__ANDROID__
									FileRecvCallback(mSid.mKey, RECV_TELL_END, 0);
								#endif
								GLOGW("MODULE_MSG_DATAEND");
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
					FileRecvCallback(mSid.mKey, RECV_TELL_SAVE_DONE, 0);
				#endif
			}
		}//bProcCmmd
		else{
			ret = recvPackData();
		}

		//GLOGE("--------1-----------recvPackData ret:%d\n",ret);
		//EventCall::addEvent( mSess, EV_READ, -1 );
		return ret;
	}

	//receive file data,head length is 16byte,other is data
	int TaskFileRecv::recvPackData() {
		int &hasRecvLen = mRecvBuffer.hasProcLen;
		int ret = recv(mSid.mKey, mRecvBuffer.data+mPackHeadLen+hasRecvLen, mRecvBuffer.totalLen-hasRecvLen, 0);
		GLOGE("------recvPackData ret:%d hasRecvLne:%d totalLen:%d", ret, hasRecvLen, mRecvBuffer.totalLen);
		if(ret>0) {
			hasRecvLen += ret;
			if(hasRecvLen==mRecvBuffer.totalLen) {

				int lValueLen;
			    char acValue[256] = {0};	//new char[256];
			    memset(acValue, 0, 256);
				LPNET_CMD pCmdbuf = (LPNET_CMD)mRecvBuffer.data;
				LPLOGIN_RET lpRet;
				LPFILE_GET  lpFrame;
				switch(pCmdbuf->dwCmd) {

					case MODULE_MSG_CONTROL_PLAY: //68
						break;

					case MODULE_MSG_VIDEO:	//25
						lpFrame 		= (LPFILE_GET)(pCmdbuf->lpData);
						fwrite(lpFrame->lpData , 1 , lpFrame->nLength , mwFile);
						fflush(mwFile);
						GLOGW("frame len:%d", lpFrame->nLength);

						#ifdef 	__ANDROID__
							FileRecvCallback(mSid.mKey, RECV_TELL_READ_LENGTH, lpFrame->nLength);
						#endif
						break;

					case MODULE_MSG_LOGINRET: //2
						LPLOGIN_RET lpRet = (LPLOGIN_RET)(mRecvBuffer.data+mPackHeadLen);
						LPFILE_INFO lpInfo= (LPFILE_INFO)lpRet->lpData;
						mTotalLen = lpInfo->tmEnd;

						GLOGW("The file to recv TotalLen:%d", mTotalLen);

						char szCmd[100];
						int len = sprintf(szCmd, "<control name=\"start\" tmstart=\"%d\" tmend=\"%d\" />", 0, mTotalLen);
						SendCmd(MODULE_MSG_CONTROL_PLAY, 0, szCmd,len + 1);

						#ifdef 	__ANDROID__
							FileRecvCallback(mSid.mKey, RECV_TELL_TOTAL_LENGTH, mTotalLen);
						#endif
						break;
				}

			    //GLOGE("recv total:%s", mRecvBuffer.buff);

			    mRecvBuffer.reset();
			}
			else if(hasRecvLen>mRecvBuffer.totalLen)
			{
				GLOGE("hasRecvLen:%d bigger than totalLen:%d maybe memory out.", hasRecvLen, mRecvBuffer.totalLen);
			}
		}
		else if(ret == 0) {

		}

		return ret;
	}

	int TaskFileRecv::writeBuffer() {

		return 0;
	}


