#pragma once

#include <Windows.h>
#include <stdio.h>

#define LOG_MISC FALSE
#define LOG_INFO TRUE
#define LOG_WARNING TRUE
#define LOG_ERROR TRUE
#define LOG_GAME TRUE

// #define LOG_TO_FILE

#define COMM_LOGGER "comm"
#define HOOKS_LOGGER "hooks"
#define BOOT_LOGGER "boot"

extern CRITICAL_SECTION logger_lock;

int log_misc(const char* caller, const char* format, ...);
int log_info(const char* caller, const char* format, ...);
int log_warning(const char* caller, const char* format, ...);
int log_error(const char* caller, const char* format, ...);
int log_game(const char* caller, const char* format, ...);

int vlog_misc(const char* caller, const char* format, va_list args);
int vlog_info(const char* caller, const char* format, va_list args);
int vlog_warning(const char* caller, const char* format, va_list args);
int vlog_error(const char* caller, const char* format, va_list args);
int vlog_game(const char* caller, const char* format, va_list args);

void setup_logging();
