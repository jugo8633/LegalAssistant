/*
 * Handler.h
 *
 *  Created on: 2017年4月27日
 *      Author: Jugo
 */

#pragma once
#include "CObject.h"

typedef int (*pfnHandleMessage)(int, int, int, void *, void *);

using namespace std;

class Handler: public CObject
{
public:
	explicit Handler(const int nMsqKey = -1, const long lFilter = -1);
	virtual ~Handler();
	void close();
	void runMessageReceive();
	void setHandleMessageListener(void *pInstance, pfnHandleMessage handleMessage);
	int sendMessage(Message &message, long lFilter = -1);

protected:
	void onReceiveMessage(int nEvent, int nCommand, unsigned long int nId, int nDataLen, const void* pData)
	{
	}
	;
	void onHandleMessage(Message &message);

private:
	int mnMsqKey;
	int mnMsqId;
	pfnHandleMessage pHandleMessage;
	void *mpInstance;
	long mlFilter;
	unsigned long int mlnThreadId;
};

