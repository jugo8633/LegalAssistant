/*
 * CTimer.cpp
 *
 *  Created on: 2016年12月19日
 *      Author: Jugo
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "CTimer.h"
#include "logHandler/LogHandler.h"
#include <map>

using namespace std;

map<int, TimerCBFun> mapCBF;
map<int, timer_t> mapTimerId;

void handle(union sigval v)
{
	if (mapCBF.find(v.sival_int) != mapCBF.end())
	{
		(*mapCBF[v.sival_int])(v.sival_int);
	}
}

timer_t _SetTimer(int nId, int nSec, int nInterSec, TimerCBFun tcbf)
{

	_KillTimer(nId);

	struct sigevent evp;
	struct itimerspec ts;
	timer_t timerid;

	memset(&evp, 0, sizeof(evp));

	evp.sigev_value.sival_ptr = &timerid;
	evp.sigev_notify = SIGEV_THREAD;
	evp.sigev_notify_function = handle;
	evp.sigev_value.sival_int = nId;

	if (-1 == timer_create(CLOCK_REALTIME, &evp, &timerid))
	{
		_log("[Timer] timer_create fail\n");
		return 0;
	}

	mapCBF[nId] = tcbf;
	mapTimerId[nId] = timerid;

	ts.it_interval.tv_sec = nInterSec; // after first start then every time due.
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = nSec; // first stat
	ts.it_value.tv_nsec = 0;

	if (-1 == timer_settime(timerid, 0, &ts, NULL))
	{
		if (mapCBF.find(nId) != mapCBF.end())
			mapCBF.erase(nId);
		if (mapTimerId.find(nId) != mapTimerId.end())
			mapTimerId.erase(nId);
		_log("[Timer] timer_settime fail");
		return 0;
	}

	printf("[Timer] Timer Create Success Id = %ld\n", (long) mapTimerId[nId]);
	return mapTimerId[nId];
}

void _KillTimer(int nId)
{
	if (mapCBF.find(nId) != mapCBF.end())
	{
		mapCBF.erase(nId);
	}

	if (mapTimerId.find(nId) != mapTimerId.end())
	{
		struct itimerspec curr_value;
		if (-1 != timer_gettime(mapTimerId[nId], &curr_value))
		{
			timer_delete(mapTimerId[nId]);
			printf("[Timer] Kill timer id = %ld\n", (long) mapTimerId[nId]);
		}

		mapTimerId.erase(nId);
	}
}

