/*
 * Handler.cpp
 *
 *  Created on: 2017年4月27日
 *      Author: root
 */

#include <sys/time.h>
#include "Handler.h"
#include "global_inc/event.h"
#include "messageHandler/CMessageHandler.h"
#include "global_inc/common.h"

void *threadHandlerMessageReceive(void *argv)
{
	Handler* ss = reinterpret_cast<Handler*>(argv);
	ss->runMessageReceive();
	return 0;
}

inline double createMsqKey()
{
	timeval tv;
	gettimeofday(&tv, NULL);
	return double(tv.tv_sec) + 0.000001 * tv.tv_usec;
}

Handler::Handler(const int nMsqKey, const long lFilter) :
		mnMsqKey(createMsqKey()), mnMsqId(-1), pHandleMessage(0), mpInstance(0), mlFilter(-1)
{
	close();
	if(-1 != nMsqKey)
		mnMsqKey = nMsqKey;
	mnMsqId = initMessage(mnMsqKey, "Handler");
	if(-1 != lFilter)
		mlFilter = lFilter;
	else
		mlFilter = mnMsqId;
	if(0 < mnMsqId)
	{
		mlnThreadId = createThread(threadHandlerMessageReceive, this, "Handler Message Receive Thread");
	}
}

Handler::~Handler()
{
	threadCancel(mlnThreadId);
	threadJoin(mlnThreadId);
}

void Handler::close()
{
	if(-1 != mnMsqId)
		CMessageHandler::closeMsg(mnMsqId);
}

void Handler::onHandleMessage(Message &message)
{
	if(pHandleMessage && mpInstance)
		(*pHandleMessage)(message.what, message.arg[0], message.arg[1], 0, mpInstance);
}

void Handler::runMessageReceive()
{
	if(0 > mlFilter)
		mlFilter = mnMsqId;
	run(mlFilter, "Handler");
	threadExit();
	threadJoin(getThreadID());
	_log("[Handler] runMessageReceive Stop, Thread join");
}

void Handler::setHandleMessageListener(void *pInstance, pfnHandleMessage handleMessage)
{
	mpInstance = pInstance;
	pHandleMessage = handleMessage;
}

int Handler::sendMessage(Message &message, long lFilter)
{
	long filter;
	if(-1 == lFilter)
		filter = mlFilter;
	else
		filter = lFilter;
	return CObject::sendMessage(filter, message);
}
