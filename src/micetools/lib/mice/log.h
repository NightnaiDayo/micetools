#include <Windows.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#define LOG_MISC
#define LOG_INFO
#define LOG_WARNING
#define LOG_ERROR

#define HOOKS_LOGGER "hooks"
#define BOOT_LOGGER "boot"

extern CRITICAL_SECTION logger_lock;

int log_misc(const char* caller, const char* format, ...);
int log_info(const char* caller, const char* format, ...);
int log_warning(const char* caller, const char* format, ...);
int log_error(const char* caller, const char* format, ...);

int vlog_misc(const char* caller, const char* format, va_list args);
int vlog_info(const char* caller, const char* format, va_list args);
int vlog_warning(const char* caller, const char* format, va_list args);
int vlog_error(const char* caller, const char* format, va_list args);

void setup_logging();
