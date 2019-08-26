/*
 * CATcpClient.cpp
 *
 *  Created on: May 4, 2017
 *      Author: joe
 */

#include "CATcpClient.h"

#include "CMessageHandler.h"
#include <time.h>
#include "LogHandler.h"
#include "event.h"
#include "utility.h"

using namespace std;

#define IDLE_TIMER			469107

static int CATCP_CLIENT_FILITER = 9000;

static int getCATcpClientFilterNum()
{
	return ++CATCP_CLIENT_FILITER;
}

void *threadCATcpClientMessageReceive(void *argv)
{
	CATcpClient* ss = reinterpret_cast<CATcpClient*>(argv);
	ss->runMessageReceive();
	return 0;
}

void *aClientthreadTcpReceive(void *argv)
{
	CATcpClient* ss = reinterpret_cast<CATcpClient*>(argv);
	ss->runTcpReceive();
	return 0;
}

void CATcpClient::runTcpReceive()
{
	int result;
	int nSocketFD;

	nSocketFD = getSocketfd();
	if (0 >= nSocketFD)
	{
		_log("[CATcpClient] %s runTcpReceive Fail, Invalid Socket FD", strTaskName.c_str());
		sendMessage(CATCP_MSQ_EVENT_FILTER, EVENT_COMMAND_THREAD_EXIT, getThreadID(), 0, 0);
		threadExit();
		return;
	}

	sendMessage(CATCP_MSQ_EVENT_FILTER, EVENT_COMMAND_SOCKET_CONNECT, nSocketFD, 0, 0);

	while (1)
	{
		if (0 >= onTcpReceive(nSocketFD))
		{
			break;
		}
		sendMessage(CATCP_MSQ_EVENT_FILTER, EVENT_COMMAND_SOCKET_TCP_CONNECT_ALIVE, nSocketFD, 0, 0);
	}

	sendMessage(CATCP_MSQ_EVENT_FILTER, EVENT_COMMAND_SOCKET_DISCONNECT, nSocketFD, 0, 0);
	threadExit();
}

int CATcpClient::onTcpReceive(unsigned long int nSocketFD)
{
	int result;
	char pBuf[MAX_SOCKET_READ];
	void* pvBuf = pBuf;
	memset(pBuf, 0, sizeof(pBuf));
	result = socketrecv(nSocketFD, &pvBuf, 0);
	_log("[CATcpClient] %s onTcpReceive Result: %d Socket[%d]", strTaskName.c_str(), result, nSocketFD);

	if (0 < result)
	{
		sendMessage(CATCP_MSQ_EVENT_FILTER, EVENT_COMMAND_SOCKET_CLIENT_RECEIVE, nSocketFD, result, pBuf);
	}
	return result;
}

void CATcpClient::runMessageReceive()
{
	munRunThreadId = getThreadID();
	run(CATCP_MSQ_EVENT_FILTER, strTaskName.c_str());
	threadExit();
	threadJoin(getThreadID());
	_log("[CATcpClient] %s runMessageReceive Stop, Thread join", strTaskName.c_str());
}

int CATcpClient::connect(const char* cszAddr, short nPort, int nMsqKey)
{
	return connectWithCallback(cszAddr, nPort, nMsqKey, NULL);
}

int CATcpClient::connectWithCallback(const char* cszAddr, short nPort, int nMsqKey,
	void(*onReceiverThreadsCreated)(CATcpClient *caller, pthread_t msgRecvTid, pthread_t pktRecvTid))
{
	int nMsgId = -1;
	int nSocketFD;
	IDLE_TIMEOUT = 10; //second
	mnExtMsqKey = FALSE;
	munRunThreadId = 0;

	CATCP_MSQ_EVENT_FILTER = getCATcpClientFilterNum();

	strTaskName = taskName();

	if (-1 != nMsqKey)
	{
		mnMsqKey = nMsqKey;
		mnExtMsqKey = TRUE;
	}
	else
	{
		mnMsqKey = clock();
	}
	if (-1 == mnMsqKey)
	{
		mnMsqKey = 20170503;
	}

	nMsgId = initMessage(mnMsqKey, strTaskName.c_str());

	if (-1 == nMsgId)
	{
		_log("[CATcpClient] %s Init Message Queue Fail", strTaskName.c_str());
		return -1;
	}

	if (-1 == setInetSocket(cszAddr, nPort))
	{
		_log("[CATcpClient] %s Set INET socket address & port fail", strTaskName.c_str());
		return -1;
	}

	if (-1 != createSocket(AF_INET, SOCK_STREAM))
	{

		if (-1 == connectServer())
		{
			socketClose();
			_log("[CATcpClient] %s Set INET socket address & port fail", strTaskName.c_str());
			return -1;
		}

		if (0 != munRunThreadId)
		{
			threadCancel(munRunThreadId);
			threadJoin(munRunThreadId);
			munRunThreadId = 0;
		}

		pthread_t msgRecvTid = createThread(threadCATcpClientMessageReceive, this);
		pthread_t pktRecvTid = createThread(aClientthreadTcpReceive, this);
		_log("[CATcpClient] %s Socket connect success, FD: %d", strTaskName.c_str(), getSocketfd());

		if (NULL != onReceiverThreadsCreated)
		{
			onReceiverThreadsCreated(this, msgRecvTid, pktRecvTid);
		}

		return getSocketfd();
	}

	return -1;
}

