#pragma once

#include <Windows.h>

typedef struct amtime {
    unsigned int seconds;
    unsigned int microseconds;
} amtime_t;

amtime_t* amiTimerGet(amtime_t* time);

#define _amTimeMs(time) (((time).microseconds / 1000 + (time).seconds * 1000))
#define _amTimeDelta(now, start) \
    (((now).microseconds - (start).microseconds) / 1000 + ((now).seconds - (start).seconds) * 1000)

#define _amTimeDeltaMircos(now, start) \
    ((((now).seconds - (start).seconds) * 1000000 - (start).microseconds) + (now).microseconds)
