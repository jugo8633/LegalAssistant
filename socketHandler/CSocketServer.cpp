/*
 * CSocketServer.cpp
 *
 *  Created on: 2015年10月19日
 *      Author: Louis Ju
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <typeinfo>
#include "CSocketServer.h"
#include "CThreadHandler.h"
#include "global_inc/common.h"
#include "CDataHandler.cpp"
#include "global_inc/packet.h"
#include "socketHandler/IReceiver.h"
#include "global_inc/event.h"

int CSocketServer::m_nInternalEventFilter = EVENT_FILTER_SOCKET_SERVER;

/** Thread Function Run **/
void *threadServerCMPHandler(void *argv)
{
	CSocketServer* ss = reinterpret_cast<CSocketServer*>(argv);
	int nFD = ss->m_nClientFD;
	ss->threadUnLock();
	ss->runCMPHandler(nFD);
	return NULL;
}

void *threadServerMessageReceive(void *argv)
{
	CSocketServer* ss = reinterpret_cast<CSocketServer*>(argv);
	ss->runMessageReceive();
	return NULL;
}

void *threadServerSocketAccept(void *argv)
{
	CSocketServer* ss = reinterpret_cast<CSocketServer*>(argv);
	ss->runSocketAccept();
	return NULL;
}

void *threadServerDataHandler(void *argv)
{
	CSocketServer* ss = reinterpret_cast<CSocketServer*>(argv);
	int nFD = ss->m_nClientFD;
	ss->threadUnLock();
	ss->runDataHandler(nFD);
	return NULL;
}

CSocketServer::CSocketServer() :
		CSocket(), m_nClientFD(-1), threadHandler(new CThreadHandler), udpClientData(0), mnPacketType(PK_CMP), mnPacketHandle(
				PK_MSQ), munRunThreadId(0)
{
	m_nInternalFilter = ++m_nInternalEventFilter;
	externalEvent.init();
}

CSocketServer::~CSocketServer()
{
	stop();
	delete threadHandler;
	if(udpClientData)
	{
		delete udpClientData;
	}
}

int CSocketServer::start(int nSocketType, const char* cszAddr, short nPort, int nStyle)
{
	int nMsgId = -1;

	if(-1 != externalEvent.m_nMsgId)
	{
		nMsgId = initMessage(externalEvent.m_nMsgId, "CSocketServer");
	}
	else
	{
		nMsgId = initMessage(m_nInternalFilter, "CSocketServer");
	}

	if(-1 == nMsgId)
	{
		_log("[CSocketServer] Init Message Queue Fail");
		return -1;
	}

	if(0 != munRunThreadId)
	{
		threadHandler->threadCancel(munRunThreadId);
		threadHandler->threadJoin(munRunThreadId);
		munRunThreadId = 0;
	}

	threadHandler->createThread(threadServerMessageReceive, this);

	if( AF_UNIX == nSocketType)
	{
		setDomainSocketPath(cszAddr);
	}
	else if( AF_INET == nSocketType)
	{
		if(-1 == setInetSocket(cszAddr, nPort))
		{
			_log("set INET socket address & port fail");
			return -1;
		}
	}

	if(-1 != createSocket(nSocketType, nStyle))
	{
		if(-1 != socketBind())
		{
			if( SOCK_STREAM == nStyle)
			{
				if(-1 == socketListen( BACKLOG))
				{
					perror("socket listen");
					socketClose();
					return -1;
				}

				threadHandler->createThread(threadServerSocketAccept, this);
			}
			else if( SOCK_DGRAM == nStyle)
			{
				if(udpClientData)
					delete udpClientData;
				udpClientData = new CDataHandler<struct sockaddr_in>;
				dataHandler(getSocketfd());
			}
			return 0;
		}
		else
		{
			socketClose();
		}
	}

	return -1;
}

void CSocketServer::stop()
{
	socketClose();

	// 清除所有Client端連線後產生的receive執行緒
	map<unsigned long int, unsigned long int>::iterator it;
	for(it = mapClientThread.begin(); mapClientThread.end() != it; ++it)
	{
		pthread_t pid = (*it).second;
		threadHandler->threadCancel(pid);
		threadHandler->threadJoin(pid);
	}
}

void CSocketServer::dataHandler(int nFD)
{
	this->threadLock();
	this->m_nClientFD = nFD;
	threadHandler->createThread(threadServerDataHandler, this);
}

