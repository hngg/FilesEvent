

#include "TcpClient.h"
#include "IOUtils.h"
#include "Session.h"

#include "event.h"
#include "basedef.h"
#include "h264.h"

TcpClient :: TcpClient( )
		:mSockId(0)
		,mSession(NULL)
{

}

TcpClient :: ~TcpClient()
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

int TcpClient :: connect(const char* destIp, unsigned short destPort) {
	int ret = IOUtils::tcpConnect(destIp, destPort, &mSockId, 0);
	log_warn("connect ret:%d sockid:%d.\n",ret, mSockId);
	if(ret>=0) {
		mSid.mKey = mSockId;
		IOUtils::setBlock( mSockId, 0 );
		mSession = new Session( mSid, VIDEO_RECV_MSG );
	}
	return ret;
}

int TcpClient :: connect(const char* destIp, unsigned short destPort, const char*filepath) {
	int ret = IOUtils::tcpConnect(destIp, destPort, &mSockId, 0);
	log_warn("connect ret:%d sockid:%d.",ret, mSockId);
	if(ret>=0) {
		mSid.mKey = mSockId;
		IOUtils::setBlock( mSockId, 0 );
		mSession = new Session( mSid, VIDEO_RECV_MSG, (char*)filepath );
	}
	return ret;
}

int TcpClient :: connect(const char* destIp, unsigned short destPort, void *surface) {
	int ret = IOUtils::tcpConnect(destIp, destPort, &mSockId, 0);
	log_warn("connect ret:%d sockid:%d.",ret, mSockId);
	if(ret>=0) {
		mSid.mKey = mSockId;
		IOUtils::setBlock( mSockId, 0 );
		mSession = new Session( mSid, VIDEO_RECV_MSG, surface );
	}
	return ret;
}

int TcpClient :: connect(const char* destIp, unsigned short destPort, const char*filepath, void *surface) {
	int ret = IOUtils::tcpConnect(destIp, destPort, &mSockId, 0);
	log_warn("connect ret:%d sockid:%d.",ret, mSockId);
	if(ret>=0) {
		mSid.mKey = mSockId;
		IOUtils::setBlock( mSockId, 0 );
		mSession = new Session( mSid, VIDEO_RECV_MSG, (char*)filepath, surface );
	}
	return ret;
}

int TcpClient :: connect(const char* destIp, unsigned short destPort, const char*remoteFile, const char*saveFile) {
	int ret = IOUtils::tcpConnect(destIp, destPort, &mSockId, 0);
	log_warn("connect ret:%d sockid:%d.",ret, mSockId);
	if(ret>=0) {
		mSid.mKey = mSockId;
		IOUtils::setBlock( mSockId, 0 );
		mSession = new Session( mSid, FILE_RECV_MSG, (char*)remoteFile, (char*)saveFile );
	}
	return ret;
}

int TcpClient :: disConnect() {
	if(mSockId > 0 && mSession) {

		EventGlobal * eventArg = (EventGlobal*)mSession->getArg();
		SessionManager *manager = eventArg->getSessionManager();

		log_warn("disconnect begin");
		uint16_t seq;
		if(manager->get(mSockId, &seq)) 
		{
			
			manager->remove(mSockId);
			event_del(mSession->getReadEvent());
			event_del(mSession->getReadEvent());
			event_del(mSession->getTimeEvent());
			close(mSockId);
			
			log_warn("disconnect remove session id:%d", mSockId);

			mSockId = -1;
		}
	}
	return 0;
}

int TcpClient :: registerEvent(const EventGlobal& evarg) {

	if(mSession!=NULL) {

		evarg.getSessionManager()->get( mSid.mKey, &mSid.mSeq );
		evarg.getSessionManager()->put( mSid.mKey, mSession, &mSid.mSeq );
		mSession->setArg( (void*)&evarg );

		event_set( mSession->getReadEvent(), mSockId, EV_READ|EV_PERSIST, EventActor::onRead, mSession );
		EventActor::addEvent( mSession, EV_READ, mSockId );

		//event_set( mSession->getWriteEvent(), mSockId, EV_WRITE, EventCall::onWrite, mSession );
		//EventCall::addEvent( mSession, EV_WRITE, mSockId );
		log_warn("tcpclient registerEvent mSession done.");
	}
	else
		log_error("tcpclient registerEvent mSession is NULL.");

	return 0;
}


