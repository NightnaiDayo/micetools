#pragma once

#include <Windows.h>
#include <stdio.h>

#define LOG_GAME 1
#define LOG_ERROR 2
#define LOG_WARNING 3
#define LOG_INFO 4
#define LOG_MISC 5
#define LOG_TRACE 6

#define COMM_LOGGER "comm"
#define HOOKS_LOGGER "hooks"
#define BOOT_LOGGER "boot"

extern CRITICAL_SECTION logger_lock;

int _log_trace(const char* caller, const char* format, ...);
int _log_misc(const char* caller, const char* format, ...);
int _log_info(const char* caller, const char* format, ...);
int _log_warning(const char* caller, const char* format, ...);
int _log_error(const char* caller, const char* format, ...);
int _log_game(const char* caller, const char* format, ...);

int vlog_trace(const char* caller, const char* format, va_list args);
int vlog_misc(const char* caller, const char* format, va_list args);
int vlog_info(const char* caller, const char* format, va_list args);
int vlog_warning(const char* caller, const char* format, va_list args);
int vlog_error(const char* caller, const char* format, va_list args);
int vlog_game(const char* caller, const char* format, va_list args);

void log_stack(const char* caller);

void setup_logging();

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
