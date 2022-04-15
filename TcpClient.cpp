
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

// int TcpClient :: connect(const char* destIp, unsigned short destPort) {
// 	int ret = IOUtils::tcpConnect(destIp, destPort, &mSockId, 0);
// 	log_warn("connect ret:%d sockid:%d.\n",ret, mSockId);
// 	if(ret>=0) {
// 		mSid.mKey = mSockId;
// 		IOUtils::setBlock( mSockId, 0 );
// 		mSession = new Session( mSid, VIDEO_RECV_MSG );
// 	}
// 	return ret;
// }

// int TcpClient :: connect(const char* destIp, unsigned short destPort, const char*filepath) {
// 	int ret = IOUtils::tcpConnect(destIp, destPort, &mSockId, 0);
// 	log_warn("connect ret:%d sockid:%d.",ret, mSockId);
// 	if(ret>=0) {
// 		mSid.mKey = mSockId;
// 		IOUtils::setBlock( mSockId, 0 );
// 		mSession = new Session( mSid, VIDEO_RECV_MSG, (char*)filepath );
// 	}
// 	return ret;
// }

// int TcpClient :: connect(const char* destIp, unsigned short destPort, void *surface) {
// 	int ret = IOUtils::tcpConnect(destIp, destPort, &mSockId, 0);
// 	log_warn("connect ret:%d sockid:%d.",ret, mSockId);
// 	if(ret>=0) {
// 		mSid.mKey = mSockId;
// 		IOUtils::setBlock( mSockId, 0 );
// 		mSession = new Session( mSid, VIDEO_RECV_MSG, surface );
// 	}
// 	return ret;
// }

// int TcpClient :: connect(const char* destIp, unsigned short destPort, const char*filepath, void *surface) {
// 	int ret = IOUtils::tcpConnect(destIp, destPort, &mSockId, 0);
// 	log_warn("connect ret:%d sockid:%d.",ret, mSockId);
// 	if(ret>=0) {
// 		mSid.mKey = mSockId;
// 		IOUtils::setBlock( mSockId, 0 );
// 		mSession = new Session( mSid, VIDEO_RECV_MSG, (char*)filepath, surface );
// 	}
// 	return ret;
// }

int TcpClient :: connect(const char* destIp, unsigned short destPort, const char*remoteFile, const char*saveFile) 
{
	int ret = IOUtils::tcpConnect(destIp, destPort, &mSockId, 0);
	log_warn("connect ret:%d sockid:%d.",ret, mSockId);
	if(ret>=0)
	{
		Sockid_t mSid;
		mSid.sid = mSockId;
		IOUtils::setBlock( mSockId, 0 );
		mSession = new Session( mSid, FILE_RECV_MSG, (char*)remoteFile, (char*)saveFile );
	}
	return ret;
}

int TcpClient :: disConnect() 
{
	if(mSockId > 0 && mSession) 
	{
		EventGlobal * eventArg  = (EventGlobal*)mSession->getArg();
		SessionManager *manager = eventArg->getSessionManager();

		log_warn("disconnect begin");
		uint16_t seq;
		if((mSockId > 0)&&manager->get(mSockId, &seq)) 
		{
			log_warn("----------------------------------1");
			Session* sessRemoved = manager->remove(mSockId);
			log_warn("----------------------------------2");
			if(sessRemoved)
			{
				log_warn("----------------------------------3");
				event_del(mSession->getReadEvent());
				event_del(mSession->getTimeEvent());
				close(mSockId);
				
				log_warn("disconnect and manager remove session id:%d seq:%d", mSockId, seq);

				mSockId = -1;
			}
		}
		log_warn("----------------------------------4");
		if(mSession)
		{
			delete mSession;
			mSession = NULL;
		}
		log_warn("disconnect end");
	}
	return 0;
}

int TcpClient :: registerEvent(const EventGlobal& evarg) 
{
	if(mSession!=NULL) 
	{
		uint16_t seq;
		mSession->setArg( (void*)&evarg );
		evarg.getSessionManager()->put( mSockId, mSession, &seq );
		
		mSession->addReadEvent(EventActor::onRead);
		//event_set( mSession->getReadEvent(), mSockId, EV_READ|EV_PERSIST, EventActor::onRead, mSession );
		//EventActor::addEvent( mSession, EV_READ, mSockId );

		//event_set( mSession->getWriteEvent(), mSockId, EV_WRITE, EventCall::onWrite, mSession );
		//EventCall::addEvent( mSession, EV_WRITE, mSockId );
		log_warn("tcpclient registerEvent mSession done.");
	}
	else
		log_error("tcpclient registerEvent mSession is NULL.");

	return 0;
}


