/*
 * CSocketClient.cpp
 *
 *  Created on: Sep 6, 2012
 *      Author: jugo
 */

#include "CSocketClient.h"
#include "CThreadHandler.h"
#include "IReceiver.h"
#include "global_inc/packet.h"
#include "LogHandler.h"
#include "CDataHandler.cpp"
#include "event.h"

int CSocketClient::m_nInternalEventFilter = EVENT_FILTER_SOCKET_CLIENT;

void *threadClientCMPHandler(void *argv)
{
	CSocketClient* ss = reinterpret_cast<CSocketClient*>(argv);
	int nFD = ss->m_nClientFD;
	ss->threadUnLock();
	ss->runCMPHandler(nFD);
	return NULL;
}

void *threadClientMessageReceive(void *argv)
{
	CSocketClient* ss = reinterpret_cast<CSocketClient*>(argv);
	ss->runMessageReceive();
	return NULL;
}

void *threadClientDataHandler(void *argv)
{
	CSocketClient* ss = reinterpret_cast<CSocketClient*>(argv);
	int nFD = ss->m_nClientFD;
	ss->threadUnLock();
	ss->runDataHandler(nFD);
	return NULL;
}

CSocketClient::CSocketClient() :
		CSocket(), m_nClientFD(-1), threadHandler(new CThreadHandler), mnPacketType(PK_CMP), mnPacketHandle(PK_MSQ), mThreadId(
				0), munRunThreadId(0)
{
	m_nInternalFilter = ++m_nInternalEventFilter;
	externalEvent.init();
}

CSocketClient::~CSocketClient()
{
	delete threadHandler;
}

int CSocketClient::start(int nSocketType, const char* cszAddr, short nPort, int nStyle)
{
	if( AF_UNIX == nSocketType)
	{
		setDomainSocketPath(cszAddr);
	}
	else if( AF_INET == nSocketType)
	{
		if(-1 == setInetSocket(cszAddr, nPort))
		{
			_DBG("set INET socket address & port fail");
			return -1;
		}
	}

	if(-1 != createSocket(nSocketType, nStyle))
	{
		if( SOCK_STREAM == nStyle)
		{
			if(-1 == connectServer())
			{
				socketClose();
				return -1;
			}
		}

		if(-1 != externalEvent.m_nMsgId)
		{
			if(-1 == initMessage(externalEvent.m_nMsgId, "CSocketClient"))
			{
				_log("[Socket Client] socket client create message id fail");
			}
		}
		else
		{
			if(-1 == initMessage(m_nInternalFilter, "CSocketClient"))
			{
				_log("[Socket Client] socket client create message id fail");
			}
		}

		if(0 != munRunThreadId)
		{
			threadHandler->threadCancel(munRunThreadId);
			threadHandler->threadJoin(munRunThreadId);
			munRunThreadId = 0;
		}
		threadHandler->createThread(threadClientMessageReceive, this);
		switch(mnPacketType)
		{
		case PK_BYTE:
			dataHandler((int) getSocketfd());
			break;
		case PK_CMP:
			cmpHandler((int) getSocketfd());
			break;
		}

		_log("[Socket Client] Socket connect success, FD:%d", getSocketfd());
		return getSocketfd();
	}

	return -1;
}

void CSocketClient::threadLock()
{
	threadHandler->threadLock();
}

void CSocketClient::threadUnLock()
{
	threadHandler->threadUnlock();
}

void CSocketClient::dataHandler(int nFD)
{
	this->threadLock();
	this->m_nClientFD = nFD;
	threadHandler->createThread(threadClientDataHandler, this);
}

void CSocketClient::cmpHandler(int nFD)
{
	this->threadLock();
	this->m_nClientFD = nFD;
	threadHandler->createThread(threadClientCMPHandler, this);
}

void CSocketClient::stop()
{
	socketClose();
	if(mThreadId)
	{
		pthread_t pid = mThreadId;
		mThreadId = 0;
		threadHandler->threadCancel(pid);
		sendMessage(m_nInternalFilter, EVENT_COMMAND_THREAD_EXIT, pid, 0, NULL);
	}
}

void CSocketClient::setPackageReceiver(int nMsgId, int nEventFilter, int nCommand)
{
	externalEvent.m_nMsgId = nMsgId;
	externalEvent.m_nEventFilter = nEventFilter;
	externalEvent.m_nEventRecvCommand = nCommand;
}

void CSocketClient::setClientDisconnectCommand(int nCommand)
{
	externalEvent.m_nEventDisconnect = nCommand;
}

void CSocketClient::runMessageReceive()
{
	munRunThreadId = threadHandler->getThreadID();
	run(m_nInternalFilter, "SocketClient");
	threadHandler->threadExit();
	threadHandler->threadJoin(threadHandler->getThreadID());
}

