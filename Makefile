CXXFLAGS += -std=c++11

CC  = gcc
CPP = g++
CFLAGS 		= -Wall -DHAVE_CONFIG_H -g -fPIC #-DHAVE_SYS_TIME_H
CXXFLAGS 	+= -Dlinux -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE

THREAD_PATH  = ./common/gthread
NAREDEC_PATH = ./common/NalBareflow
CODEC_PATH	 = ./Codec
#CAMERA_PATH	 = ../Camera

COMPLIE_INCL = -I./net -I./common -I$(THREAD_PATH) -I$(CODEC_PATH)# -I$(CAMERA_PATH)
#COMPLIE_LIB  = -L$(FFMPEG_PATH)/lib -lavformat -lavcodec -lavdevice -lavfilter -lswresample -lswscale -lavutil
CFLAGS   += -I./ -I./net -I./common
CXXFLAGS += $(COMPLIE_INCL)
LDFLAGS  += $(COMPLIE_LIB) -lpthread

TARGET = UdpEventServer UdpEventClient TcpEventServer TcpEventClient librealserver.a move

#--------------------------------------------------------------------

all: $(TARGET)

OBJECTS = net/buffer.o \
			net/epoll.o \
			net/epoll_sub.o \
			net/event.o \
			net/evbuffer.o \
			net/signal.o \
			net/log.o \
			net/net_protocol.o \
			BufferCache.o \
			DataUtils.o \
			IOUtils.o \
			Session.o \
			EventCall.o \
			ActorStation.o \
			TaskBase.o \
			TaskFileSend.o \
			TaskFileRecv.o \
			$(THREAD_PATH)/gthreadpool.o \
			$(NAREDEC_PATH)/NALDecoder.o

librealserver.a: $(OBJECTS)
	 $(AR) rs $@ $?

TcpEventServer=$(OBJECTS) TcpEventServer.o TcpServer.o
TcpEventClient=$(OBJECTS) TcpEventClient.o TcpClient.o
 
UdpEventServer=$(OBJECTS) UdpEventServer.o UdpServer.o
UdpEventClient=$(OBJECTS) UdpEventClient.o UdpClient.o

TcpEventServer:$(TcpEventServer) 
	 $(CPP) -o TcpEventServer $(TcpEventServer) $(LDFLAGS)

TcpEventClient:$(TcpEventClient) 
	 $(CPP) -o TcpEventClient $(TcpEventClient) $(LDFLAGS)

UdpEventServer:$(UdpEventServer) 
	 $(CPP) -o UdpEventServer $(UdpEventServer) $(LDFLAGS)

UdpEventClient:$(UdpEventClient) 
	 $(CPP) -o UdpEventClient $(UdpEventClient) $(LDFLAGS)

move:
	$(shell mv *.o obj)

.PHONY:clean
clean:
	rm obj/*.o net/*.o $(NAREDEC_PATH)/*.o $(THREAD_PATH)/*.o

	
