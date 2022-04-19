#ifndef __BASE_DEF_H__
#define __BASE_DEF_H__

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "glog.h"
#include "protocol.h"

#ifdef __cplusplus
	  extern "C"{
#endif



#ifndef false
#define false	0
#endif

#ifndef true
#define true	1
#endif

#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

#ifndef MAX_PACKET_SIZE
#define MAX_PACKET_SIZE	((60* 1024) - 1)
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) 	do{delete(p); p = NULL;}while(0)
#endif

#define SAFE_DELETE_ELEMENT(ptr) if(ptr != NULL){ delete ptr; ptr = NULL;}

#ifndef SAFE_FREE
#define SAFE_FREE(p) 	do{free(p); p = NULL;}while(0)
#endif

#ifndef FILE_BUFFER_LEN
static const int	FILE_MEMORY_LEN  	= 1024*1024;//512k 1m (1024*1024->1048576)
#endif

//////////////////////////////////////////////////////////////////////////////////////////////


#ifndef  CMD_LEN
#define  CMD_LEN 1500
#endif

//recv data struct
struct tagCmdBuffer {
	char cmmd[CMD_LEN];
	bool bProcCmmd;
	int  hasProcLen;
	int  totalLen;
	void reset() {
		bProcCmmd 	= true;
		hasProcLen 	= 0;
		totalLen 	= sizeof(NET_CMD);
		memset(cmmd,0, CMD_LEN);
	}
};

struct tagFileProcBuffer {
	char cmmd[CMD_LEN];
	bool bProcCmmd;

	int  hasProcLen;
	int  totalLen;//1500 is cmd len
	int  dataLen;
	int  memLen;
	char *data;

	tagFileProcBuffer() 
	{
		data 		= NULL;
		totalLen 	= 0;
		dataLen 	= 0;
		hasProcLen 	= 0;
		bProcCmmd 	= true;
	}

	void reset() 
	{
		//std::lock_guard<std::mutex> lk(mut);
		memset(cmmd, 0, CMD_LEN);

		totalLen 	= 0;
		dataLen 	= 0;
		hasProcLen 	= 0;
		bProcCmmd 	= true;
	}

	void createMem(int len) 
	{
		if(data==NULL)
		{
			data = (char*)malloc(len);
			memLen = len;
		}
	}

	int maxMemoryLengh()
	{
		return memLen;
	}

	void releaseMem() 
	{
		if(data)
		{
			free(data);
			data=NULL;
			memLen = 0;
		}
	}

	bool isSendVideo() 
	{
		return bProcCmmd==false;
	}

	void setToVideo() 
	{
		bProcCmmd	= false;
		hasProcLen 	= 0;
		totalLen 	= dataLen;
	}
};


#ifdef __cplusplus
}
#endif



#endif
