#include <Windows.h>
#include <stdbool.h>
#include <stdio.h>

HANDLE start_and_inject(HANDLE hJob, LPCSTR path, LPSTR cmdline, LPCSTR inject, BOOL debug_wait,
                        DWORD delay, LPCSTR extra_injections, DWORD flags);

#define MICELIB "mice.dll"
