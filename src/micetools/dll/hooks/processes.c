#include "processes.h"

const wchar_t* HOOK_BINARIES[] = {
    L"app\\ALLNetProc.exe",
    L"app\\CameraUploader.exe",
    L"app\\GmSync.exe",
};

BOOL WINAPI FakeCreateProcessA(LPCSTR lpApplicationName, LPSTR lpCommandLine,
                               LPSECURITY_ATTRIBUTES lpProcessAttributes,
                               LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles,
                               DWORD dwCreationFlags, LPVOID lpEnvironment,
                               LPCSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo,
                               LPPROCESS_INFORMATION lpProcessInformation) {
    if (dwCreationFlags & CREATE_SUSPENDED) {
        return TrueCreateProcessA(lpApplicationName, lpCommandLine, lpProcessAttributes,
                                  lpThreadAttributes, bInheritHandles, dwCreationFlags,
                                  lpEnvironment, lpCurrentDirectory, lpStartupInfo,
                                  lpProcessInformation);
    }

    log_info(plfProcesses, "CreateProcessA %s %s", lpApplicationName, lpCommandLine);

    return start_and_inject(INVALID_HANDLE_VALUE, lpApplicationName, lpCommandLine,
                                       MICELIB, FALSE, 0, NULL, 0, lpProcessInformation);
}
BOOL WINAPI FakeCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
                               LPSECURITY_ATTRIBUTES lpProcessAttributes,
                               LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles,
                               DWORD dwCreationFlags, LPVOID lpEnvironment,
                               LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo,
                               LPPROCESS_INFORMATION lpProcessInformation) {
    log_info(plfProcesses, "CreateProcessW %ls %ls", lpApplicationName, lpCommandLine);

    CHAR applicationName[MAX_PATH + 1];
    WideCharToMultiByte(CP_ACP, 0, lpApplicationName, -1, applicationName, sizeof applicationName,
                        NULL, NULL);

    int nMultiChars = WideCharToMultiByte(CP_ACP, 0, lpCommandLine, -1, NULL, 0, NULL, NULL);
    LPSTR commandLine = malloc(nMultiChars);
    WideCharToMultiByte(CP_ACP, 0, lpCommandLine, -1, commandLine, nMultiChars, NULL, NULL);

    return start_and_inject(INVALID_HANDLE_VALUE, applicationName, commandLine, MICELIB,
                                       FALSE, 0, NULL, 0, lpProcessInformation);
}

BOOL WINAPI FakeGetExitCodeProcess(HANDLE hProcess, LPDWORD lpExitCode) {
    *lpExitCode = 0;
    return TRUE;
}

void hook_processes() {
    hook("Kernel32.dll", "CreateProcessW", FakeCreateProcessW, (void**)&TrueCreateProcessW);
    hook("Kernel32.dll", "CreateProcessA", FakeCreateProcessA, (void**)&TrueCreateProcessA);
    // hook("Kernel32.dll", "GetExitCodeProcess", FakeGetExitCodeProcess, NULL);
}