void CSocketClient::onReceiveMessage(int nEvent, int nCommand, unsigned long int nId, int nDataLen, const void* pData)
{
	switch(nCommand)
	{
	case EVENT_COMMAND_THREAD_EXIT:
		threadHandler->threadJoin(nId);
		_log("[Socket Client] Thread Join:%d", (int) nId);
		break;
	case EVENT_COMMAND_SOCKET_CLIENT_RECEIVE:
		_log("[Socket Client] Receive CMP , SocketFD:%d", (int) nId);
		break;
	case EVENT_COMMAND_TIMER:
		break;
	default:
		_log("[Socket Server] unknow message command");
		break;
	}
}

void CSocketClient::setPacketConf(int nType, int nHandle)
{
	if(0 <= nType && PK_TYPE_SIZE > nType)
		mnPacketType = nType;

	if(0 <= nHandle && PK_HANDLE_SIZE > nHandle)
		mnPacketHandle = nHandle;
}

int CSocketClient::runDataHandler(int nClientFD)
{
	int result;
	char pBuf[BUF_SIZE];
	void* pvBuf = pBuf;

	/**
	 * clientSockaddr is used for UDP server send packet to client.
	 */
	struct sockaddr_in *clientSockaddr;
	clientSockaddr = new struct sockaddr_in;

	if(externalEvent.isValid() && -1 != externalEvent.m_nEventConnect)
	{
		sendMessage(externalEvent.m_nEventFilter, externalEvent.m_nEventConnect, nClientFD, 0, 0);
	}

	mThreadId = threadHandler->getThreadID();

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

		switch(mnPacketHandle)
		{
		case PK_MSQ:
			if(externalEvent.isValid())
			{
				sendMessage(externalEvent.m_nEventFilter, externalEvent.m_nEventRecvCommand, nClientFD, result, pBuf);
			}
			else
			{
				sendMessage(m_nInternalFilter, EVENT_COMMAND_SOCKET_SERVER_RECEIVE, nClientFD, result, pBuf);
			}
			break;
		case PK_ASYNC:
			ClientReceive(nClientFD, result, pBuf);
			break;
		}
	}

	mThreadId = 0;

	delete clientSockaddr;

	sendMessage(m_nInternalFilter, EVENT_COMMAND_THREAD_EXIT, threadHandler->getThreadID(), 0, NULL);

	threadHandler->threadSleep(1);
	threadHandler->threadExit();

	return 0;
}

int CSocketClient::runCMPHandler(int nClientFD)
{
	int result = 0;
	int nTotalLen = 0;
	int nBodyLen = 0;
	int nCommand = generic_nack;
	int nSequence = 0;

	CMP_PACKET cmpPacket;
	void* pHeader = &cmpPacket.cmpHeader;
	void* pBody = &cmpPacket.cmpBody;

	CMP_HEADER cmpHeader;
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

	mThreadId = threadHandler->getThreadID();

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
				_log("[CSocketClient] get Enquire Link Request!");
				socketSend(nClientFD, &cmpHeader, sizeof(CMP_HEADER));
				continue;
			}

			nBodyLen = nTotalLen - sizeof(CMP_HEADER);

			//START Joe add deal with Big Data
			if(static_cast<int>(nTotalLen) > (MAX_SIZE - 1))
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
			//END Joe add deal with Big Data

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
					_log("[Socket Client] socket close client: %d , packet length error: %d != %d", nClientFD, nBodyLen,
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
			_log("[Socket Client] Send Message: Please use CMP to communicate");

			if(externalEvent.isValid() && -1 != externalEvent.m_nEventDisconnect)
			{
				sendMessage(externalEvent.m_nEventFilter, externalEvent.m_nEventDisconnect, nClientFD, 0, 0);
			}

			socketClose(nClientFD);
			_log("[Socket Client] socket close client: %d , packet header length error: %d", nClientFD, result);
			break;
		}

		switch(mnPacketHandle)
		{
		case PK_MSQ:
			if(externalEvent.isValid())
			{
				sendMessage(externalEvent.m_nEventFilter, externalEvent.m_nEventRecvCommand, nClientFD, nTotalLen,
						&cmpPacket);
			}
			else
			{
				sendMessage(m_nInternalFilter, EVENT_COMMAND_SOCKET_SERVER_RECEIVE, nClientFD, nTotalLen, &cmpPacket);
			}
			break;
		case PK_ASYNC:
			ClientReceive(nClientFD, nTotalLen, &cmpPacket);
			break;
		}

	} // while

	mThreadId = 0;

	delete clientSockaddr;

	sendMessage(m_nInternalFilter, EVENT_COMMAND_THREAD_EXIT, threadHandler->getThreadID(), 0, NULL);

	threadHandler->threadSleep(1);
	threadHandler->threadExit();

	return 0;
}
