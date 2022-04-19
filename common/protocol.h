/*
 * FileName:       gs_tlib.h
 * Author:         mingjiawan  Version: 2.0  Date: 2011-10-22
 */

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__


#include <pthread.h>


#ifdef __cplusplus
	  extern "C"{
#endif

#define FRAME_VIDEO_I	1
#define FRAME_VIDEO_P	2
#define FRAME_AUDIO		3
#define FRAME_TALKAUDIO			4

#define AUDCODEC_G711_ULAW 1
#define AUDCODEC_G711_ALAW 2
#define AUDCODEC_PCM 3
#define AUDCODEC_AAC 4

#define PACKED		__attribute__((packed, aligned(1))) //
#define PACKED4		__attribute__((packed, aligned(4))) //


//===============================================================
#define NET_FLAG	0xfefdfcfb

//type
typedef enum MSG_TYPE {
	FILE_RECV_MSG = 1,
	FILE_RECV_STREAM,
	FILE_SEND_MSG,
	FILE_SEND_STREAM,

	VIDEO_RECV_MSG = 101,
	VIDEO_RECV_STREAM,
	VIDEO_SEND_MSG,
	VIDEO_SEND_STREAM,
	VIDEO_REAL_MSG,
	VIDEO_REAL_STREAM,
}MSG_TYPE_t;

//cmds
enum MODULE_MSG_ID{
	MODULE_MSG_PING,
	MODULE_MSG_LOGIN ,
	MODULE_MSG_LOGINRET,		//->2

	MODULE_MSG_FILES,	   		//->3
	MODULE_MSG_VIDEO,	   		//->4

	MODULE_MSG_STARTSERVICE,
	MODULE_MSG_STOPSVC,			

	MODULE_MSG_CONTROL_PLAY,	//->7
	MODULE_MSG_DATAEND,			//->8
	MODULE_MSG_SECTION_END,		//->9
};


//err code for client.
 #define ERR_NOERROR		0


//user in session
////#pragma   pack(1)
typedef struct tagNET_HEAD
{
	unsigned int dwFlag;
	unsigned int dwCmd;
	unsigned int dwIndex;
	unsigned int dwLength;
}NET_HEAD,*LPNET_HEAD;
////#pragma   pack()

typedef struct tagNET_CMD
{
	unsigned int dwFlag;
	unsigned int dwCmd;
	unsigned int dwIndex;
	unsigned int dwLength;
	char  lpData[];
}NET_CMD,*LPNET_CMD;

typedef struct tagLOGIN_RET
{
	unsigned int 	lRet;
	int 			nLength;
	char 			lpData[1024];
}LOGIN_RET,*LPLOGIN_RET;

typedef struct tagFILE_GET
{
	unsigned int 	dwPos;
	int 			nLength;
	char 			lpData[];
}FILE_GET,*LPFILE_GET;

#pragma   pack(1)
typedef struct tagFILE_INFO
{
	unsigned int tmStart;
	unsigned int tmEnd;
}FILE_INFO,*LPFILE_INFO;//
#pragma   pack(0)

#ifdef __cplusplus
}
#endif

#endif

