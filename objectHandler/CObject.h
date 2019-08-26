/*
 * CObject.cpp
 *
 *  Created on: 2014年12月2日
 *      Author: jugo
 */

#pragma once

#include <map>
#include <stdio.h>
#include <sys/types.h>
#include "messageHandler/CMessageHandler.h"

#define _OBJ(obj)			reinterpret_cast<CObject*>(obj)
#define _VOID(const_char)	const_cast<void*>(reinterpret_cast<const void*>(const_char))

struct EVENT_EXTERNAL
{
	int m_nMsgId;
	int m_nEventFilter;
	int m_nEventRecvCommand;
	int m_nEventDisconnect;
	int m_nEventConnect;
	void init()
	{
		m_nMsgId = -1;
		m_nEventFilter = -1;
		m_nEventRecvCommand = -1;
		m_nEventDisconnect = -1;
		m_nEventConnect = -1;
	}
	bool isValid()
	{
		if(-1 != m_nMsgId && -1 != m_nEventFilter && -1 != m_nEventRecvCommand)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
};

class CObject
{
public:
	explicit CObject();
	virtual ~CObject();
	void clearMessage();
	int sendMessage(int nEvent, int nCommand, unsigned long int nId, int nDataLen, const void* pData);
	int sendMessage(int nCommand, unsigned long int nId, int nDataLen, const void* pData);
	int sendMessage(int nEvent, int nCommand, unsigned long int nId);
	int sendMessage(int nEvent, Message &message);
	int sendMessage(Message &message);
	void _OnTimer(int nId);
	int initMessage(int nKey, const char * szDescript = 0);
	int run(int lFilter, const char * szDescript = 0);
	timer_t setTimer(int nId, int nSecStart, int nInterSec, int nEvent = -1);
	void killTimer(int nId);
	unsigned long int createThread(void* (*entry)(void*), void* arg, const char *szDesc = 0);
	void threadJoin(unsigned long int thdid);
	void threadExit();
	int threadCancel(unsigned long int thread);
	unsigned long int getThreadID();
	void closeMsq();

protected:
	// virtual function, child must overload
	virtual void onReceiveMessage(int lFilter, int nCommand, unsigned long int nId, int nDataLen,
			const void* pData) = 0;
	virtual void onHandleMessage(Message &message)
	{
	}
	;
	virtual void onTimer(int nId)
	{
	}
	;

	virtual std::string taskName();
private:
	CMessageHandler *messageHandler;
	int mnTimerEventId;
	int mnFilter;
};
