/*
 * CATcpServer.cpp
 *
 *  Created on: 2017年3月15日
 *      Author: root
 */

#include <time.h>
#include "CATcpServer.h"
#include "CMessageHandler.h"
#include "LogHandler.h"
#include "event.h"
#include "utility.h"
#include "common.h"

using namespace std;

#define IDLE_TIMER			469107
#define MAX_CLIENT			666
int IDLE_TIMEOUT = 10; // secons

int mnExtMsqKey;

void *threadTcpAccept(void *argv)
{
	CATcpServer* ss = reinterpret_cast<CATcpServer*>(argv);
	ss->runSocketAccept();
	return 0;
}

void *threadCATcpServerMessageReceive(void *argv)
{
	CATcpServer* ss = reinterpret_cast<CATcpServer*>(argv);
	ss->runMessageReceive();
	return 0;
}

void *threadTcpReceive(void *argv)
{
	CATcpServer* ss = reinterpret_cast<CATcpServer*>(argv);
	ss->runTcpReceive();
	return 0;
}

int CATcpServer::start(const char* cszAddr, short nPort, int nMsqKey)
{
	int nMsgId = -1;
	int nSocketFD;
	mnExtMsqKey = FALSE;

	CATCP_MSQ_EVENT_FILTER = nPort;
	strTaskName = taskName();

	if(-1 != nMsqKey)
	{
		mnMsqKey = nMsqKey;
		mnExtMsqKey = TRUE;
	}
	else
		mnMsqKey = clock();

	if(-1 == mnMsqKey)
		mnMsqKey = 20150727;

	nMsgId = initMessage(mnMsqKey, strTaskName.c_str());

	if(-1 == nMsgId)
	{
		_log("[CATcpServer] %s Init Message Queue Fail", strTaskName.c_str());
		return -1;
	}

	if(-1 == setInetSocket(cszAddr, nPort))
	{
		_log("[CATcpServer] %s Set INET socket address & port fail", strTaskName.c_str());
		return -1;
	}

	nSocketFD = createSocket(AF_INET, SOCK_STREAM);

	if(-1 != nSocketFD)
	{

		if(-1 != socketBind())
		{
			if(-1 == socketListen(BACKLOG))
			{
				perror("socket listen");
				socketClose();
				return -1;
			}
			createThread(threadCATcpServerMessageReceive, this, "CATcpServer Message Receive");

			createThread(threadTcpAccept, this, "CATcpServer Socket Accept Thread");
			_log("[CATcpServer] %s Create Server Success Port: %d Socket FD: %lu", strTaskName.c_str(), nPort,
					nSocketFD);
		}
		else
		{
			socketClose();
		}
	}
	else
		_log("[CATcpServer] %s Create Server Fail", strTaskName.c_str());

	return nSocketFD;
}

void CATcpServer::stop()
{
	socketClose();

	/**
	 * Close Message queue run thread
	 */
	if(0 < munRunThreadId)
	{
		threadCancel(munRunThreadId);
		threadJoin(munRunThreadId);
		munRunThreadId = 0;

		if(!mnExtMsqKey)
		{
			CMessageHandler::closeMsg(CMessageHandler::registerMsq(mnMsqKey));
		}
	}

	/**
	 * Close all Client Socket
	 */
	map<unsigned long int, SOCKET_CLIENT>::iterator it;
	for(it = mapClient.begin(); mapClient.end() != it; ++it)
	{
		socketClose(it->first);
		threadCancel(it->second.ulReceiveThreadID);
		threadJoin(it->second.ulReceiveThreadID);
	}
	mapClient.clear();
}

void CATcpServer::closeClient(int nClientFD)
{
	if(mapClient.end() != mapClient.find(nClientFD))
	{
		sendMessage(CATCP_MSQ_EVENT_FILTER, EVENT_COMMAND_SOCKET_CLIENT_CLOSE, nClientFD, 0, 0);
	}
}

