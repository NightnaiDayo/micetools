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

int _do_log(const char* caller, const char* level, const char* format, va_list args, bool toStdout, char* colour) {
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
    if (toStdout) {
        HANDLE sout = GetStdHandle(STD_OUTPUT_HANDLE);
        if (HAS_COLOUR) (TrueWriteFile ? TrueWriteFile : WriteFile)(sout, colour, strlen(colour), NULL, NULL);
        if (sout != INVALID_HANDLE_VALUE) (TrueWriteFile ? TrueWriteFile : WriteFile)(sout, buf, len, NULL, NULL);
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
int vlog_misc(const char* caller, const char* format, va_list args) {
    return _do_log(caller, "M", format, args, LOG_MISC, "\033[90m");
}
int log_misc(const char* caller, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vlog_misc(caller, format, args);
    va_end(args);
    return ret;
}
int vlog_info(const char* caller, const char* format, va_list args) {
    return _do_log(caller, "I", format, args, LOG_INFO, "\033[97m");
}
int log_info(const char* caller, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vlog_info(caller, format, args);
    va_end(args);
    return ret;
}
int vlog_warning(const char* caller, const char* format, va_list args) {
    return _do_log(caller, "W", format, args, LOG_WARNING, "\033[33m");
}
int log_warning(const char* caller, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vlog_warning(caller, format, args);
    va_end(args);
    return ret;
}
int vlog_error(const char* caller, const char* format, va_list args) {
    return _do_log(caller, "E", format, args, LOG_ERROR, "\033[91m");
}
int log_error(const char* caller, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vlog_error(caller, format, args);
    va_end(args);
    return ret;
}
int vlog_game(const char* caller, const char* format, va_list args) {
    return _do_log(caller, "G", format, args, LOG_GAME, "\033[96m");
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
    AttachConsole(ATTACH_PARENT_PROCESS);
    if (GetStdHandle(STD_INPUT_HANDLE)) {
        freopen("CONIN$", "r", stdin);
        setvbuf(stdin, NULL, _IONBF, 0);
    }
    if (GetStdHandle(STD_OUTPUT_HANDLE)) {
        freopen("CONOUT$", "w", stdout);
        setvbuf(stdout, NULL, _IONBF, 0);
    }
    if (GetStdHandle(STD_ERROR_HANDLE)) {
        freopen("CONOUT$", "w", stderr);
        setvbuf(stderr, NULL, _IONBF, 0);
    }

    // Enable colour in CMD
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hConsole, &dwMode))
        HAS_COLOUR = SetConsoleMode(hConsole, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    InitializeCriticalSection(&logger_lock);

    if (LOG_FILE == NULL)
        LOG_FILE = CreateFileA("log.txt", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
}
