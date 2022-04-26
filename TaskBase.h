#ifndef __TaskBase_h__
#define __TaskBase_h__


#include <stdint.h>

#ifndef OWN_SOCK_EXIT
#define OWN_SOCK_EXIT	-1528
#endif

enum FILE_PROCESS_STATUS
{
	RECV_TELL_TOTAL_LENGTH = 1,		//file total length
	RECV_TELL_READ_LENGTH,			//current recv length
	RECV_TELL_END,					//file recv end
	RECV_TELL_SAVE_DONE,			//file save done
	RECV_TELL_USE_TIME,				//file recv use time

	SEND_TELL_FILE_END     = 101	//server send current file end
};

class Session;

typedef struct tagSockid 
{
	uint16_t sid;	//sockid like
	uint16_t seq;	//index like

	enum {
		eTimerKey = 0,
		eTimerSeq = 65535
	};
} Sockid_t;

class TaskBase 
{
	public:
		TaskBase( Session* sess, Sockid_t& sid);
		virtual ~TaskBase() = 0;	//pure abstract function
		virtual int setFetchFile(const char* filepath);//set local fetch filepath for server
		virtual int setFetchAndSaveFile(const char* fetchfile, const char* savefile);//set remote filepath and save filepath for client
		virtual int setHeartCount();
		virtual int readBuffer();
		virtual int writeBuffer();

	protected:
		Sockid_t mSid;
		int   	 mHeartCount;
};


#endif