void CATcpServer::runSocketAccept()
{
	int nChildSocketFD = -1;

	_log("[CATcpServer] %s Thread runSocketAccept Start", strTaskName.c_str());
	while(1)
	{
		nChildSocketFD = socketAccept();

		if(MAX_CLIENT < (mapClient.size() + 1))
		{
			_log("[CATcpServer] %s Max Client Connect: %d", strTaskName.c_str(), mapClient.size());
			socketClose(nChildSocketFD);
			sleep(5);
			continue;
		}

		if(-1 != nChildSocketFD)
		{
			_log("[CATcpServer] %s Socket Accept, Client Socket ID: %d", strTaskName.c_str(), nChildSocketFD);
			sendMessage(CATCP_MSQ_EVENT_FILTER, EVENT_COMMAND_SOCKET_ACCEPT, nChildSocketFD, 0, NULL);
		}
		else
		{
			_log("[CATcpServer] %s socket server accept fail", strTaskName.c_str());
			sleep(3);
		}
	}

	_log("[CATcpServer] %s Thread runSocketAccept End", strTaskName.c_str());
	sendMessage(CATCP_MSQ_EVENT_FILTER, EVENT_COMMAND_THREAD_EXIT, getThreadID(), 0, 0);
	threadExit();
}

void CATcpServer::runMessageReceive()
{
	munRunThreadId = getThreadID();
	run(CATCP_MSQ_EVENT_FILTER, strTaskName.c_str());
	threadExit();
	threadJoin(getThreadID());
	_log("[CATcpServer] %s runMessageReceive Stop, Thread join", strTaskName.c_str());
}

int CATcpServer::getEventFilter()
{
	return CATCP_MSQ_EVENT_FILTER;
}

void CATcpServer::runTcpReceive()
{
	int result;
	int nSocketFD;

	nSocketFD = getClientSocketFD(getThreadID());
	if(0 >= nSocketFD)
	{
		_log("[CATcpServer] %s runTcpReceive Fail, Invalid Socket FD", strTaskName.c_str());
		sendMessage(CATCP_MSQ_EVENT_FILTER, EVENT_COMMAND_THREAD_EXIT, getThreadID(), 0, 0);
		threadExit();
		return;
	}

	sendMessage(CATCP_MSQ_EVENT_FILTER, EVENT_COMMAND_SOCKET_CONNECT, nSocketFD, 0, 0);

	while(1)
	{
		if(0 >= onTcpReceive(nSocketFD))
			break;
		sendMessage(CATCP_MSQ_EVENT_FILTER, EVENT_COMMAND_SOCKET_TCP_CONNECT_ALIVE, nSocketFD, 0, 0);
	}

	sendMessage(CATCP_MSQ_EVENT_FILTER, EVENT_COMMAND_SOCKET_DISCONNECT, nSocketFD, 0, 0);
	threadExit();
}

int CATcpServer::onTcpReceive(unsigned long int nSocketFD)
{
	int result;
	char pBuf[MAX_SOCKET_READ];
	void* pvBuf = pBuf;
	memset(pBuf, 0, sizeof(pBuf));
	result = socketrecv(nSocketFD, &pvBuf, 0);
	_log("[CATcpServer] %s onTcpReceive Result: %d Socket[%d]", strTaskName.c_str(), result, nSocketFD);

	if(0 < result)
	{
		sendMessage(CATCP_MSQ_EVENT_FILTER, EVENT_COMMAND_SOCKET_SERVER_RECEIVE, nSocketFD, result, pBuf);
	}
	return result;
}

unsigned long int CATcpServer::getClientSocketFD(unsigned long int unThreadId)
{
	map<unsigned long int, SOCKET_CLIENT>::iterator it;

	for(it = mapClient.begin(); mapClient.end() != it; ++it)
	{
		if(it->second.ulReceiveThreadID == unThreadId)
		{
			return it->first;
		}
	}
	return 0;
}

unsigned long int CATcpServer::getClientThreadID(unsigned long int unSocketFD)
{
	if(mapClient.find(unSocketFD) != mapClient.end())
	{
		return mapClient[unSocketFD].ulReceiveThreadID;
	}
	return 0;
}

void CATcpServer::setIdleTimeout(int nSeconds)
{
	IDLE_TIMEOUT = nSeconds;
}

