#include "amiTimer.h"

int frequency_loaded = 0;
LARGE_INTEGER frequency;

AM_LIB_C_HEADER(amiTimer, AMI_TIMER)

amtime_t* amiTimerGet(amtime_t* time) {
    LARGE_INTEGER counter;

    if (time == NULL) return NULL;

    if (frequency_loaded == 0) {
        QueryPerformanceFrequency(&frequency);
        frequency_loaded = 1;
    }
    QueryPerformanceCounter(&counter);
    time->microseconds = (unsigned int)((counter.QuadPart * 1000000) / frequency.QuadPart);
    time->seconds = (unsigned int)(counter.QuadPart / frequency.QuadPart);
    return time;
}

int amiTimerDiffSec(amtime_t* start, amtime_t* now) {
    if (start == NULL || now == NULL) return -1;

    return (now->seconds - (now->microseconds < start->microseconds)) - start->seconds;
}

int amiTimerDiffMsec(amtime_t* start, amtime_t* now) {
    if (start == NULL || now == NULL) return -1;

    return (now->microseconds - start->microseconds) / 1000 +
           (now->seconds - start->seconds) * 1000;
}

int amiTimerDiffUsec(amtime_t* start, amtime_t* now) {
    if (start == NULL || now == NULL) return -1;

    return ((now->seconds - start->seconds) * 1000000 - start->microseconds) + now->microseconds;
}
