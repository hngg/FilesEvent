#ifndef __TaskBase_h__
#define __TaskBase_h__



#include <stdint.h>

#ifndef OWN_SOCK_EXIT
#define OWN_SOCK_EXIT	-1528
#endif

class Session;

typedef struct tagSockid {
	uint16_t sid;	//sockid like
	uint16_t seq;	//index like

	enum {
		eTimerKey = 0,
		eTimerSeq = 65535
	};
} Sockid_t;

class TaskBase {

public:
	// TaskBase( Sockid_t sid );
	// TaskBase( Sockid_t &sid, char*filename);
	TaskBase( Session* sess, Sockid_t& sid, char* filename, char* saveFile);
	virtual ~TaskBase();
	virtual int setHeartCount();
	virtual int readBuffer();
	virtual int writeBuffer();

protected:
	Sockid_t mSid;
	int   	 mHeartCount;
};


#endif


