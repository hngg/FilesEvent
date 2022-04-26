
#include "ReactorStation.h"
#include "TcpClient.h"
#include "TcpServer.h"

#include <jni.h>
#include "basedef.h"

#include <android/native_window_jni.h>

#define REG_PATH "com/hnggpad/modtrunk/medialib/NativeVideoRtc"

JavaVM*		 g_javaVM		= NULL;
jclass 		 g_mClass		= NULL;

ReactorStation 	mStatiion;
TcpClient	 	*mpClient		= NULL;
TcpServer	 	*mpServer		= NULL;

#define SUCCESS 1
#define FAILED -1

/////////////////////////////////////////////////////Server and real view////////////////////////////////////////////////////////

static int StartNetWork(JNIEnv *env, jobject) 
{
	if(mStatiion.isStartup() == 0) 
	{
		log_warn("______startup begin.");
		mStatiion.startup();
		return SUCCESS;
	}
	else
	{
		log_error("___reactor station is running.");
	}
	
	return FAILED;
}

static int StopNetWork(JNIEnv *env, jobject) 
{
	if(mStatiion.isStartup() == 1) 
	{
		mStatiion.shutdown();
		log_warn("______shutdown done.");
		return SUCCESS;
	}
	else
	{
		log_error("___reactor station is not running.");
	}

	return FAILED;
}

static int StartServer(JNIEnv *env, jobject obj, jstring localip, jint destport)
{
	g_mClass = (jclass)env->NewGlobalRef(obj);

	if(mpServer==NULL)
	{
		jboolean isOk = JNI_FALSE;
		const char*ip = env->GetStringUTFChars(localip, &isOk);

		mpServer = new TcpServer(ip, destport);
		int rest = mpServer->registerEvent(mStatiion.getEventGlobal());

		env->ReleaseStringUTFChars(localip, ip);
		
		return rest;
	}

	return FAILED;
}

static int StopServer(JNIEnv *env, jobject)
{
	if(mpServer!=NULL) 
	{
		mpServer->shutdown();
		delete mpServer;
		mpServer = NULL;

		return SUCCESS;
	}
	
	return FAILED;
}


////////////////////////////////////////////client/////////////////////////////////////////////////////

static int StartFileRecv(JNIEnv *env, jobject obj, jstring destip, jint destport)	//ip port remotefile savefile
{
	int ret = 0;
	g_mClass = (jclass)env->NewGlobalRef(obj);
	if(NULL == mpClient)
	{
		jboolean isOk = JNI_FALSE;
		const char*chaip 	= env->GetStringUTFChars(destip, &isOk);
		mpClient = new TcpClient();
		ret = mpClient->connect(chaip, destport);
		if(ret < 0) 
		{
			delete mpClient;
			mpClient = NULL;
			log_error("connect ip:%s port:%d failed.", chaip, destport);

			return FAILED;
		}

		mpClient->registerEvent(mStatiion.getEventGlobal());

		env->ReleaseStringUTFChars(destip, chaip);

		return SUCCESS;
	}

	return FAILED;
}

static int StopFileRecv(JNIEnv *env, jobject)
{
	if(mpClient)
	{
		mpClient->disConnect();
		delete mpClient;
		mpClient = NULL;

		return SUCCESS;
	}

	return FAILED;
}

static int FetchAndSaveFile(JNIEnv *env, jobject, int key, jstring remoteFile, jstring saveFile)
{
	if(mpClient) 
	{
		jboolean isOk = JNI_FALSE;
		const char* rfile 	= env->GetStringUTFChars(remoteFile, &isOk);
		const char* sfile 	= env->GetStringUTFChars(saveFile, &isOk);
		mpClient->fetchAndSaveFile(0, rfile, sfile);
		env->ReleaseStringUTFChars(remoteFile, rfile);
		env->ReleaseStringUTFChars(saveFile, sfile);

		return SUCCESS;
	}

	return FAILED;
}

static int CancelFetchingFile(JNIEnv *env, jobject, int key)
{
	return SUCCESS;
}

static JNINativeMethod video_method_table[] = {

	{"StartNetWork", "()I", (void*)StartNetWork },
	{"StopNetWork", "()I", (void*)StopNetWork },
	{"StartServer", "(Ljava/lang/String;I)I", (void*)StartServer },
	{"StopServer", "()I", (void*)StopServer },

	{"StartFileRecv", "(Ljava/lang/String;I)I", (void*)StartFileRecv },
	{"FetchAndSaveFile", "(ILjava/lang/String;Ljava/lang/String;)I", (void*)FetchAndSaveFile },
	{"StopFileRecv", "()I", (void*)StopFileRecv },
};

int registerNativeMethods(JNIEnv* env, const char* className, JNINativeMethod* methods, int numMethods)
{
    jclass clazz;

    clazz = env->FindClass(className);
    if (clazz == NULL) 
	{
        log_error("Native registration unable to find class '%s'", className);
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, methods, numMethods) < 0) 
	{
        log_error("RegisterNatives failed for '%s'", className);
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv* env = NULL;
	jint result = -1;

	log_info("Compile: %s %s\n", __DATE__, __TIME__);

	if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) 
	{
		log_error("GetEnv failed!");
		return result;
	}

	g_javaVM = vm;

	registerNativeMethods(env, REG_PATH, video_method_table, NELEM(video_method_table));

	log_info("JNI_OnLoad......");

	return JNI_VERSION_1_4;
}

