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

    // return TrueCreateProcessA("mxAuthDisc.bat", "", lpProcessAttributes, lpThreadAttributes,
    //                           bInheritHandles, dwCreationFlags, lpEnvironment,
    //                           lpCurrentDirectory, lpStartupInfo, lpProcessInformation);

    // HANDLE fake_evt = CreateEvent(NULL, TRUE, FALSE, NULL);
    // SetEvent(fake_evt);

    HANDLE hProcess = start_and_inject(INVALID_HANDLE_VALUE, lpApplicationName, lpCommandLine,
                                       MICELIB, FALSE, 0, "", 0);

    if (lpProcessInformation) {
        lpProcessInformation->hProcess = hProcess;
    }

    return TRUE;
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

    // WideCharToMultiByte(CP_ACP, 0, lpApplicationName, -1, applicationName, sizeof
    // applicationName,
    //                     NULL, NULL);

    // HANDLE child;
    // CHAR commandLine[MAX_PATH + 1];
    // WCHAR commandLineW[MAX_PATH + 1];
    // WCHAR micePathW[MAX_PATH + 1];
    // GetModuleFileNameW(NULL, micePathW, MAX_PATH);

    // HANDLE fake_evt = CreateEvent(NULL, TRUE, FALSE, NULL);
    // SetEvent(fake_evt);

    // return TrueCreateProcessA(applicationName, commandLine, lpProcessAttributes, lpThreadAttributes,
    //                           bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory,
    //                           lpStartupInfo, lpProcessInformation);

    HANDLE hProcess = start_and_inject(INVALID_HANDLE_VALUE, applicationName, commandLine, MICELIB,
                                       TRUE, 0, NULL, 0);
    if (lpProcessInformation) {
        lpProcessInformation->hProcess = hProcess;
        lpProcessInformation->hThread = GetDummyHandle();
    }

    log_game(plfProcesses, "hP: %08x", hProcess);

    return TRUE;

    // if (lpCommandLine != NULL) {
    //     log_error(plfProcesses, "!!");
    //     return FALSE;
    //     // WideCharToMultiByte(CP_ACP, 0, lpCommandLine, -1, commandLine, sizeof commandLine,
    //     NULL,
    //     //                     NULL);
    //     // child = start_and_inject(INVALID_HANDLE_VALUE, applicationName, commandLine, MICELIB,
    //     // false, 0, NULL,
    //     //                          CREATE_NEW_CONSOLE);
    // } else {
    //     dwCreationFlags |= CREATE_NEW_CONSOLE;
    //     wsprintfW(commandLineW, L"mice -b %ls", lpApplicationName);
    //     printf("%ls %ls\n", micePathW, commandLineW);
    //     BOOL ret =
    //         TrueCreateProcessW(L"mice.cmd", commandLineW, lpProcessAttributes,
    //         lpThreadAttributes,
    //                            bInheritHandles, dwCreationFlags, lpEnvironment,
    //                            lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
    //     printf("%d\n", ret);
    //     return ret;
    //     // CHAR commandLine[]
    //     // child =
    //     //     start_and_inject(INVALID_HANDLE_VALUE, applicationName, NULL, MICELIB, false, 0,
    //     //     NULL, CREATE_NEW_CONSOLE);
    // }

    // return !FAILED(child);
    // // #endif
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