void CATcpClient::stop()
{

	socketClose();
	/**
	 * Close Message queue run thread
	 */

//	_log("[CATcpClient] Close Message Queue START");
	if (0 < munRunThreadId)
	{
		//	_log("[CATcpClient] munRunThreadId > 0");
		threadCancel(munRunThreadId);
		threadJoin(munRunThreadId);
		munRunThreadId = 0;

		if (mnExtMsqKey == FALSE)
		{
			//	_log("[CATcpClient] closeMsg");
			CMessageHandler::closeMsg(CMessageHandler::registerMsq(mnMsqKey));
		}
	}
	else
	{
		//_log("[CATcpClient] munRunThreadId < 0");
	}
	//_log("[CATcpClient] Close Message Queue END");

	//_log("[CATcpClient] Close all Client Socket START");

	//threadCancel(getThreadID());
	//_log("[CATcpClient] Close all Client threadCancel END");
	//threadJoin(getThreadID());

	//_log("[CATcpClient] Close all Client Socket END");

}

//active close server
void CATcpClient::closeServer()
{

	sendMessage(CATCP_MSQ_EVENT_FILTER, EVENT_COMMAND_SOCKET_SERVER_CLOSE, getSocketfd(), 0, 0);

}

int CATcpClient::getEventFilter()
{
	return CATCP_MSQ_EVENT_FILTER;
}

void CATcpClient::checkIdle()
{
	double diff;
	diff = difftime(nowSecond(), mSocketServer.ulAliveTime);

	if (IDLE_TIMEOUT < (int) diff)
	{
		_log("[CATcpClient] %s Socket Client: %d idle: %d seconds", strTaskName.c_str(), mSocketServer, (int) diff);
		closeServer();
	}
}

void CATcpClient::updateClientAlive()
{
	mSocketServer.ulAliveTime = nowSecond();
}

void CATcpClient::setIdleTimeout(int nSeconds)
{
	IDLE_TIMEOUT = nSeconds;
}

void CATcpClient::runIdleTimeout(bool bRun)
{
	if (bRun && (0 < IDLE_TIMEOUT))
	{
		setTimer(IDLE_TIMER, 3, 1, CATCP_MSQ_EVENT_FILTER);
	}
	else
	{
		killTimer(IDLE_TIMER);
	}
}
/**========================================================================================================
 *  IPC Message queue callback function.
 *  Receive MSQ message from sendMessage.
 */
void CATcpClient::onReceiveMessage(int nEvent, int nCommand, unsigned long int nId, int nDataLen, const void* pData)
{
	if (callbackReceiveMessage(nEvent, nCommand, nId, nDataLen, pData))
	{
		return;
	}
	unsigned long int ulThreadID;
	unsigned long int ulSocjetFD;

	switch (nCommand)
	{

	case EVENT_COMMAND_SOCKET_CONNECT:
		onServerConnect(nId);

		_log("[CATcpClient] %s Socket Server Connect FD: %lu", strTaskName.c_str(), nId);
		updateClientAlive();
		break;
	case EVENT_COMMAND_SOCKET_DISCONNECT: // Server Disconnect

		onServerDisconnect(nId);

		ulThreadID = getThreadID();
		if (ulThreadID)
		{
			threadJoin(ulThreadID);
		}
		_log("[CATcpClient] %s Socket Server Disconnect FD: %lu", strTaskName.c_str(), nId);
		break;
	case EVENT_COMMAND_SOCKET_SERVER_CLOSE: // Client close Server
		ulThreadID = getThreadID();
		if (ulThreadID)
		{
			threadCancel(ulThreadID);
			threadJoin(ulThreadID);
		}
		break;
	case EVENT_COMMAND_THREAD_EXIT:

		_log("[CATcpClient] %s Receive Thread Joined, Thread ID: %lu", strTaskName.c_str(), nId);
		break;
	case EVENT_COMMAND_SOCKET_CLIENT_RECEIVE:

		onReceive(nId, nDataLen, pData);

		break;
	case EVENT_COMMAND_TIMER:
		switch (nId)
		{
		case IDLE_TIMER:
			checkIdle();
			break;
		default:
			onTimer(nId); // overload function
		}
		break;
	case EVENT_COMMAND_SOCKET_TCP_CONNECT_ALIVE:
		updateClientAlive();
		break;
	default:
		_log("[CATcpClient] %s Unknown message command %s", strTaskName.c_str(), numberToHex(nCommand).c_str());
		break;
	}

}

string CATcpClient::taskName()
{
	return "CATcpClient";
}
