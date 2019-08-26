/*
 * CSocketClient.h
 *
 *  Created on: 2015年10月21日
 *      Author: jugo
 */

#pragma once

#include "CSocket.h"
#include "objectHandler/CObject.h"

class CThreadHandler;

class CSocketClient: public CSocket, public CObject
{
public:
	CSocketClient();
	virtual ~CSocketClient();
	int start(int nSocketType, const char* cszAddr, short nPort, int nStyle = SOCK_STREAM);
	void stop();
	void setPackageReceiver(int nMsgId, int nEventFilter, int nCommand);
	void setClientDisconnectCommand(int nCommand);
	void runMessageReceive();
	int runDataHandler(int nClientFD);
	int runCMPHandler(int nClientFD);
	void setPacketConf(int nType, int nHandle);
	void threadLock();
	void threadUnLock();

public:
	int m_nClientFD;

protected:
	void onReceiveMessage(int nEvent, int nCommand, unsigned long int nId, int nDataLen, const void* pData);
	virtual void onTimer(int nId)
	{
		printf("[CSocketClient] onTimer Id:%d\n", nId);
	}
	;

private:
	void dataHandler(int nFD);
	void cmpHandler(int nFD);
	CThreadHandler *threadHandler;
	EVENT_EXTERNAL externalEvent;
	static int m_nInternalEventFilter;
	int m_nInternalFilter;
	int mnPacketType;
	int mnPacketHandle;
	unsigned long int mThreadId;
	unsigned long int munRunThreadId;
};

