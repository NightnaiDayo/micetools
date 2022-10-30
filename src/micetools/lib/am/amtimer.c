#include <amTimer.h>

int frequency_loaded = 0;
LARGE_INTEGER frequency;

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