void CSocketServer::cmpHandler(int nFD)
{
	this->threadLock();
	this->m_nClientFD = nFD;
	threadHandler->createThread(threadServerCMPHandler, this);
}

void CSocketServer::threadLock()
{
	threadHandler->threadLock();
}

void CSocketServer::threadUnLock()
{
	threadHandler->threadUnlock();
}

int CSocketServer::runDataHandler(int nClientFD)
{
	int nFD;
	int result;
	char pBuf[BUF_SIZE];
	void* pvBuf = pBuf;
	char szTmp[16];

	/**
	 * clientSockaddr is used for UDP server send packet to client.
	 */
	struct sockaddr_in *clientSockaddr;
	clientSockaddr = new struct sockaddr_in;

	if(externalEvent.isValid() && -1 != externalEvent.m_nEventConnect)
	{
		sendMessage(externalEvent.m_nEventFilter, externalEvent.m_nEventConnect, nClientFD, 0, 0);
	}

	while(1)
	{
		memset(pBuf, 0, sizeof(pBuf));
		result = socketrecv(nClientFD, &pvBuf, clientSockaddr);

		if(0 >= result)
		{
			if(externalEvent.isValid() && -1 != externalEvent.m_nEventDisconnect)
			{
				sendMessage(externalEvent.m_nEventFilter, externalEvent.m_nEventDisconnect, nClientFD, 0, 0);
			}
			socketClose(nClientFD);
			break;
		}

		if(nClientFD == getSocketfd())
		{
			/**
			 * UDP server receive packet
			 */
			nFD = ntohs(clientSockaddr->sin_port);
			memset(szTmp, 0, sizeof(szTmp));
			sprintf(szTmp, "%d", nFD);
			udpClientData->setData(szTmp, *clientSockaddr);
		}
		else
		{
			nFD = nClientFD;
		}

		switch(mnPacketHandle)
		{
		case PK_MSQ:
			if(externalEvent.isValid())
			{
				sendMessage(externalEvent.m_nEventFilter, externalEvent.m_nEventRecvCommand, nFD, result, pBuf);
			}
			else
			{
				sendMessage(m_nInternalFilter, EVENT_COMMAND_SOCKET_SERVER_RECEIVE, nFD, result, pBuf);
			}
			break;
		case PK_ASYNC:
			ServerReceive(nFD, result, pBuf);
			break;
		}
	}

	delete clientSockaddr;

	sendMessage(m_nInternalFilter, EVENT_COMMAND_THREAD_EXIT, threadHandler->getThreadID(), 0, NULL);

	threadHandler->threadSleep(1);
	threadHandler->threadExit();

	return 0;
}

