#include "log.h"

#include <dbghelp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#pragma comment(lib, "DbgHelp.lib")

#include "../../lib/mice/config.h"
#include "../hooks/files.h"
#include "../hooks/logging.h"

extern WCHAR exeName[MAX_PATH + 1];
extern DWORD imageOffset;

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

static HANDLE log_file = NULL;
VOID trace_hook(char* output);
CRITICAL_SECTION logger_lock;

static char* log_colours[] = {
    "",          // Always
    "\033[96m",  // Game
    "\033[91m",  // Error
    "\033[33m",  // Warning
    "\033[97m",  // Info
    "\033[90m",  // Misc
    "\033[90m",  // Trace
};
static const char* COLOR_RESET = "\033[0m";
#define LOG_PREFIXES "!GEWIMT"

void logcb(LPCSTR param_1) { log_game("amLog", param_1); }
void __stdcall amLogCallback(DWORD level, char* format) {
    if (level == 0)
        log_game("amLog:E", format);
    else if (level == 0)
        log_game("amLog:W", format);
    else
        log_game("amLog:I", format);
}

DWORD pLogcb;
DWORD* ppLogcb;

static char log_buf[1024];
int _do_log(BYTE log_level, const char* caller, const char* format, va_list args) {
    // TODO: These are all horrible bodges
    if (wcscmp(exeName, L"mxnetwork.exe") == 0) {
        // *((DWORD*)(imageOffset + 0x004438e8)) = (DWORD)(&logcb);
        *((DWORD*)(imageOffset + 0x004438e8)) = 0x00000000;
    }
    if (wcscmp(exeName, L"maimai_dump_.exe") == 0) {
        *((DWORD*)(imageOffset + 0x00c820ec)) = 0x00000001;
        pLogcb = (DWORD)(&amLogCallback);
        ppLogcb = &pLogcb;
        *((DWORD***)(imageOffset + 0x00c820F4)) = &ppLogcb;
        // *((DWORD*)(imageOffset + 0x004438e8)) = (DWORD)(&logcb);
    }

    char prefix = LOG_PREFIXES[log_level];

    EnterCriticalSection(&logger_lock);

    int col_len = strlen(log_colours[log_level]);

    int log_len = snprintf(log_buf, _countof(log_buf), "%s%s%c:%s:", log_colours[log_level],
                           log_prelude(), prefix, caller);
    log_len += vsnprintf(log_buf + log_len, _countof(log_buf) - log_len, format, args);
    log_len += snprintf(log_buf + log_len, _countof(log_buf) - log_len, "%s\n", COLOR_RESET);
    log_buf[_countof(log_buf) - 1] = '\0';

    if (MiceConfig.mice.log_level >= log_level) {
        HANDLE sout = GetStdHandle(STD_OUTPUT_HANDLE);
        _WriteFile(sout, log_buf, log_len, NULL, NULL);
        // FlushFileBuffers(sout);
    }

    if (MiceConfig.mice.log_to_file) {
        if (log_file && log_file != INVALID_HANDLE_VALUE) {
            // Replace the colour reset with a newline, then skip the prefix when writing
            log_buf[log_len - col_len] = '\n';
            log_buf[log_len - col_len + 1] = '\0';
            _WriteFile(log_file, log_buf + col_len, log_len - col_len - sizeof(COLOR_RESET), NULL, NULL);
        }
    }

    LeaveCriticalSection(&logger_lock);
    return log_len;
}
int vlog_trace(const char* caller, const char* format, va_list args) {
    return _do_log(LOG_TRACE, caller, format, args);
}
int _log_trace(const char* caller, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vlog_trace(caller, format, args);
    va_end(args);
    return ret;
}
int vlog_misc(const char* caller, const char* format, va_list args) {
    return _do_log(LOG_MISC, caller, format, args);
}
int _log_misc(const char* caller, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vlog_misc(caller, format, args);
    va_end(args);
    return ret;
}
int vlog_info(const char* caller, const char* format, va_list args) {
    return _do_log(LOG_INFO, caller, format, args);
}
int _log_info(const char* caller, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vlog_info(caller, format, args);
    va_end(args);
    return ret;
}
int vlog_warning(const char* caller, const char* format, va_list args) {
    return _do_log(LOG_WARNING, caller, format, args);
}
int _log_warning(const char* caller, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vlog_warning(caller, format, args);
    va_end(args);
    return ret;
}
int vlog_error(const char* caller, const char* format, va_list args) {
    return _do_log(LOG_ERROR, caller, format, args);
}
int _log_error(const char* caller, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vlog_error(caller, format, args);
    va_end(args);
    return ret;
}
int vlog_game(const char* caller, const char* format, va_list args) {
    return _do_log(LOG_GAME, caller, format, args);
}
int _log_game(const char* caller, const char* format, ...) {
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
    // AttachConsole(ATTACH_PARENT_PROCESS);

    // Enable colour in CMD
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hConsole, &dwMode))
        HAS_COLOUR = SetConsoleMode(hConsole, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    InitializeCriticalSection(&logger_lock);

    if (MiceConfig.mice.log_to_file) {
        if (log_file == NULL && MiceConfig.mice.log_file[0] != '\0')
            log_file = CreateFileA(MiceConfig.mice.log_file, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                                   CREATE_ALWAYS, 0, NULL);
    }
}

void log_stack(const char* caller) {
    char name[MAX_PATH * sizeof(TCHAR)];
    char Storage[sizeof(IMAGEHLP_SYMBOL64) + sizeof(name)];

    IMAGEHLP_SYMBOL64* symbol = (IMAGEHLP_SYMBOL64*)Storage;
    CONTEXT context;
    RtlCaptureContext(&context);

    STACKFRAME64 stack = { 0 };
    stack.AddrPC.Offset = context.Eip;
    stack.AddrPC.Mode = AddrModeFlat;
    stack.AddrStack.Offset = context.Esp;
    stack.AddrStack.Mode = AddrModeFlat;
    stack.AddrFrame.Offset = context.Ebp;
    stack.AddrFrame.Mode = AddrModeFlat;

    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();
    BOOL initres = SymInitialize(process, NULL, true);
    for (ULONG frame = 0;; frame++) {
        BOOL result = StackWalk64(IMAGE_FILE_MACHINE_I386, process, thread, &stack, &context, NULL,
                                  SymFunctionTableAccess64, SymGetModuleBase64, NULL);

        symbol->SizeOfStruct = sizeof(Storage);
        symbol->MaxNameLength = sizeof(name);

        DWORD64 displacement;
        SymGetSymFromAddr64(process, (ULONG64)stack.AddrPC.Offset, &displacement, symbol);
        UnDecorateSymbolName(symbol->Name, (PSTR)name, sizeof(name), UNDNAME_COMPLETE);

        log_error(caller, "%02u called from 0x%08X STACK=0x%08X FRAME=0x%08X  %s\n", frame,
                  (ULONG64)stack.AddrPC.Offset, (ULONG64)stack.AddrStack.Offset,
                  (ULONG64)stack.AddrFrame.Offset, symbol->Name);

        if (result == FALSE) {
            DWORD frameError = GetLastError();
            break;
        }
    }
}
