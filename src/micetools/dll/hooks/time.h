#pragma once
#include "../common.h"

static BOOL(WINAPI* TrueSetLocalTime)(const SYSTEMTIME* lpSystemTime);
static BOOL(WINAPI* TrueGetLocalTime)(SYSTEMTIME* lpSystemTime);
static BOOL(WINAPI* TrueSetSystemTime)(const SYSTEMTIME* lpSystemTime);
static BOOL(WINAPI* TrueGetSystemTime)(SYSTEMTIME* lpSystemTime);

void hook_time();
