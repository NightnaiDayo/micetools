#pragma once

#include <Windows.h>

#include "../_am.h"

AM_LIB_H_HEADER(amiTimer, AMI_TIMER)

typedef struct AMI_TIMER {
    unsigned char Rsv00;
} AMI_TIMER;

typedef struct amtime {
    int seconds;
    int microseconds;
} amtime_t;

amtime_t* amiTimerGet(amtime_t* time);

#define _amTimeMs(time) (((time).microseconds / 1000 + (time).seconds * 1000))
#define _amTimeDelta(now, start) \
    (int)(((now).microseconds - (start).microseconds) / 1000 + ((now).seconds - (start).seconds) * 1000)

int amiTimerDiffSec(amtime_t* start, amtime_t* now);
int amiTimerDiffMsec(amtime_t* start, amtime_t* now);
int amiTimerDiffUsec(amtime_t* start, amtime_t* now);
