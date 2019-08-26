/*
 * CMessageHandler.h
 *
 *  Created on: 2014年12月2日
 *      Author: jugo
 */

#pragma once

#include <string>
#include <memory.h>

#define DATA_LEN    4096
#define ARG_LEN		9

typedef struct _Message
{
	int what;
	int arg[ARG_LEN];
	std::string strData;
	void clear()
	{
		what = 0;
		memset(arg, 0, sizeof(arg));
		strData.clear();
	}
} Message;

/*
 * Declare the message structure.
 */
struct MESSAGE_BUF
{
	long lFilter; // Message Filter, Who call CObject::run(long lFilter, const char * szDescript) will callback onReceiveMessage
	int nCommand;
	unsigned long int nId;
	int nDataLen;
	char cData[DATA_LEN];
	int what;
	int arg[ARG_LEN];
	void clear()
	{
		lFilter = 0;
		nCommand = 0;
		nId = 0;
		nDataLen = 0;
		memset(cData, 0, DATA_LEN);
		what = 0;
		memset(arg, 0, sizeof(arg));
	}
};

class CMessageHandler
{
public:
	CMessageHandler();
	virtual ~CMessageHandler();
	void close();
	int init(const long lkey);
	int sendMessage(long lFilter, int nCommand, unsigned long int nId, int nDataLen, const void* pData);
	int sendMessage(long lFilter, int nCommand, unsigned long int nId);
	int sendMessage(long lFilter, int nCommand, unsigned long int nId, Message &message);
	int recvMessage(void **pbuf);
	void setRecvEvent(int nEvent);
	int getRecvEvent() const;
	int getMsqid() const;
	void setMsqid(int nId);
	int getBufLength() const;
	static int registerMsq(const long lkey);
	static void closeMsg(const int nMsqId);
	static void release();

private:
	int msqid;
	int buf_length;
	int m_nEvent;

};

