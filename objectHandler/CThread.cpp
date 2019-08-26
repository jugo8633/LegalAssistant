/*
 * CThread.cpp
 *
 *  Created on: 2017年3月15日
 *      Author: Jugo
 */

#include <stdio.h>
#include <sched.h>
#include <sys/time.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include "CThread.h"
#include "logHandler/LogHandler.h"
#include "global_inc/common.h"

pthread_t _CreateThread(void* (*entry)(void*), void* arg)
{
	pthread_t thd;
	int rc = 0;
	pthread_attr_t attr;
	struct sched_param param;
	int policy = SCHED_FIFO;
	int priority_max = 99;
	//int priority_min = 0;

	pthread_attr_init(&attr);

	/*
	 * set stack size,
	 * use command 'ulimit -s' to show system default value. 'ulimit -a' show all
	 * min value: PTHREAD_STACK_MIN
	 */
	// pthread_attr_setstacksize(&attr, _THREAD_STACK_SIZE_4MB_);
	/**
	 * 設為非分離線程 or 分離線程
	 * PTHREAD_CREATE_DETACHED（分離線程）和 PTHREAD _CREATE_JOINABLE（非分離線程）
	 */
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	/**
	 * 設為綁定的
	 * PTHREAD_SCOPE_SYSTEM（綁定的）和PTHREAD_SCOPE_PROCESS（非綁定的）
	 */
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

	/**
	 * 設定線程的優先級
	 * thread policy: SCHED_FIFO, SCHED_RR 和 SCHED_OTHER
	 */
	pthread_attr_getschedpolicy(&attr, &policy);
	if(SCHED_FIFO != policy)
	{
		policy = SCHED_FIFO;
		pthread_attr_setschedpolicy(&attr, policy);
	}

	//	priority_max = sched_get_priority_max(policy);
	//	priority_min = sched_get_priority_min(policy);

	pthread_attr_getschedparam(&attr, &param);
	param.sched_priority = priority_max;
	pthread_attr_setschedparam(&attr, &param);

	rc = pthread_create(&thd, &attr, entry, arg);

	pthread_attr_destroy(&attr);

	if(0 != rc)
	{
		_log("[_CreateThread] Create Fail");
		return 0;
	}

	return thd;
}

void _ThreadJoin(pthread_t thdid)
{
	if(/*0 > thdid || */(0 != pthread_join(thdid, 0)))
	{
		_log("[_ThreadJoin] Thread Join Fail ID: %lu", thdid);
	}
	_log("[_ThreadJoin] Thread Join Success id: %lu", thdid);
}

void _ThreadExit()
{
	pthread_exit(0);
}

int _ThreadCancel(pthread_t thread)
{
	int kill_rc;
	if(0 >= thread)
		return 0;

	/**
	 *  pthread_kill, signal 0 is to check thread
	 */
	kill_rc = pthread_kill(thread, 0);

	if(kill_rc == ESRCH)
	{
		/**
		 * the specified thread did not exists or already quit
		 */
		_log("[CThread] The specified thread did not exists or already quit");
	}
	else if(kill_rc == EINVAL)
	{
		_log("[CThread] Signal is invalid for pthread_kill");
	}
	else
	{
		_log("[CThread] Thread is valid to cancel, Id=%lu", thread);
		return pthread_cancel(thread);
	}

	return 0;
}

pthread_t _GetThreadID()
{
	return pthread_self();
}
