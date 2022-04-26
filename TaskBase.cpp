#include "TaskBase.h"


	TaskBase::TaskBase(Session* sess, Sockid_t& sid)
			:mSid(sid)
			,mHeartCount(0)
	{

	}

	TaskBase::~TaskBase() {

	}

	int TaskBase::setFetchFile(const char* filepath)
	{
		return 0;
	}

	int TaskBase::setFetchAndSaveFile(const char* fetchfile, const char* savefile)
	{
		return 0;
	}

	int TaskBase::setHeartCount() {
		return 0;
	}

	int TaskBase::readBuffer() {
		return 0;
	}

	int TaskBase::writeBuffer() {
		return 0;
	}
