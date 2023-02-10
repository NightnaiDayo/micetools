#include <Windows.h>
#include <stdbool.h>
#include <stdio.h>

HANDLE start_and_inject(LPCSTR path, LPSTR cmdline, LPCSTR inject, BOOL debug_wait, DWORD delay,
                        LPCSTR extra_injections, DWORD flags);

#ifndef MICELIB
#ifdef MICE_WIN32
#define MICELIB "mice86.dll"
#else  // MICE_WIN32
#define MICELIB "mice64.dll"
#endif  // MICE_WIN32
#endif  // MICELIB
