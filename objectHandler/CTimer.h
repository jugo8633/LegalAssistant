/*
 * CTimer.h
 *
 *  Created on: 2016年12月19日
 *      Author: Jugo
 */

#pragma once

typedef void (*TimerCBFun)(int param);

/**
 *   nId: callback id
 *   nSec: first run at nSec seconds
 *   nInterSec: every time run
 *   TimerCBFun: callback function
 */
extern timer_t _SetTimer(int nId, int nSec, int nInterSec, TimerCBFun);
extern void _KillTimer(int nId);
