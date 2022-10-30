#include "log.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../hooks/logging.h"

extern BOOL(WINAPI* TrueWriteFile)(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                                   LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);

BOOL HAS_COLOUR = FALSE;

char _log_prelude[64];
char* log_prelude() {
    time_t rawtime;
    struct tm timeinfo;

    time(&rawtime);
    localtime_s(&timeinfo, &rawtime);

    strftime(_log_prelude, sizeof _log_prelude, "[%Y/%m/%d %H:%M:%S] ", &timeinfo);
    return _log_prelude;
}

HANDLE LOG_FILE = NULL;
VOID trace_hook(char* output);
CRITICAL_SECTION logger_lock;

char* log_colours[] = {
    "",          // Always
    "\033[96m",  // Game
    "\033[91m",  // Error
    "\033[33m",  // Warning
    "\033[97m",  // Info
    "\033[90m",  // Misc
    "\033[90m",  // Trace
};
#define LOG_PREFIXES "!GEWIMT"

void logcb(LPCSTR param_1) { log_error("logcb", param_1); }

extern WCHAR exePath[MAX_PATH + 1];
int _do_log(BYTE log_level, const char* caller, const char* format, va_list args) {
    if (wcscmp(exePath, L"mxnetwork.exe") == 0) {
        // *((DWORD*)(0x004438e8)) = (DWORD)(&logcb);
        *((DWORD*)(0x004438e8)) = 0x00000000;
    }

    char prefix = LOG_PREFIXES[log_level];

    EnterCriticalSection(&logger_lock);
    int len = snprintf(NULL, 0, "%s%c:%s:", log_prelude(), prefix, caller) +
              vsnprintf(NULL, 0, format, args);
    char* buf = (char*)malloc(len + 2);
    if (!buf) {
        LeaveCriticalSection(&logger_lock);
        return 0;
    }

    int wrote_a = snprintf(buf, len, "%s%c:%s:", log_prelude(), prefix, caller);
    int wrote_b = vsnprintf(buf + wrote_a, len - wrote_a + 1, format, args);
    buf[len] = '\n';
    buf[len + 1] = '\0';

    // No +1 here to not get the \n
    if (LOG_LEVEL >= log_level) {
        HANDLE sout = GetStdHandle(STD_OUTPUT_HANDLE);
        if (HAS_COLOUR)
            (TrueWriteFile ? TrueWriteFile : WriteFile)(sout, log_colours[log_level],
                                                        strlen(log_colours[log_level]), NULL, NULL);
        if (sout != INVALID_HANDLE_VALUE)
            (TrueWriteFile ? TrueWriteFile : WriteFile)(sout, buf, len, NULL, NULL);
        puts(HAS_COLOUR ? "\033[0m" : "");
    }
#ifdef LOG_TO_FILE
    if (LOG_FILE && LOG_FILE != INVALID_HANDLE_VALUE)
        (TrueWriteFile ? TrueWriteFile : WriteFile)(LOG_FILE, buf, len + 1, NULL, NULL);
#endif

    free(buf);

    LeaveCriticalSection(&logger_lock);
    return wrote_b;
}
int vlog_trace(const char* caller, const char* format, va_list args) {
    return _do_log(LOG_TRACE, caller, format, args);
}
int log_trace(const char* caller, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vlog_trace(caller, format, args);
    va_end(args);
    return ret;
}
int vlog_misc(const char* caller, const char* format, va_list args) {
    return _do_log(LOG_MISC, caller, format, args);
}
int log_misc(const char* caller, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vlog_misc(caller, format, args);
    va_end(args);
    return ret;
}
int vlog_info(const char* caller, const char* format, va_list args) {
    return _do_log(LOG_INFO, caller, format, args);
}
int log_info(const char* caller, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vlog_info(caller, format, args);
    va_end(args);
    return ret;
}
int vlog_warning(const char* caller, const char* format, va_list args) {
    return _do_log(LOG_WARNING, caller, format, args);
}
int log_warning(const char* caller, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vlog_warning(caller, format, args);
    va_end(args);
    return ret;
}
int vlog_error(const char* caller, const char* format, va_list args) {
    return _do_log(LOG_ERROR, caller, format, args);
}
int log_error(const char* caller, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vlog_error(caller, format, args);
    va_end(args);
    return ret;
}
int vlog_game(const char* caller, const char* format, va_list args) {
    return _do_log(LOG_GAME, caller, format, args);
}
int log_game(const char* caller, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vlog_game(caller, format, args);
    va_end(args);
    return ret;
}

VOID trace_hook(char* output) {
    output[strcspn(output, "\n")] = 0;
    log_error("trace", output);
}

void setup_logging() {
    // Force stdio even for GUI applications
    // TODO: Is there a more robust way to check if we have a proper stdio?
    AttachConsole(ATTACH_PARENT_PROCESS);
    if (GetStdHandle(STD_ERROR_HANDLE) > (HANDLE)0x10) {
        FILE* newstream;
        if (GetStdHandle(STD_INPUT_HANDLE)) {
            freopen_s(&newstream, "CONIN$", "r", stdin);
            setvbuf(stdin, NULL, _IONBF, 0);
        }
        if (GetStdHandle(STD_OUTPUT_HANDLE)) {
            freopen_s(&newstream, "CONOUT$", "w", stdout);
            setvbuf(stdout, NULL, _IONBF, 0);
        }
        if (GetStdHandle(STD_ERROR_HANDLE)) {
            freopen_s(&newstream, "CONOUT$", "w", stderr);
            setvbuf(stderr, NULL, _IONBF, 0);
        }
    }

    // Enable colour in CMD
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hConsole, &dwMode))
        HAS_COLOUR = SetConsoleMode(hConsole, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    InitializeCriticalSection(&logger_lock);

#ifdef LOG_TO_FILE
    if (LOG_FILE == NULL)
        LOG_FILE =
            CreateFileA("log.txt", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
#endif
}
