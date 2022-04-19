LOCAL_PATH := $(call my-dir)

LOCAL_PROJECT_ROOT := $(LOCAL_PATH)#$(subst $(LOCAL_PATH)/,,$(wildcard $(LOCAL_PATH)))


THREAD_PATH  = ./common/gthread
NAREDEC_PATH = ./common/NalBareflow



include $(CLEAR_VARS)

APP_ALLOW_MISSING_DEPS=true

LOCAL_CFLAGS := -D__ANDROID__ -DHAVE_CONFIG_H -D__STDC_CONSTANT_MACROS -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE
LOCAL_CPPFLAGS 	+= -std=c++11
LOCAL_MODULE := FilesEvent

LOCAL_C_INCLUDES += \
				   $(LOCAL_PROJECT_ROOT)/net \
				   $(LOCAL_PROJECT_ROOT)/common \
				   $(LOCAL_PROJECT_ROOT)/$(CODEC_PATH) \
				   $(LOCAL_PROJECT_ROOT)/$(MCNDK_PATH) \
				   $(LOCAL_PROJECT_ROOT)/$(CAMERA_PATH) \
				   $(LOCAL_PROJECT_ROOT)/$(THREAD_PATH) \
				   external/stlport/stlport bionic

LOCAL_SRC_FILES := net/buffer.c \
				net/epoll.c \
				net/epoll_sub.c \
				net/event.c \
				net/evbuffer.c \
				net/signal.c \
				net/log.c \
				common/glog.c \
				BaseUtils.cpp \
				ReactorStation.cpp \
				BufferCache.cpp \
				EventGlobal.cpp \
				IOUtils.cpp \
				Session.cpp \
				TaskBase.cpp \
				TaskFileSend.cpp \
				TaskFileRecv.cpp \
				TcpClient.cpp \
				TcpServer.cpp \
				NativeApi.cpp

#LOCAL_SHARED_LIBRARIES := avformat avcodec avutil swresample
LOCAL_LDLIBS := -llog -landroid

include $(BUILD_SHARED_LIBRARY)

