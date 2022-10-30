#pragma once
// #include "../common.h"
#include <Windows.h>
#include <string.h>
#include <stdlib.h>

static HANDLE(WINAPI* TrueRegisterEventSourceA)(LPCSTR lpUNCServerName, LPCSTR lpSourceName);
static BOOL(WINAPI* TrueReportEventA)(HANDLE hEventLog, WORD wType, WORD wCategory, DWORD dwEventID, PSID lpUserSid,
                                      WORD wNumStrings, DWORD dwDataSize, LPCSTR* lpStrings, LPVOID lpRawData);
static BOOL(WINAPI* TrueDeregisterEventSource)(HANDLE hEventLog);

int(WINAPIV* Trueprintf)(const char* _Format, ...);
static int(WINAPIV* Truefprintf)(FILE* _File, const char* _Format, ...);
static int(WINAPIV* Truefprintf_s)(FILE* _Stream, const char* _Format, ...);
static int(WINAPIV* Truevfprintf_s)(FILE* _Stream, const char* _Format, va_list _ArgList);

void hook_logging();
