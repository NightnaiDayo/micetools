#include "processes.h"

#include "../lib/mice/mice.h"

const wchar_t* HOOK_BINARIES[] = {
    L"app\\ALLNetProc.exe",
    L"app\\CameraUploader.exe",
    L"app\\GmSync.exe",
};

BOOL WINAPI FakeCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
                               LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
                               BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment,
                               LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo,
                               LPPROCESS_INFORMATION lpProcessInformation) {
    log_info("spawn", "CreateProcessW %ls %ls", lpApplicationName, lpCommandLine);

    CHAR applicationName[MAX_PATH + 1];
    WideCharToMultiByte(CP_ACP, 0, lpApplicationName, -1, applicationName, sizeof applicationName, NULL, NULL);
    CHAR commandLine[MAX_PATH + 1];
    WideCharToMultiByte(CP_ACP, 0, lpCommandLine, -1, commandLine, sizeof commandLine, NULL, NULL);

    HANDLE child = start_and_inject(applicationName, commandLine, MICELIB, false);
    return child != NULL;
}

void hook_processes() { hook("Kernel32.dll", "CreateProcessW", FakeCreateProcessW, (void**)&TrueCreateProcessW, 6); }
