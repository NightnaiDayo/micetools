#include <Windows.h>
#include <stdbool.h>
#include <stdio.h>

BOOL start_and_inject(HANDLE hJob, LPCSTR path, LPSTR cmdline, LPCSTR inject, BOOL debug_wait,
                      DWORD delay, LPCSTR extra_injections, DWORD flags,
                      LPPROCESS_INFORMATION lpProcessInformation);

#ifdef _WIN64
#define MICELIB "mice64.dll"
#define MICEEXE "mice64.exe"
#else
#define MICELIB "mice.dll"
#define MICEEXE "mice.exe"
#endif
