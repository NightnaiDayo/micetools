#pragma once
#include "../common.h"

static BOOL(WINAPI* TrueGetLocalTime)(SYSTEMTIME* lpSystemTime);
static BOOL(WINAPI* TrueGetSystemTime)(SYSTEMTIME* lpSystemTime);

void hook_time();
