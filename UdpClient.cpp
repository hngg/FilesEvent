

#include "UdpClient.h"
#include "IOUtils.h"
#include "Session.h"

#include "event.h"
#include "basedef.h"
#include "h264.h"

UdpClient :: UdpClient( )
		:mSockId(0)
		,mSession(NULL)
{

}

UdpClient :: ~UdpClient()
{
	if(mSockId > 0) {
		close(mSockId);
		mSockId = 0;
	}

	if(mSession != NULL) {
		delete mSession;
		mSession = NULL;
	}
}

int UdpClient :: connect(const char* destIp, unsigned short destPort) {
	int ret = IOUtils::tcpConnect(destIp, destPort, &mSockId, 0);
	GLOGW("connect ret:%d sockid:%d.\n",ret, mSockId);
	if(ret>=0) {
		mSid.mKey = mSockId;
		IOUtils::setBlock( mSockId, 0 );
		mSession = new Session( mSid, VIDEO_RECV_MSG );
	}
	return ret;
}

int UdpClient :: connect(const char* destIp, unsigned short destPort, const char*filepath) {
	int ret = IOUtils::tcpConnect(destIp, destPort, &mSockId, 0);
	GLOGW("connect ret:%d sockid:%d.\n",ret, mSockId);
	if(ret>=0) {
		mSid.mKey = mSockId;
		IOUtils::setBlock( mSockId, 0 );
		mSession = new Session( mSid, VIDEO_RECV_MSG, (char*)filepath );
	}
	return ret;
}

int UdpClient :: connect(const char* destIp, unsigned short destPort, void *surface) {
	int ret = IOUtils::tcpConnect(destIp, destPort, &mSockId, 0);
	GLOGW("connect ret:%d sockid:%d.\n",ret, mSockId);
	if(ret>=0) {
		mSid.mKey = mSockId;
		IOUtils::setBlock( mSockId, 0 );
		mSession = new Session( mSid, VIDEO_RECV_MSG, surface );
	}
	return ret;
}

int UdpClient :: connect(const char* destIp, unsigned short destPort, const char*filepath, void *surface) {
	int ret = IOUtils::tcpConnect(destIp, destPort, &mSockId, 0);
	GLOGW("connect ret:%d sockid:%d.\n",ret, mSockId);
	if(ret>=0) {
		mSid.mKey = mSockId;
		IOUtils::setBlock( mSockId, 0 );
		mSession = new Session( mSid, VIDEO_RECV_MSG, (char*)filepath, surface );
	}
	return ret;
}

int UdpClient :: connect(const char* destIp, unsigned short destPort, const char*remoteFile, const char*saveFile) {
	int ret = IOUtils::tcpConnect(destIp, destPort, &mSockId, 0);
	GLOGW("connect ret:%d sockid:%d.\n",ret, mSockId);
	if(ret>=0) {
		mSid.mKey = mSockId;
		IOUtils::setBlock( mSockId, 0 );
		mSession = new Session( mSid, FILE_RECV_MSG, (char*)remoteFile, (char*)saveFile );
	}
	return ret;
}

int UdpClient :: disConnect() {
	if(mSockId > 0 && mSession) {

		EventGlobal * eventArg = (EventGlobal*)mSession->getArg();

		SessionManager *manager = eventArg->getSessionManager();

		uint16_t seq;
		if(manager->get(mSockId, &seq)) {
			GLOGW("disconnect begin\n");
			manager->remove(mSockId);
			event_del(mSession->getReadEvent());
			event_del(mSession->getReadEvent());
			event_del(mSession->getTimeEvent());
			close(mSockId);
			mSockId = 0;
			GLOGW("disconnect remove session id:%d\n", mSockId);
		}
		GLOGW("disConnect sockid:%d.\n", mSockId);
	}
	return 0;
}

int UdpClient :: registerEvent(const EventGlobal& evarg) {

	if(mSession!=NULL) {

		evarg.getSessionManager()->get( mSid.mKey, &mSid.mSeq );
		evarg.getSessionManager()->put( mSid.mKey, mSession, &mSid.mSeq );
		mSession->setArg( (void*)&evarg );

		event_set( mSession->getReadEvent(), mSockId, EV_READ|EV_PERSIST, EventActor::onRead, mSession );
		EventActor::addEvent( mSession, EV_READ, mSockId );

		//event_set( mSession->getWriteEvent(), mSockId, EV_WRITE, EventCall::onWrite, mSession );
		//EventCall::addEvent( mSession, EV_WRITE, mSockId );
		GLOGW("tcpclient registerEvent mSession done.\n");
	}
	else
		GLOGE("tcpclient registerEvent mSession is NULL.\n");

	return 0;
}