void CATcpServer::runIdleTimeout(bool bRun)
{
	if(bRun && (0 < IDLE_TIMEOUT))
		setTimer(IDLE_TIMER, 3, 1, CATCP_MSQ_EVENT_FILTER);
	else
		killTimer(IDLE_TIMER);
}

void CATcpServer::checkIdle()
{
	map<unsigned long int, SOCKET_CLIENT>::iterator it;
	double diff;
	long int lnNow;

	for(it = mapClient.begin(); mapClient.end() != it; ++it)
	{
		lnNow = nowSecond();
		_log("[CATcpServer] checkIdle get now: %d", (int) lnNow);
		diff = difftime(lnNow, it->second.ulAliveTime);
		if(IDLE_TIMEOUT < (int) diff)
		{
			_log("[CATcpServer] %s Socket Client: %d idle: %d seconds", strTaskName.c_str(), it->first, (int) diff);
			closeClient(it->first);
		}
	}
}

void CATcpServer::eraseClient(unsigned long int ulSocketFD)
{
	socketClose(ulSocketFD);
	if(mapClient.find(ulSocketFD) != mapClient.end())
	{
		mapClient.erase(ulSocketFD);
	}
}

void CATcpServer::updateClientAlive(unsigned long int ulSocketFD)
{
	if(mapClient.find(ulSocketFD) != mapClient.end())
	{
		mapClient[ulSocketFD].ulAliveTime = nowSecond();
	}
}
/**========================================================================================================
 *  IPC Message queue callback function.
 *  Receive MSQ message from sendMessage.
 */
void CATcpServer::onReceiveMessage(int nEvent, int nCommand, unsigned long int nId, int nDataLen, const void* pData)
{
	if(callbackReceiveMessage(nEvent, nCommand, nId, nDataLen, pData))
		return;

	unsigned long int ulThreadID;
	unsigned long int ulSocjetFD;

	switch(nCommand)
	{
	case EVENT_COMMAND_SOCKET_ACCEPT:
		mapClient[nId].ulReceiveThreadID = createThread(threadTcpReceive, this);
		mapClient[nId].ulAliveTime = nowSecond();
		if(0 >= mapClient[nId].ulReceiveThreadID)
		{
			eraseClient(nId);
		}
		break;
	case EVENT_COMMAND_SOCKET_CONNECT:
		onClientConnect(nId);
		_log("[CATcpServer] %s Socket Client Connect FD: %lu", strTaskName.c_str(), nId);
		updateClientAlive(nId);
		break;
	case EVENT_COMMAND_SOCKET_DISCONNECT: // Client Disconnect
		onClientDisconnect(nId);
		ulThreadID = getClientThreadID(nId);
		if(ulThreadID)
			threadJoin(ulThreadID);
		eraseClient(nId);
		_log("[CATcpServer] %s Socket Client Disconnect FD: %lu", strTaskName.c_str(), nId);
		break;
	case EVENT_COMMAND_SOCKET_CLIENT_CLOSE: // Server close Client
		onClientDisconnect(nId);
		ulThreadID = getClientThreadID(nId);
		if(ulThreadID)
		{
			threadCancel(ulThreadID);
			threadJoin(ulThreadID);
		}
		eraseClient(nId);
		break;
	case EVENT_COMMAND_THREAD_EXIT:
		threadJoin(nId);
		ulSocjetFD = getClientSocketFD(nId);
		if(ulSocjetFD)
			eraseClient(ulSocjetFD);
		_log("[CATcpServer] %s Receive Thread Joined, Thread ID: %lu", strTaskName.c_str(), nId);
		break;
	case EVENT_COMMAND_SOCKET_SERVER_RECEIVE:
		onReceive(nId, nDataLen, pData);
		break;
	case EVENT_COMMAND_TIMER:
		switch(nId)
		{
		case IDLE_TIMER:
			checkIdle();
			break;
		default:
			onTimer(nId); // overload function
		}
		break;
	case EVENT_COMMAND_SOCKET_TCP_CONNECT_ALIVE:
		updateClientAlive(nId);
		break;
	default:
		_log("[CATcpServer] %s Unknow message command", strTaskName.c_str());
		break;
	}

}

string CATcpServer::taskName()
{
	return "CATcpServer";
}
