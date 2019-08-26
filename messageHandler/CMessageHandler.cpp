/*
 * CMessageHandler.cpp
 *
 *  Created on: Dec 02, 2014
 *      Author: jugo
 */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <set>
#include "CMessageHandler.h"
#include "global_inc/common.h"
#include "logHandler/LogHandler.h"

using namespace std;

static set<int> listMsqId;

CMessageHandler::CMessageHandler() :
		msqid(-1), m_nEvent(-1)
{
	buf_length = sizeof(struct MESSAGE_BUF) - sizeof(long);
}

CMessageHandler::~CMessageHandler()
{

}

void CMessageHandler::close()
{
	if((listMsqId.end() == listMsqId.find(msqid)) || 0 >= msqid)
		return;

	if(msgctl(msqid, IPC_RMID, NULL) == -1)
	{
		perror("msgctl");
	}
	else
	{
		listMsqId.erase(msqid);
	}

	_log("[CMessageHandler] message queue close");
}

int CMessageHandler::init(const long lkey)
{
	int nMsqid;
	int msgflg = IPC_CREAT | 0666;

	if(0 >= lkey)
	{
		nMsqid = -1;
	}
	else
	{
		nMsqid = msgget(lkey, msgflg);

		if(0 >= nMsqid)
		{
			_log("[CMessageHandler] message queue init fail, msqid=%d error=%s errorno=%d", nMsqid, strerror(errno),
			errno);
		}
		else
		{
			/**
			 * config msq
			 */
			struct msqid_ds ds;

			memset(&ds, 0, sizeof(struct msqid_ds));
			if(msgctl(nMsqid, IPC_STAT, &ds))
			{
				_log("[CMessageHandler] message queue control fail, msqid=%d error=%s errorno=%d", nMsqid,
						strerror(errno),
						errno);
			}
			else
			{
				//	_DBG("[Message] Queue size = %lu", ds.msg_qbytes);
				ds.msg_qbytes = 1024 * 1024 * 8;
				if(msgctl(nMsqid, IPC_SET, &ds))
				{
					_log("[CMessageHandler] message queue control fail, msqid=%d error=%s errorno=%d", nMsqid,
							strerror(errno),
							errno);
				}
			}
		}
	}

	listMsqId.insert(nMsqid);
	setMsqid(nMsqid);

	return nMsqid;

}

int CMessageHandler::sendMessage(long lFilter, int nCommand, unsigned long int nId, int nDataLen, const void* pData)
{
	int nRet;
	MESSAGE_BUF pBuf;
	const void *pbuf = &pBuf;

	pBuf.clear();
	pBuf.lFilter = lFilter;
	pBuf.nCommand = nCommand;
	pBuf.nId = nId;

	if( NULL != pData && 0 < nDataLen)
	{
		memcpy(pBuf.cData, pData, nDataLen);
		pBuf.nDataLen = nDataLen;
	}

	if(-1 == msgsnd(getMsqid(), pbuf, getBufLength(), 0))
	{
		_log("[CMessageHandler] message queue send fail, msqid=%d error=%s errorno=%d", getMsqid(), strerror(errno),
		errno);
		nRet = -1;
	}
	else
	{
		nRet = getBufLength();
	}

	return nRet;
}

int CMessageHandler::sendMessage(long lFilter, int nCommand, unsigned long int nId)
{
	int nRet;
	MESSAGE_BUF pBuf;
	const void *pbuf = &pBuf;

	pBuf.clear();
	pBuf.lFilter = lFilter;
	pBuf.nCommand = nCommand;
	pBuf.nId = nId;

	if(-1 == msgsnd(getMsqid(), pbuf, getBufLength(), 0))
	{
		_log("[CMessageHandler] message queue send fail, msqid=%d error=%s errorno=%d", getMsqid(), strerror(errno),
		errno);
		nRet = -1;
	}
	else
	{
		nRet = getBufLength();
	}

	return nRet;
}

int CMessageHandler::sendMessage(long lFilter, int nCommand, unsigned long int nId, Message &message)
{
	int nRet;
	MESSAGE_BUF pBuf;
	const void *pbuf = &pBuf;

	pBuf.clear();
	pBuf.lFilter = lFilter;
	pBuf.nCommand = nCommand;
	pBuf.nId = nId;

	//====== Handle Message ========//
	pBuf.what = message.what;
	for(int i = 0; i < ARG_LEN; ++i)
	{
		pBuf.arg[i] = message.arg[i];
	}

	if(DATA_LEN >= message.strData.length())
	{
		pBuf.nDataLen = message.strData.length();
		memcpy(pBuf.cData, message.strData.c_str(), message.strData.length());
	}
	else
	{
		pBuf.nDataLen = 0;
	}

	if(-1 == msgsnd(getMsqid(), pbuf, getBufLength(), 0))
	{
		_log("[CMessageHandler] message queue send fail (Message), msqid=%d error=%s errorno=%d", getMsqid(),
				strerror(errno),
				errno);
		nRet = -1;
	}
	else
	{
		nRet = getBufLength();
	}

	return nRet;
}

int CMessageHandler::recvMessage(void **pbuf)
{
	ssize_t recvSize = 0;

	if( NULL == *pbuf)
		return -1;

	if(-1 == getRecvEvent())
	{
		_log("[CMessageHandler] message queue receive fail, msqid=%d invalid event", getMsqid());
		return -1;
	}

	recvSize = msgrcv(getMsqid(), *pbuf, getBufLength(), getRecvEvent(), 0);

	if(0 > recvSize)
	{
		if( errno == EINTR)
		{
			_log("[CMessageHandler] message queue receive fail get EINTR, msqid=%d error=%s errorno=%d", getMsqid(),
					strerror(errno),
					errno);
			return -2;
		}
		return -1;
	}

	return recvSize;
}

void CMessageHandler::setRecvEvent(int nEvent)
{
	m_nEvent = nEvent;
}

int CMessageHandler::getRecvEvent() const
{
	return m_nEvent;
}

void CMessageHandler::setMsqid(int nId)
{
	msqid = nId;
}

int CMessageHandler::getMsqid() const
{
	return msqid;
}

int CMessageHandler::getBufLength() const
{
	return buf_length;
}

//************* Static Function **************//

int CMessageHandler::registerMsq(const long lkey)
{
	int nMsqid;
	int msgflg = IPC_CREAT | 0666;

	if(0 >= lkey)
	{
		nMsqid = -1;
	}
	else
	{
		nMsqid = msgget(lkey, msgflg);

		if(0 >= nMsqid && 17 != errno)
		{
			_log("[CMessageHandler] message queue init fail, msqid=%d error=%s errorno=%d", nMsqid, strerror(errno),
			errno);
		}
	}

	return nMsqid;
}

void CMessageHandler::closeMsg(const int nMsqId)
{
	if(-1 != msgctl(nMsqId, IPC_RMID, NULL))
	{
		if((listMsqId.end() != listMsqId.find(nMsqId)))
		{
			listMsqId.erase(nMsqId);
		}
		_log("[CMessageHandler] Message Queue Close, id: %d", nMsqId);

	}
}

void CMessageHandler::release()
{
	if(listMsqId.empty())
		return;

	set<int>::iterator it;
	for(it = listMsqId.begin(); listMsqId.end() != it; ++it)
	{
		if(-1 != msgctl(*it, IPC_RMID, NULL))
			_log("[CMessageHandler] Message Queue Close, id: %d", *it);
	}

	listMsqId.clear();
}
