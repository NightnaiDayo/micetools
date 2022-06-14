#include "log.h"

BOOL HAS_COLOUR = FALSE;

char _log_prelude[32];
char* log_prelude() {
    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(_log_prelude, 32, "[%Y/%m/%d %H:%M:%S] ", timeinfo);
    return _log_prelude;
}

HANDLE LOG_FILE = NULL;
VOID trace_hook(char* output);
CRITICAL_SECTION logger_lock;

int _do_log(const char* caller, const char* level, const char* format, va_list args, bool toStdout) {
    EnterCriticalSection(&logger_lock);

    int len = snprintf(NULL, 0, "%s%s:%s:", log_prelude(), level, caller) + vsnprintf(NULL, 0, format, args);
    char* buf = (char*)malloc(len + 2);
    if (!buf) {
        LeaveCriticalSection(&logger_lock);
        return 0;
    }

    int wrote_a = snprintf(buf, len, "%s%s:%s:", log_prelude(), level, caller);
    int wrote_b = vsnprintf(buf + wrote_a, len - wrote_a + 1, format, args);
    buf[len] = '\n';
    buf[len + 1] = '\0';

    // No +1 here to not get the \n
    if (toStdout) WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buf, len, NULL, NULL);
#ifdef LOG_TO_FILE
    if (LOG_FILE && LOG_FILE != INVALID_HANDLE_VALUE) WriteFile(LOG_FILE, buf, len + 1, NULL, NULL);
#endif

    free(buf);

    LeaveCriticalSection(&logger_lock);
    return wrote_b;
}
int vlog_misc(const char* caller, const char* format, va_list args) {
#ifdef LOG_MISC
    if (HAS_COLOUR) WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "\033[90m", 5, NULL, NULL);
    int ret = _do_log(caller, "M", format, args, true);
    puts(HAS_COLOUR ? "\033[0m" : "");
    return ret;
#else
    return _do_log(caller, "M", format, args, false);
#endif
}
int log_misc(const char* caller, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vlog_misc(caller, format, args);
    va_end(args);
    return ret;
}
int vlog_info(const char* caller, const char* format, va_list args) {
#ifdef LOG_INFO
    if (HAS_COLOUR) WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "\033[97m", 5, NULL, NULL);
    int ret = _do_log(caller, "I", format, args, true);
    puts(HAS_COLOUR ? "\033[0m" : "");
    return ret;
#else
    return _do_log(caller, "I", format, args, false);
#endif
}
int log_info(const char* caller, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vlog_info(caller, format, args);
    va_end(args);
    return ret;
}
int vlog_warning(const char* caller, const char* format, va_list args) {
#ifdef LOG_WARNING
    if (HAS_COLOUR) WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "\033[33m", 5, NULL, NULL);
    int ret = _do_log(caller, "W", format, args, true);
    puts(HAS_COLOUR ? "\033[0m" : "");
    return ret;
#else
    return _do_log(caller, "W", format, args, false);
#endif
}
int log_warning(const char* caller, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vlog_warning(caller, format, args);
    va_end(args);
    return ret;
}
int vlog_error(const char* caller, const char* format, va_list args) {
#ifdef LOG_ERROR
    if (HAS_COLOUR) WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "\033[91m", 5, NULL, NULL);
    int ret = _do_log(caller, "E", format, args, true);
    puts(HAS_COLOUR ? "\033[0m" : "");
    return ret;
#else
    return _do_log(caller, "E", format, args, false);
#endif
}
int log_error(const char* caller, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vlog_error(caller, format, args);
    va_end(args);
    return ret;
}

// char format_buf[1024];  // Will do. We guard against overflow in
// Fake[f]printf int WINAPIV Fakeprintf(const char* _Format, ...) { 	int
// flen = strlen(_Format); 	if (flen == strcspn(_Format, "\n") + 1 && flen
// < (sizeof format_buf)) { 		strcpy_s(format_buf, flen, _Format);
// format_buf[flen - 1] = 0; 		_Format = format_buf;
// 	}
// 	va_list args;
// 	va_start(args, _Format);
// 	int ret = vlog_info("printf", _Format, args);
// 	va_end(args);
// 	return ret;
// };
// int WINAPIV Fakefprintf(FILE* _File, const char* _Format, ...) {
// 	int flen = strlen(_Format);
// 	if (flen == strcspn(_Format, "\n") + 1 && flen < (sizeof format_buf))
// { 		strcpy_s(format_buf, flen, _Format); 		format_buf[flen
// - 1] = 0; 		_Format = format_buf;
// 	}
// 	va_list args;
// 	va_start(args, _Format);
// 	int ret = vlog_error("fprintf", _Format, args);
// 	va_end(args);
// 	return ret;
// };

// int WINAPIV Fakefprintf_s(FILE* _Stream, const char* _Format, ...) {
// 	va_list args;
// 	va_start(args, _Format);
// 	int ret = vlog_error("fprintf_s", _Format, args);
// 	va_end(args);
// 	return ret;
// };

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

HANDLE WINAPI FakeRegisterEventSourceA(LPCSTR lpUNCServerName, LPCSTR lpSourceName) { return (HANDLE)0xDEADBEEF; }

BOOL WINAPI FakeReportEventA(HANDLE hEventLog, WORD wType, WORD wCategory, DWORD dwEventID, PSID lpUserSid,
                             WORD wNumStrings, DWORD dwDataSize, LPCSTR* lpStrings, LPVOID lpRawData) {
    switch (wType) {
        case EVENTLOG_SUCCESS:
        case EVENTLOG_AUDIT_SUCCESS:
            for (int i = 0; i < wNumStrings; i++) log_misc("evtlog", trim_string((char*)lpStrings[i]));
            break;
        case EVENTLOG_AUDIT_FAILURE:
        case EVENTLOG_ERROR_TYPE:
            for (int i = 0; i < wNumStrings; i++) log_error("evtlog", trim_string((char*)lpStrings[i]));
            break;
        case EVENTLOG_WARNING_TYPE:
            for (int i = 0; i < wNumStrings; i++) log_warning("evtlog", trim_string((char*)lpStrings[i]));
            break;
        case EVENTLOG_INFORMATION_TYPE:
        default:
            for (int i = 0; i < wNumStrings; i++) log_info("evtlog", trim_string((char*)lpStrings[i]));
            break;
    }
    return TRUE;
};

BOOL WINAPI FakeDeregisterEventSource(HANDLE hEventLog) { return TRUE; }

static VOID(WINAPI* TrueOutputDebugStringA)(LPCSTR lpOutputString);
VOID WINAPI FakeOutputDebugStringA(LPCSTR lpOutputString) { log_info("debug", "%s", lpOutputString); }

VOID trace_hook(char* output) {
    output[strcspn(output, "\n")] = 0;
    log_error("trace", output);
}

void setup_logging() {
    // Enable colour in CMD
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hConsole, &dwMode))
        HAS_COLOUR = SetConsoleMode(hConsole, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    InitializeCriticalSection(&logger_lock);

    if (LOG_FILE == NULL)
        LOG_FILE = CreateFileA("log.txt", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);

    hook("Advapi32.dll", "RegisterEventSourceA", FakeRegisterEventSourceA, &TrueRegisterEventSourceA);
    hook("Advapi32.dll", "ReportEventA", FakeReportEventA, &TrueReportEventA);
    hook("Advapi32.dll", "DeregisterEventSource", FakeDeregisterEventSource, &TrueDeregisterEventSource);
}
