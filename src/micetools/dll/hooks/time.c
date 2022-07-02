#include "time.h"

SYSTEMTIME localTime;
BOOL ltCache = FALSE;
SYSTEMTIME systemTime;
BOOL stCache = FALSE;

BOOL WINAPI Fake_SetLocalTime(const SYSTEMTIME* lpSystemTime) {
    memcpy(&localTime, lpSystemTime, sizeof localTime);
    ltCache = TRUE;

    log_info("time", "Not setting local time to: %04d-%02d-%02d %02d:%02d:%02d.%04d", lpSystemTime->wYear,
             lpSystemTime->wMonth, lpSystemTime->wDay, lpSystemTime->wHour, lpSystemTime->wMinute,
             lpSystemTime->wSecond, lpSystemTime->wMilliseconds);
    return TRUE;
}
BOOL WINAPI Fake_SetSystemTime(const SYSTEMTIME* lpSystemTime) {
    memcpy(&systemTime, lpSystemTime, sizeof systemTime);
    stCache = TRUE;

    log_info("time", "Not setting system time to: %04d-%02d-%02d %02d:%02d:%02d.%04d", lpSystemTime->wYear,
             lpSystemTime->wMonth, lpSystemTime->wDay, lpSystemTime->wHour, lpSystemTime->wMinute,
             lpSystemTime->wSecond, lpSystemTime->wMilliseconds);
    return TRUE;
}

// TODO: Store deltas instead
BOOL WINAPI Fake_GetLocalTime(SYSTEMTIME* lpSystemTime) {
    if (ltCache) {
        memcpy(lpSystemTime, &localTime, sizeof localTime);
        return TRUE;
    }
    return TrueGetLocalTime(lpSystemTime);
}
BOOL WINAPI Fake_GetSystemTime(SYSTEMTIME* lpSystemTime) {
    if (stCache) {
        memcpy(lpSystemTime, &systemTime, sizeof systemTime);
        return TRUE;
    }
    return TrueGetSystemTime(lpSystemTime);
}

void hook_time() {
    hook("Kernel32.dll", "SetLocalTime", Fake_SetLocalTime, (void**)&TrueSetLocalTime, 6);
    hook("Kernel32.dll", "SetSystemTime", Fake_SetSystemTime, (void**)&TrueSetSystemTime, 6);
    hook("Kernel32.dll", "GetLocalTime", Fake_GetLocalTime, (void**)&TrueGetLocalTime, 6);
    hook("Kernel32.dll", "GetSystemTime", Fake_GetSystemTime, (void**)&TrueGetSystemTime, 6);
}
