/*==========================================================================
 *FILENAME:      net_protocol.h
 *
 *DESCRIPTION:   net_protocol Module
 *
 *AUTHOR:        mingjiawan               
 *
 *DATE:          2011-01-01
 *
 *COMPANY:      	 
===========================================================================*/

#ifndef __BASE_UTILS_H__
#define __BASE_UTILS_H__

#include "glog.h"

#ifdef __cplusplus
extern "C"{
#endif

#ifdef 	__ANDROID__

#include <jni.h>

extern JavaVM*	g_javaVM;
extern jclass   g_mClass;

int FileRecvCallback( int sockId, int command, int fileLen );
#endif //android


int PROTO_GetValueByName(char *pData, char *pName, char * pValue, int *pValueLen);


#ifdef __cplusplus
}
#endif

#endif //__BASE_UTILS_H__


