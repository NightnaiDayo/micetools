#pragma once

#include <Windows.h>
#include <stdio.h>
#include <varargs.h>

#include "../_am.h"

typedef struct AM_DEBUG {
    unsigned char Rsv00;
} AM_DEBUG;
AM_LIB_H_HEADER(amiDebug, AM_DEBUG)

typedef int(amiDebugCallback_t)(LPCSTR data);

extern amiDebugCallback_t* amiDebugCallback;
extern FILE* amiDebugFile;

int amiDebugVprintf(LPCSTR format, va_list args);
int amiDebugPrintf(LPCSTR format, ...);
#define amiDebugPrintf(fmt, ...) amiDebugPrintf(fmt, __VA_ARGS__)

int amiDebugVprintfEx(LPCSTR caller, DWORD line, LPCSTR format, va_list args);
int amiDebugPrintfEx(LPCSTR caller, DWORD line, LPCSTR format, ...);
#define amiDebugLog(fmt, ...) amiDebugPrintfEx(__func__, __LINE__, fmt##"\n", __VA_ARGS__)

void amiDebugSetFile(FILE* file);
void amiDebugSetCallback(amiDebugCallback_t* callback);