int CSocketServer::runCMPHandler(int nClientFD)
{
	int nFD;
	int result = 0;
	char szTmp[16];
	int nTotalLen = 0;
	int nBodyLen = 0;
	int nCommand = generic_nack;
	int nSequence = 0;

	CMP_PACKET cmpPacket;
	void* pHeader = &cmpPacket.cmpHeader;
	void* pBody;

	CMP_HEADER cmpHeader;
	//void *pHeaderResp = &cmpHeader;
	int nCommandResp;

	/**
	 * clientSockaddr is used for UDP server send packet to client.
	 */
	struct sockaddr_in *clientSockaddr;
	clientSockaddr = new struct sockaddr_in;

	if(externalEvent.isValid() && -1 != externalEvent.m_nEventConnect)
	{
		sendMessage(externalEvent.m_nEventFilter, externalEvent.m_nEventConnect, nClientFD, 0, 0);
	}

	mapClientThread[nClientFD] = threadHandler->getThreadID();

	while(1)
	{
		memset(&cmpPacket, 0, sizeof(cmpPacket));
		result = socketrecv(nClientFD, sizeof(CMP_HEADER), &pHeader, clientSockaddr);
		if(sizeof(CMP_HEADER) == result)
		{
			nTotalLen = ntohl(cmpPacket.cmpHeader.command_length);
			nCommand = ntohl(cmpPacket.cmpHeader.command_id);
			nSequence = ntohl(cmpPacket.cmpHeader.sequence_number);

			memset(&cmpHeader, 0, sizeof(CMP_HEADER));
			nCommandResp = generic_nack | nCommand;
			cmpHeader.command_id = htonl(nCommandResp);
			cmpHeader.command_status = htonl( STATUS_ROK);
			cmpHeader.sequence_number = htonl(nSequence);
			cmpHeader.command_length = htonl(sizeof(CMP_HEADER));

			if( enquire_link_request == nCommand)
			{
				socketSend(nClientFD, &cmpHeader, sizeof(CMP_HEADER));
				continue;
			}
			nBodyLen = nTotalLen - sizeof(CMP_HEADER);

			if(static_cast<int>(nTotalLen) > MAX_SIZE - 1)
			{
				cmpPacket.cmpBodyUnlimit.cmpdata = new char[nBodyLen + 1];

				pBody = cmpPacket.cmpBodyUnlimit.cmpdata;
				result = socketrecv(nClientFD, nBodyLen, &pBody, clientSockaddr);

				if(result == nBodyLen)
				{
					ServerReceive(nClientFD, static_cast<int>(nTotalLen), &cmpPacket);
					continue;
				}
				else
				{
					socketSend(nClientFD, "Invalid Packet\r\n", strlen("Invalid Packet\r\n"));
					if(externalEvent.isValid() && -1 != externalEvent.m_nEventDisconnect)
					{
						sendMessage(externalEvent.m_nEventFilter, externalEvent.m_nEventDisconnect, nClientFD, 0, 0);
					}
					socketClose(nClientFD);
					_log("[Socket Server] socket close client: %d , packet length error: %d != %d", nClientFD, nBodyLen,
							result);
					break;
				}

			}
			else
			{
				pBody = &cmpPacket.cmpBody;
			}

			if(0 < nBodyLen)
			{
				result = socketrecv(nClientFD, nBodyLen, &pBody, clientSockaddr);
				if(result != nBodyLen)
				{
					socketSend(nClientFD, "Invalid Packet\r\n", strlen("Invalid Packet\r\n"));
					if(externalEvent.isValid() && -1 != externalEvent.m_nEventDisconnect)
					{
						sendMessage(externalEvent.m_nEventFilter, externalEvent.m_nEventDisconnect, nClientFD, 0, 0);
					}
					socketClose(nClientFD);
					_log("[Socket Server] socket close client: %d , packet length error: %d != %d", nClientFD, nBodyLen,
							result);
					break;
				}
			}

			if(access_log_request == nCommand)
			{
				socketSend(nClientFD, &cmpHeader, sizeof(CMP_HEADER));
			}
		}
		else if(0 >= result)
		{
			if(externalEvent.isValid() && -1 != externalEvent.m_nEventDisconnect)
			{
				sendMessage(externalEvent.m_nEventFilter, externalEvent.m_nEventDisconnect, nClientFD, 0, 0);
			}
			socketClose(nClientFD);
			break;
		}
		else
		{
			socketSend(nClientFD, "Control Center: Please use CMP to communicate\r\n",
					strlen("Control Center: Please use CMP to communicate\r\n"));
			_log("[Socket Server] Send Message: Please use CMP to communicate");

			if(externalEvent.isValid() && -1 != externalEvent.m_nEventDisconnect)
			{
				sendMessage(externalEvent.m_nEventFilter, externalEvent.m_nEventDisconnect, nClientFD, 0, 0);
			}

			socketClose(nClientFD);
			_log("[Socket Server] socket close client: %d , packet header length error: %d", nClientFD, result);
			break;
		}

		if(nClientFD == getSocketfd())
		{
			/**
			 * UDP server receive packet,record client information
			 */
			nFD = ntohs(clientSockaddr->sin_port);
			memset(szTmp, 0, sizeof(szTmp));
			sprintf(szTmp, "%d", nFD);
			udpClientData->setData(szTmp, *clientSockaddr);
		}
		else
		{
			nFD = nClientFD;
		}

		switch(mnPacketHandle)
		{
		case PK_MSQ:
			if(externalEvent.isValid())
			{
				sendMessage(externalEvent.m_nEventFilter, externalEvent.m_nEventRecvCommand, nFD, nTotalLen,
						&cmpPacket);
			}
			else
			{
				sendMessage(m_nInternalFilter, EVENT_COMMAND_SOCKET_SERVER_RECEIVE, nFD, nTotalLen, &cmpPacket);

			}
			break;
		case PK_ASYNC:
			ServerReceive(nFD, nTotalLen, &cmpPacket);
			break;
		}

	} // while

	mapClientThread.erase(nClientFD);

	delete clientSockaddr;

	sendMessage(m_nInternalFilter, EVENT_COMMAND_THREAD_EXIT, threadHandler->getThreadID(), 0, NULL);

	threadHandler->threadSleep(1);
	threadHandler->threadExit();

	return 0;
}

