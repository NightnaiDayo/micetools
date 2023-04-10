#include "time.h"

SYSTEMTIME localTime;
BOOL ltCache = FALSE;
SYSTEMTIME systemTime;
BOOL stCache = FALSE;

BOOL WINAPI Fake_SetLocalTime(const SYSTEMTIME* lpSystemTime) {
    memcpy(&localTime, lpSystemTime, sizeof localTime);
    ltCache = TRUE;

    log_info(plfTime, "Not setting local time to: %04d-%02d-%02d %02d:%02d:%02d.%04d",
             lpSystemTime->wYear, lpSystemTime->wMonth, lpSystemTime->wDay, lpSystemTime->wHour,
             lpSystemTime->wMinute, lpSystemTime->wSecond, lpSystemTime->wMilliseconds);
    return TRUE;
}
BOOL WINAPI Fake_SetSystemTime(const SYSTEMTIME* lpSystemTime) {
    memcpy(&systemTime, lpSystemTime, sizeof systemTime);
    stCache = TRUE;

    log_info(plfTime, "Not setting system time to: %04d-%02d-%02d %02d:%02d:%02d.%04d",
             lpSystemTime->wYear, lpSystemTime->wMonth, lpSystemTime->wDay, lpSystemTime->wHour,
             lpSystemTime->wMinute, lpSystemTime->wSecond, lpSystemTime->wMilliseconds);
    return TRUE;
}

BOOL WINAPI Fake_SetTimeZoneInformation(const TIME_ZONE_INFORMATION* lpTimeZoneInformation) {
    log_info(plfTime, "Not setting timezone to: %d", lpTimeZoneInformation->Bias);
    return TRUE;
}

// TODO: Store deltas instead
BOOL WINAPI Fake_GetLocalTime(SYSTEMTIME* lpSystemTime) {
    log_trace(plfTime, "GetLocalTime");
    if (ltCache) {
        memcpy(lpSystemTime, &localTime, sizeof localTime);
        return TRUE;
    }

    // lpSystemTime->wYear = 2023;
    // lpSystemTime->wMonth = 2;
    // lpSystemTime->wDayOfWeek = 0;
    // lpSystemTime->wDay = 12;
    // lpSystemTime->wHour = 17;
    // lpSystemTime->wMinute = 29;
    // lpSystemTime->wSecond = 12;
    // lpSystemTime->wMilliseconds = 764;
    // return TRUE;
    return TrueGetLocalTime(lpSystemTime);
}
BOOL WINAPI Fake_GetSystemTime(SYSTEMTIME* lpSystemTime) {
    if (stCache) {
        memcpy(lpSystemTime, &systemTime, sizeof systemTime);
        return TRUE;
    }
    return TrueGetSystemTime(lpSystemTime);

    // lpSystemTime->wYear = 2023;
    // lpSystemTime->wMonth = 2;
    // lpSystemTime->wDayOfWeek = 0;
    // lpSystemTime->wDay = 12;
    // lpSystemTime->wHour = 17;
    // lpSystemTime->wMinute = 29;
    // lpSystemTime->wSecond = 12;
    // lpSystemTime->wMilliseconds = 764;
    // return TRUE;
}

void hook_time() {
    hook("Kernel32.dll", "SetLocalTime", Fake_SetLocalTime, NULL);
    hook("Kernel32.dll", "SetSystemTime", Fake_SetSystemTime, NULL);
    hook("Kernel32.dll", "SetTimeZoneInformation", Fake_SetTimeZoneInformation, NULL);
    hook("Kernel32.dll", "GetLocalTime", Fake_GetLocalTime, (void**)&TrueGetLocalTime);
    hook("Kernel32.dll", "GetSystemTime", Fake_GetSystemTime, (void**)&TrueGetSystemTime);
}
