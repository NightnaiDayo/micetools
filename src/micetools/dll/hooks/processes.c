#include "processes.h"

const wchar_t* HOOK_BINARIES[] = {
    L"app\\ALLNetProc.exe",
    L"app\\CameraUploader.exe",
    L"app\\GmSync.exe",
};

#define DISABLE_PROC_SPAWNING

BOOL WINAPI FakeCreateProcessA(LPCSTR lpApplicationName, LPSTR lpCommandLine,
                               LPSECURITY_ATTRIBUTES lpProcessAttributes,
                               LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles,
                               DWORD dwCreationFlags, LPVOID lpEnvironment,
                               LPCSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo,
                               LPPROCESS_INFORMATION lpProcessInformation) {
    log_info("spawn", "CreateProcessA %s %s", lpApplicationName, lpCommandLine);

    return TrueCreateProcessA("mxAuthDisc.bat", "", lpProcessAttributes,
                              lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment,
                              lpCurrentDirectory, lpStartupInfo, lpProcessInformation);

    HANDLE fake_evt = CreateEvent(NULL, TRUE, FALSE, NULL);
    SetEvent(fake_evt);

    if (lpProcessInformation) {
        lpProcessInformation->hProcess = fake_evt;
    }
    return TRUE;
}
BOOL WINAPI FakeCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
                               LPSECURITY_ATTRIBUTES lpProcessAttributes,
                               LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles,
                               DWORD dwCreationFlags, LPVOID lpEnvironment,
                               LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo,
                               LPPROCESS_INFORMATION lpProcessInformation) {
    // #ifdef DISABLE_PROC_SPAWNING
    //     log_error("spawn", "CreateProcessW %ls %ls", lpApplicationName, lpCommandLine);
    //     return FALSE;
    // #else
    log_info("spawn", "CreateProcessW %ls %ls", lpApplicationName, lpCommandLine);

    // log_info("spawn", "CreateProcessW %ls", lpApplicationName);

    // lpProcessInformation->hThread = GetDummyHandle();
    // return TRUE;

    CHAR applicationName[MAX_PATH + 1];
    WideCharToMultiByte(CP_ACP, 0, lpApplicationName, -1, applicationName, sizeof applicationName,
                        NULL, NULL);

    HANDLE child;
    CHAR commandLine[MAX_PATH + 1];
    WCHAR commandLineW[MAX_PATH + 1];
    WCHAR micePathW[MAX_PATH + 1];
    GetModuleFileNameW(NULL, micePathW, MAX_PATH);

    HANDLE fake_evt = CreateEvent(NULL, TRUE, FALSE, NULL);
    SetEvent(fake_evt);

    if (lpProcessInformation) {
        lpProcessInformation->hProcess = fake_evt;
        lpProcessInformation->hThread = GetDummyHandle();
    }

    return TRUE;

    if (lpCommandLine != NULL) {
        log_error("process", "!!");
        return FALSE;
        // WideCharToMultiByte(CP_ACP, 0, lpCommandLine, -1, commandLine, sizeof commandLine, NULL,
        //                     NULL);
        // child = start_and_inject(applicationName, commandLine, MICELIB, false, 0, NULL,
        //                          CREATE_NEW_CONSOLE);
    } else {
        dwCreationFlags |= CREATE_NEW_CONSOLE;
        wsprintfW(commandLineW, L"mice -b %ls", lpApplicationName);
        printf("%ls %ls\n", micePathW, commandLineW);
        BOOL ret =
            TrueCreateProcessW(L"mice.cmd", commandLineW, lpProcessAttributes, lpThreadAttributes,
                               bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory,
                               lpStartupInfo, lpProcessInformation);
        printf("%d\n", ret);
        return ret;
        // CHAR commandLine[]
        // child =
        //     start_and_inject(applicationName, NULL, MICELIB, false, 0, NULL, CREATE_NEW_CONSOLE);
    }

    return !FAILED(child);
    // #endif
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
