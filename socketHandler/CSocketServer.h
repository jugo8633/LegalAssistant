/*
 * CSocketServer.h
 *
 *  Created on: Dec 02, 2014
 *      Author: jugo
 */

#pragma once

#include <map>
#include "CSocket.h"
#include "objectHandler/CObject.h"

class CThreadHandler;

template<typename T>
class CDataHandler;

class CSocketServer: public CSocket, public CObject
{
public:
	explicit CSocketServer();
	virtual ~CSocketServer();
	int start(int nSocketType, const char* cszAddr, short nPort, int nStyle = SOCK_STREAM);
	void stop();
	void setPackageReceiver(int nMsgId, int nEventFilter, int nCommand);
	int runDataHandler(int nClientFD);
	int runCMPHandler(int nClientFD);
	void runSocketAccept();
	void runMessageReceive();
	int getInternalEventFilter() const;
	void setClientConnectCommand(int nCommand);
	void setClientDisconnectCommand(int nCommand);
	int sendtoUDPClient(int nClientId, const void* pBuf, int nBufLen);
	void eraseUDPCliefnt(int nClientId);
	void threadLock();
	void threadUnLock();
	void setPacketConf(int nType, int nHandle);
	void closeClient(int nClientFD);

protected:
	void onReceiveMessage(int nEvent, int nCommand, unsigned long int nId, int nDataLen, const void* pData);
	virtual void onTimer(int nId)
	{
		printf("[CSocketServer] onTimer Id:%d\n", nId);
	}
	;

private:
	int recvHandler();
	void dataHandler(int nFD);
	void cmpHandler(int nFD);

public:
	int m_nClientFD;

private:
	CThreadHandler *threadHandler;
	EVENT_EXTERNAL externalEvent;
	static int m_nInternalEventFilter;
	int m_nInternalFilter;
	CDataHandler<struct sockaddr_in> *udpClientData;
	int mnPacketType;
	int mnPacketHandle;
	std::map<unsigned long int, unsigned long int> mapClientThread;
	unsigned long int munRunThreadId;
};

