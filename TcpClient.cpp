
#include "TcpClient.h"
#include "IOUtils.h"
#include "Session.h"

#include "event.h"
#include "basedef.h"

TcpClient :: TcpClient( )
		:mSockId(0)
		,mSession(NULL)
{

}

TcpClient :: ~TcpClient()
{
	//disConnect(); //call this function would segmentfault in x86
	log_warn("~TcpClient destroy");
}

int TcpClient :: connect(const char* destIp, unsigned short destPort) 
{
	int ret = IOUtils::tcpConnect(destIp, destPort, &mSockId, 0);
	log_warn("connect ret:%d sockid:%d.",ret, mSockId);
	if(ret>=0)
	{
		Sockid_t mSid;
		mSid.sid = mSockId;
		IOUtils::setBlock( mSockId, 0 );
		mSession = new Session( mSid, FILE_RECV_MSG);
	}
	
	return ret;
}

int TcpClient :: disConnect() 
{
	if((mSockId>0) && mSession) 
	{
		EventGlobal* eventArg   = (EventGlobal*)mSession->getGlobal();
		SessionManager* manager = eventArg->getSessionManager();

		log_warn("disconnect begin");
		uint16_t seq;
		if((mSockId > 0) && manager->get(mSockId, &seq)) 
		{
			Session* sessRemoved = manager->getAndRemove(mSockId);
			if(sessRemoved)
			{
				delete sessRemoved;
				sessRemoved = NULL;

				log_warn("disconnect and manager remove session and delete id:%d seq:%d", mSockId, seq);

				//close(mSockId);
				//mSockId = -1;
			}
		}

		log_warn("disconnect end");
	}

	return 0;
}

int TcpClient :: registerEvent(EventGlobal* evglobal) 
{
	if(mSession && evglobal)
	{
		uint16_t seq;
		mSession->setGlobal( evglobal );
		evglobal->getSessionManager()->put( mSockId, mSession, &seq );
		
		mSession->addReadEvent();

		log_warn("tcpclient registerEvent mSession done seq:%d.", seq);
	}
	else
		log_error("tcpclient registerEvent mSession is NULL.");

	return 0;
}

int TcpClient :: fetchAndSaveFile(int key, const char* remoteFile, const char* saveFile)
{
	if(mSession)
		return mSession->fetchFileAndSave(remoteFile, saveFile);
		
	return 0;
}

int TcpClient :: cancelFetchingFile(int key)
{
	return 0;
}


