#pragma once

#include <Windows.h>
#include <stdio.h>

#define LOG_GAME 1
#define LOG_ERROR 2
#define LOG_WARNING 3
#define LOG_INFO 4
#define LOG_MISC 5
#define LOG_TRACE 6

#define LOG_LEVEL LOG_TRACE

// #define LOG_TO_FILE

#define COMM_LOGGER "comm"
#define HOOKS_LOGGER "hooks"
#define BOOT_LOGGER "boot"

extern CRITICAL_SECTION logger_lock;

int log_trace(const char* caller, const char* format, ...);
int log_misc(const char* caller, const char* format, ...);
int log_info(const char* caller, const char* format, ...);
int log_warning(const char* caller, const char* format, ...);
int log_error(const char* caller, const char* format, ...);
int log_game(const char* caller, const char* format, ...);

int vlog_trace(const char* caller, const char* format, va_list args);
int vlog_misc(const char* caller, const char* format, va_list args);
int vlog_info(const char* caller, const char* format, va_list args);
int vlog_warning(const char* caller, const char* format, va_list args);
int vlog_error(const char* caller, const char* format, va_list args);
int vlog_game(const char* caller, const char* format, va_list args);

void setup_logging();
