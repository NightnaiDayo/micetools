#include "logging.h"

#include "../util/_util.h"

char* trim_string(char* string) {
    size_t len = strlen(string) - 1;

    DWORD oldProt;
    while (len > 0 && (string[len] == '\n' || string[len] == '\r')) {
        // TODO: Reassess this. Suspect it may be causing issues.
        // Make sure we can write! This is a terrible hack, but it does work.
        VirtualProtect(string + len, 1, PAGE_EXECUTE_READWRITE, &oldProt);
        string[len--] = '\0';
        VirtualProtect(string + len + 1, 1, oldProt, &oldProt);
    }
    return string;
}

#define WORK_FORMAT_MAX 1024
char format_buf[WORK_FORMAT_MAX];  // Will do. We guard against overflow in Fake[f]printf

int WINAPIV Fakeprintf(const char* _Format, ...) {
    size_t flen = strlen(_Format);
    if (flen == strcspn(_Format, "\n") + 1 && flen < (sizeof format_buf)) {
        strcpy_s(format_buf, WORK_FORMAT_MAX, _Format);
        format_buf[flen - 1] = 0;
        _Format = format_buf;
    }
    va_list args;
    va_start(args, _Format);

    int ret = vlog_game(plfPrintf, _Format, args);

    va_end(args);
    return ret;
}

int WINAPIV Fakefprintf(FILE* _File, const char* _Format, ...) {
    size_t flen = strlen(_Format);
    if (flen == strcspn(_Format, "\n") + 1 && flen < (sizeof format_buf)) {
        strcpy_s(format_buf, WORK_FORMAT_MAX, _Format);
        format_buf[flen - 1] = 0;
        _Format = format_buf;
    }
    va_list args;
    va_start(args, _Format);

    int ret = vlog_game(plfFprintf, _Format, args);

    va_end(args);
    return ret;
}

int WINAPIV Fakefprintf_s(FILE* _Stream, const char* _Format, ...) {
    va_list args;
    va_start(args, _Format);

    int ret = vlog_game(plfFprintf_s, _Format, args);

    va_end(args);
    return ret;
}
int WINAPIV Fakevfprintf_s(FILE* _Stream, const char* _Format, va_list _ArgList) {
    return vlog_game(plfVfprintf_s, _Format, _ArgList);
}
HANDLE WINAPI FakeRegisterEventSourceA(LPCSTR lpUNCServerName, LPCSTR lpSourceName) {
    return (HANDLE)0xDEADBEEF;
}

BOOL WINAPI FakeReportEventA(HANDLE hEventLog, WORD wType, WORD wCategory, DWORD dwEventID,
                             PSID lpUserSid, WORD wNumStrings, DWORD dwDataSize, LPCSTR* lpStrings,
                             LPVOID lpRawData) {
    switch (wType) {
        case EVENTLOG_SUCCESS:
        case EVENTLOG_AUDIT_SUCCESS:
            for (int i = 0; i < wNumStrings; i++)
                log_misc(plfEvtlog, trim_string((char*)lpStrings[i]));
            break;
        case EVENTLOG_AUDIT_FAILURE:
        case EVENTLOG_ERROR_TYPE:
            for (int i = 0; i < wNumStrings; i++)
                log_error(plfEvtlog, trim_string((char*)lpStrings[i]));
            break;
        case EVENTLOG_WARNING_TYPE:
            for (int i = 0; i < wNumStrings; i++)
                log_warning(plfEvtlog, trim_string((char*)lpStrings[i]));
            break;
        case EVENTLOG_INFORMATION_TYPE:
        default:
            for (int i = 0; i < wNumStrings; i++)
                log_info(plfEvtlog, trim_string((char*)lpStrings[i]));
            break;
    }
    return TRUE;
};

BOOL WINAPI FakeDeregisterEventSource(HANDLE hEventLog) { return TRUE; }

// static VOID(WINAPI* TrueOutputDebugStringA)(LPCSTR lpOutputString);
// VOID WINAPI FakeOutputDebugStringA(LPCSTR lpOutputString) { log_info("debug", "%s",
// lpOutputString); }

void hook_logging() {
    // hook("MSVCR90.DLL", "printf", Fakeprintf, (void**)&Trueprintf);
    // hook("MSVCR90.DLL", "fprintf", Fakefprintf, (void**)&Truefprintf);
    // hook("MSVCR90.DLL", "fprintf_s", Fakefprintf_s, (void**)&Truefprintf_s);
    // hook("MSVCR90.DLL", "vfprintf_s", Fakevfprintf_s, (void**)&Truevfprintf_s);

    hook("Advapi32.dll", "RegisterEventSourceA", FakeRegisterEventSourceA,
         (void**)&TrueRegisterEventSourceA);
    hook("Advapi32.dll", "ReportEventA", FakeReportEventA, (void**)&TrueReportEventA);
    hook("Advapi32.dll", "DeregisterEventSource", FakeDeregisterEventSource,
         (void**)&TrueDeregisterEventSource);
}
