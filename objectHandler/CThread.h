/*
 * CThread.h
 *
 *  Created on: 2017年3月15日
 *      Author: Jugo
 */

#pragma once

#include <pthread.h>

extern pthread_t _CreateThread(void* (*entry)(void*), void* arg);
extern void _ThreadJoin(pthread_t thdid);
extern void _ThreadExit();
extern int _ThreadCancel(pthread_t thread);
extern pthread_t _GetThreadID();
