
#include "ActorStation.hpp"
#include "TcpClient.hpp"
#include "TcpServer.hpp"
// #include "RealCameraCodec.h"
// #include "UpperNdkEncodec.h"

#include <jni.h>
#include "basedef.h"

#include <android/native_window_jni.h>

#define REG_PATH "com/hnggpad/modtrunk/medialib/NativeVideoRtc"

JavaVM*		 g_javaVM		= NULL;
jclass 		 g_mClass		= NULL;

ActorStation mStatiion;
TcpClient	 *mpClient		= NULL;
TcpServer	 *mpServer		= NULL;
// RealCameraCodec *gRealCam   = NULL;
// UpperNdkEncodec *gUpcamEnc	= NULL;

/////////////////////////////////////////////////////Server and real view////////////////////////////////////////////////////////

static jboolean StartNetWork(JNIEnv *env, jobject) {
	if(mStatiion.isRunning() == 0) {
		mStatiion.startup();
		return true;
	}
	return false;
}

static jboolean StopNetWork(JNIEnv *env, jobject) {
	if(mStatiion.isRunning() == 1) {
		mStatiion.shutdown();
		return true;
	}
	return false;
}

static jboolean StartServer(JNIEnv *env, jobject obj, jstring localip, jint destport)
{
	if(mpServer==NULL) {
		mpServer = new TcpServer("", destport);
		mpServer->setMaxThreads( 10 );
		mpServer->registerEvent(mStatiion.getEventArg());
	}

	g_mClass = (jclass)env->NewGlobalRef(obj);

	return true;
}

static jboolean StopServer(JNIEnv *env, jobject)
{
	if(mpServer!=NULL) {
		mpServer->shutdown();
		delete mpServer;
		mpServer = NULL;
	}
	return true;
}


////////////////////////////////////////////client/////////////////////////////////////////////////////

static jboolean StartFileRecv(JNIEnv *env, jobject obj, jstring destip, jint destport, jstring remoteFile, jstring saveFile)	//ip port remotefile savefile
{
	int ret = 0;
	g_mClass = (jclass)env->NewGlobalRef(obj);
	if(mpClient==NULL) {

		jboolean isOk = JNI_FALSE;
		const char*rfile = env->GetStringUTFChars(remoteFile, &isOk);
		const char*sfile = env->GetStringUTFChars(saveFile, &isOk);
		const char *ip = env->GetStringUTFChars(destip, &isOk);
		mpClient = new TcpClient();
		ret = mpClient->connect( ip, destport, rfile, sfile );

		if(ret < 0) {
			delete mpClient;
			mpClient = NULL;
			return false;
		}

		mpClient->registerEvent(mStatiion.getEventArg());

		env->ReleaseStringUTFChars(remoteFile, rfile);
		env->ReleaseStringUTFChars(saveFile, sfile);
		env->ReleaseStringUTFChars(destip, ip);

		return true;
	}
	return false;
}

static jboolean StopFileRecv(JNIEnv *env, jobject)
{
	if(mpClient!=NULL){
		mpClient->disConnect();
		delete mpClient;
		mpClient = NULL;
	}
	return true;
}

static JNINativeMethod video_method_table[] = {

		{"StartNetWork", "()Z", (void*)StartNetWork },
		{"StopNetWork", "()Z", (void*)StopNetWork },
		{"StartServer", "(Ljava/lang/String;I)Z", (void*)StartServer },
		{"StopServer", "()Z", (void*)StopServer },

		{"StartFileRecv", "(Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;)Z", (void*)StartFileRecv },
		{"StopFileRecv", "()Z", (void*)StopFileRecv },
};

int registerNativeMethods(JNIEnv* env, const char* className, JNINativeMethod* methods, int numMethods)
{
    jclass clazz;

    clazz = env->FindClass(className);
    if (clazz == NULL) {
        GLOGE("Native registration unable to find class '%s'", className);
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, methods, numMethods) < 0) {
        GLOGE("RegisterNatives failed for '%s'", className);
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv* env = NULL;
	jint result = -1;

	GLOGI("Compile: %s %s\n", __DATE__, __TIME__);

	if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
		GLOGE("GetEnv failed!");
		return result;
	}

	g_javaVM = vm;

	registerNativeMethods(env, REG_PATH, video_method_table, NELEM(video_method_table));

	GLOGW("JNI_OnLoad......");

	return JNI_VERSION_1_4;
}

