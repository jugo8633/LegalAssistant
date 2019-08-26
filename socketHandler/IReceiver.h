#pragma once

/**
 *  Declare socket client receive
 */
extern int ClientReceive(int nSocketFD, int nDataLen, const void *pData);

/**
 *  Declare socket server receive
 */
extern int ServerReceive(int nSocketFD, int nDataLen, const void *pData);

/** 定義封包格式 **/
typedef enum _PACKET_TYPE
{
	PK_BYTE = 0, PK_CMP, PK_TYPE_SIZE
} PACKET_TYPE;

/** 定義封包處理模式 **/
typedef enum _PACKET_HANDLE
{
	PK_MSQ = 0, PK_ASYNC, PK_HANDLE_SIZE
} PACKET_HANDLE;
