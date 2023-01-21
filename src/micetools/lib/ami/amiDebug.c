#include "amiDebug.h"

#include "../mice/mice.h"

AM_LIB_C_HEADER(amiDebug, AM_DEBUG)

FILE* amiDebugFile;
amiDebugCallback_t* amiDebugCallback;

int amiDebugVprintf(LPCSTR format, va_list args) {
    if (amiDebugCallback == NULL) {
        return vfprintf_s(amiDebugFile ? amiDebugFile : stderr, format, args);
    }

    char buffer[1024];
    vsnprintf_s(buffer, sizeof buffer, sizeof buffer - 1, format, args);
    return (*amiDebugCallback)(buffer);
}

int amiDebugPrintf(LPCSTR format, ...) {
    va_list args;
    va_start(args, format);
    int ret = amiDebugVprintf(format, args);
    va_end(args);
    return ret;
}

int amiDebugVprintfEx(LPCSTR caller, DWORD line, LPCSTR format, va_list args) {
    char buf[1024];

    int prefixLength = _snprintf_s(buf, sizeof buf, sizeof buf - 1, "%s: Line%d  ", caller, line);
    if (amiDebugCallback == NULL) {
        FILE* file = amiDebugFile ? amiDebugFile : stderr;
        if (-1 < fprintf_s(file, "%s", buf)) {
            return vfprintf_s(file, format, args);
        }
        return 0;
    }

    vsnprintf_s(buf + prefixLength, sizeof buf - prefixLength, (sizeof buf - 1) - prefixLength,
                format, args);
    return (*amiDebugCallback)(buf);
}

int amiDebugPrintfEx(LPCSTR caller, DWORD line, LPCSTR format, ...) {
    va_list args;
    va_start(args, format);
    int ret = amiDebugVprintfEx(caller, line, format, args);
    va_end(args);
    return ret;
}

void amiDebugSetFile(FILE* file) { amiDebugFile = file; }
void amiDebugSetCallback(amiDebugCallback_t* callback) { amiDebugCallback = callback; }