void CSocketServer::runSocketAccept()
{
	int nChildSocketFD = -1;

	while(1)
	{
		nChildSocketFD = socketAccept();

		if(-1 != nChildSocketFD)
		{
			_log("[CSocketServer] Socket Accept, Client Socket ID: %d", nChildSocketFD);
			sendMessage(m_nInternalFilter, EVENT_COMMAND_SOCKET_ACCEPT, nChildSocketFD, 0, NULL);
		}
		else
		{
			_log("socket server accept fail");
			sleep(3);
		}
	}

	sendMessage(m_nInternalFilter, EVENT_COMMAND_THREAD_EXIT, threadHandler->getThreadID(), 0, NULL);
	threadHandler->threadExit();
}

void CSocketServer::runMessageReceive()
{
	munRunThreadId = threadHandler->getThreadID();
	run(m_nInternalFilter, "SocketServer");
	threadHandler->threadExit();
	threadHandler->threadJoin(threadHandler->getThreadID());
}

void CSocketServer::setPackageReceiver(int nMsgId, int nEventFilter, int nCommand)
{
	externalEvent.m_nMsgId = nMsgId;
	externalEvent.m_nEventFilter = nEventFilter;
	externalEvent.m_nEventRecvCommand = nCommand;
}

void CSocketServer::setClientConnectCommand(int nCommand)
{
	externalEvent.m_nEventConnect = nCommand;
}

void CSocketServer::setClientDisconnectCommand(int nCommand)
{
	externalEvent.m_nEventDisconnect = nCommand;
}

void CSocketServer::onReceiveMessage(int nEvent, int nCommand, unsigned long int nId, int nDataLen, const void* pData)
{
	switch(nCommand)
	{
	case EVENT_COMMAND_SOCKET_ACCEPT:
		switch(mnPacketType)
		{
		case PK_BYTE:
			dataHandler((int) nId);
			break;
		case PK_CMP:
			cmpHandler((int) nId);
			break;
		}
		break;
	case EVENT_COMMAND_THREAD_EXIT:
		threadHandler->threadJoin(nId);
		_log("[CSocketServer] Receive Thread Joined, Thread ID: %lu", nId);
		break;
	case EVENT_COMMAND_SOCKET_SERVER_RECEIVE:
		_log("[CSocketServer] Receive Package , Socket FD: %lu", nId);
		break;
	case EVENT_COMMAND_TIMER:
		_log("[CSocketServer] On Timer , ID: %lu", nId);
		break;
	default:
		_log("[Socket Server] unknow message command");
		break;
	}
}

int CSocketServer::getInternalEventFilter() const
{
	return m_nInternalFilter;
}

int CSocketServer::sendtoUDPClient(int nClientId, const void* pBuf, int nBufLen)
{
	int nSend = -1;
	char szId[16];

	memset(szId, 0, sizeof(szId));
	sprintf(szId, "%d", nClientId);
	if(udpClientData && udpClientData->isValidKey(szId))
	{
		nSend = socketSend((*udpClientData)[szId], pBuf, nBufLen);
	}

	return nSend;
}

void CSocketServer::eraseUDPCliefnt(int nClientId)
{
	char szId[16];

	memset(szId, 0, sizeof(szId));
	sprintf(szId, "%d", nClientId);
	if(udpClientData && udpClientData->isValidKey(szId))
	{
		udpClientData->erase(szId);
	}

	_DBG("[Socket Server] UDP client %d in queue", udpClientData->size());
}

void CSocketServer::setPacketConf(int nType, int nHandle)
{
	if(0 <= nType && PK_TYPE_SIZE > nType)
		mnPacketType = nType;

	if(0 <= nHandle && PK_HANDLE_SIZE > nHandle)
		mnPacketHandle = nHandle;
}

void CSocketServer::closeClient(int nClientFD)
{
	socketClose(nClientFD);
	if(mapClientThread.end() != mapClientThread.find(nClientFD))
	{
		pthread_t pid = mapClientThread[nClientFD];
		mapClientThread.erase(nClientFD);
		threadHandler->threadCancel(pid);
		sendMessage(m_nInternalFilter, EVENT_COMMAND_THREAD_EXIT, pid, 0, NULL);
	}
}

