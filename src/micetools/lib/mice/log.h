#pragma once

#include <Windows.h>
#include <stdio.h>

#define LOG_GAME 1
#define LOG_ERROR 2
#define LOG_WARNING 3
#define LOG_INFO 4
#define LOG_MISC 5
#define LOG_TRACE 6

typedef struct {
    char* m_name;
} LOG_FACILITY, *PLOG_FACILITY;

#define _LF(category, name, display) extern PLOG_FACILITY plf##name;
#include "log_facilities.def"
#undef _LF

extern PLOG_FACILITY plfNetwork;

extern CRITICAL_SECTION logger_lock;

int _log_trace(PLOG_FACILITY facility, const char* format, ...);
int _log_misc(PLOG_FACILITY facility, const char* format, ...);
int _log_info(PLOG_FACILITY facility, const char* format, ...);
int _log_warning(PLOG_FACILITY facility, const char* format, ...);
int _log_error(PLOG_FACILITY facility, const char* format, ...);
int _log_game(PLOG_FACILITY facility, const char* format, ...);

int vlog_trace(PLOG_FACILITY facility, const char* format, va_list args);
int vlog_misc(PLOG_FACILITY facility, const char* format, va_list args);
int vlog_info(PLOG_FACILITY facility, const char* format, va_list args);
int vlog_warning(PLOG_FACILITY facility, const char* format, va_list args);
int vlog_error(PLOG_FACILITY facility, const char* format, va_list args);
int vlog_game(PLOG_FACILITY facility, const char* format, va_list args);

void log_stack(PLOG_FACILITY facility);

void setup_logging(DWORD slotPid);

// Disable some logging entirely at build time for speed
#define COMPILE_LOG_LEVEL 6

#if COMPILE_LOG_LEVEL >= 6
#define log_trace _log_trace
#else
#define log_trace(...)
#endif

#if COMPILE_LOG_LEVEL >= 5
#define log_misc _log_misc
#else
#define log_misc(...)
#endif

#if COMPILE_LOG_LEVEL >= 4
#define log_info _log_info
#else
#define log_info(...)
#endif

#if COMPILE_LOG_LEVEL >= 3
#define log_warning _log_warning
#else
#define log_warning(...)
#endif

#if COMPILE_LOG_LEVEL >= 2
#define log_error _log_error
#else
#define log_error(...)
#endif

#if COMPILE_LOG_LEVEL >= 1
#define log_game _log_game
#else
#define log_game(...)
#endif
