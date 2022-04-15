// MPEG2RTP.h
#ifndef H264_H_
#define H264_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <mutex>

#include "protocol.h"


#define PACKET_BUFFER_END            (unsigned int)0x00000000
#define MAX_RTP_PKT_LENGTH     1360

#define H264                    96


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

typedef struct {
    //byte 0
	unsigned char TYPE:5;
    unsigned char NRI:2;
	unsigned char F:1;    
} MSG_HEADER; /**//* 1 BYTES */

typedef struct {
    //byte 0
	unsigned char TYPE:5;
    unsigned char NRI:2;
	unsigned char F:1;
} NALU_HEADER;

typedef struct {
    //byte 0
    unsigned char TYPE:5;
	unsigned char NRI:2; 
	unsigned char F:1;    
} FU_INDICATOR; /**//* 1 BYTES */

typedef struct {
	char CC:4;
	char X:1;
	char M:1;
	char V:2;
}PACK_TYPE;

typedef struct {
    //byte 0
    unsigned char TYPE:5;
	unsigned char R:1;
	unsigned char E:1;
	unsigned char S:1;    
} FU_HEADER; /**//* 1 BYTES */

typedef struct {
	MSG_HEADER  	head;	//1
	unsigned char	type;	//1
	unsigned short 	pid;	//2  every transmit package id
	unsigned int 	fid;	//4	 every media frame id
	unsigned int    len;	//4  len for every frame
}__attribute__((packed))PACK_HEAD,*LPPACK_HEAD;//按照实际占用字节数进行对齐， 8 byte

typedef struct
{
	int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
	unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
	unsigned max_size;            //! Nal Unit Buffer size
	int forbidden_bit;            //! should be always FALSE
	int nal_reference_idc;        //! NALU_PRIORITY_xxxx
	int nal_unit_type;            //! NALU_TYPE_xxxx
	unsigned char *buf;                    //! contains the first byte followed by the EBSP
	unsigned short lost_packets;  //! true, if packet loss is detected
} NALU_t;


#ifndef  MAX_LEN
#define  MAX_LEN 1300
#endif

#ifndef  MAX_MTU
const int  MAX_MTU  = MAX_LEN+sizeof(PACK_HEAD);
#endif

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

// struct tagFileSendBuffer 
// {
// 	char cmmd[CMD_LEN];
// 	bool bProcCmmd;

// 	int  hasProcLen;
// 	int  totalLen;//1500 is cmd len
// 	int  dataLen;
// 	int  memLen;
// 	char *data;

// 	tagFileSendBuffer() 
// 	{
// 		data 		= NULL;
// 		totalLen 	= 0;
// 		dataLen 	= 0;
// 		memLen		= 0;
// 		hasProcLen 	= 0;
// 		bProcCmmd 	= true;
// 	}

// 	void reset() 
// 	{
// 		//std::lock_guard<std::mutex> lk(mut);
// 		memset(cmmd, 0, CMD_LEN);
// 		if(data) {

// 		}
// 		totalLen 	= 0;
// 		dataLen 	= 0;
// 		memLen		= 0;
// 		hasProcLen 	= 0;
// 		bProcCmmd 	= true;
// 	}

// 	void createMem(int len) 
// 	{
// 		if(data==NULL)
// 		{
// 			data = (char*)malloc(len);
// 			memLen = len;
// 		}
// 	}

// 	void releaseMem() 
// 	{
// 		if(data) 
// 		{
// 			free(data);
// 			data=NULL;
// 		}
// 	}

// 	bool isSendVideo() 
// 	{
// 		return bProcCmmd==false;
// 	}

// 	void setToVideo() 
// 	{
// 		bProcCmmd	= false;
// 		hasProcLen 	= 0;
// 		totalLen 	= dataLen;
// 	}
// };


#endif
