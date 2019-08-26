/*
 * dataType.h
 *
 *  Created on: 2018年11月9日
 *      Author: jugo
 */

#pragma once

typedef long long __int64;
typedef unsigned long DWORD, *PDWORD, *LPDWORD;
typedef int BOOL, *PBOOL, *LPBOOL;
typedef unsigned char BYTE, *PBYTE, *LPBYTE;
typedef BYTE BOOLEAN, *PBOOLEAN;
typedef wchar_t WCHAR, *PWCHAR;
typedef WCHAR* BSTR;
typedef char CHAR, *PCHAR;
typedef double DOUBLE;
typedef long LONG, *PLONG, *LPLONG;
typedef unsigned int ULONG_PTR;
typedef ULONG_PTR DWORD_PTR;
typedef unsigned int DWORD32;
typedef unsigned long long DWORD64, *PDWORD64;
typedef unsigned long long ULONGLONG;
typedef ULONGLONG DWORDLONG, *PDWORDLONG;
typedef unsigned long error_status_t;
typedef float FLOAT;
typedef void* HANDLE;
typedef DWORD HCALL;
typedef LONG HRESULT;
typedef int INT, *LPINT;
typedef signed char INT8;
typedef signed short INT16;
typedef signed int INT32;
typedef signed long long INT64;
typedef void* LDAP_UDP_HANDLE;
typedef const wchar_t* LMCSTR;
typedef WCHAR* LMSTR;
typedef signed long long LONGLONG;
typedef signed int LONG32;
typedef signed long long LONG64, *PLONG64;
typedef const char* LPCSTR;
typedef const void* LPCVOID;
typedef const wchar_t* LPCWSTR;
typedef char* PSTR, *LPSTR;
typedef wchar_t* LPWSTR, *PWSTR;
typedef DWORD NET_API_STATUS;
typedef long NTSTATUS;
typedef unsigned long long QWORD;
typedef void* RPC_BINDING_HANDLE;
typedef short SHORT;
typedef unsigned int ULONG32;
typedef ULONG_PTR SIZE_T;
typedef unsigned char UCHAR, *PUCHAR;
typedef UCHAR* STRING;
typedef unsigned int UINT;
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;
typedef unsigned long ULONG, *PULONG;
typedef unsigned long long ULONG64;
typedef wchar_t UNICODE;
typedef unsigned short USHORT;
typedef void VOID, *PVOID, *LPVOID;
typedef wchar_t WCHAR, *PWCHAR;
typedef unsigned short WORD, *PWORD, *LPWORD;
typedef const char* LPCTSTR;		// const char* or const wchar_t* depending on _UNICODE

#ifndef NULL
#define NULL	0
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif
