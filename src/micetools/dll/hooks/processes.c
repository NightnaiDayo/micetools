#include "processes.h"

#include "../lib/mice/ipc.h"

BOOL WINAPI FakeCreateProcessA(LPCSTR lpApplicationName, LPSTR lpCommandLine,
                               LPSECURITY_ATTRIBUTES lpProcessAttributes,
                               LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles,
                               DWORD dwCreationFlags, LPVOID lpEnvironment,
                               LPCSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo,
                               LPPROCESS_INFORMATION lpProcessInformation) {
    // Somewhat janky way to identify when this is a loopback from start_and_inject
    // TODO: _CreateProcessA()
    if (dwCreationFlags & CREATE_SUSPENDED) {
        return TrueCreateProcessA(lpApplicationName, lpCommandLine, lpProcessAttributes,
                                  lpThreadAttributes, bInheritHandles, dwCreationFlags,
                                  lpEnvironment, lpCurrentDirectory, lpStartupInfo,
                                  lpProcessInformation);
    }
    // Bind everything to our one single console
    dwCreationFlags &= ~CREATE_NEW_CONSOLE;
    dwCreationFlags |= CREATE_NO_WINDOW;

    if (lpCommandLine &&
        (strcmp(lpCommandLine, "s:\\mxkeychip.exe") == 0 ||
         strcmp(lpCommandLine, "C:\\WINDOWS\\system32\\regini.exe S:\\default_regset.txt") == 0)) {
        if (lpProcessInformation) {
            lpProcessInformation->hProcess = CreateEventA(NULL, FALSE, TRUE, NULL);
            lpProcessInformation->hThread = CreateEventA(NULL, FALSE, TRUE, NULL);
        }
        return TRUE;
    }

    CHAR szApplicationName[MAX_PATH + 1];
    if (lpApplicationName == NULL) {
        strcpy_s(szApplicationName, sizeof szApplicationName, lpCommandLine);
        for (int i = 0; i < sizeof szApplicationName; i++) {
            if (szApplicationName[i] == ' ') {
                szApplicationName[i] = '\0';
                break;
            }
        }

        lpApplicationName = szApplicationName;
    }

    MiceFSRedirectPathA(lpApplicationName, &lpApplicationName);
    log_info(plfProcesses, "CreateProcessA %s %s", lpApplicationName, lpCommandLine);

    if (!_miceIpcData->m_LauncherIsReady || _miceIpcData->m_MiceDll[0] == '\0') {
        log_error(plfProcesses, "MiceLIB not provided via IPC!");
        return FALSE;
    }

    log_info(plfProcesses, "Spawning \"%s\" %s", lpApplicationName, lpCommandLine);
    return start_and_inject(INVALID_HANDLE_VALUE, lpApplicationName, lpCommandLine,
                            _miceIpcData->m_MiceDll, FALSE, 0, NULL, dwCreationFlags,
                            lpProcessInformation);
}
BOOL WINAPI FakeCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
                               LPSECURITY_ATTRIBUTES lpProcessAttributes,
                               LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles,
                               DWORD dwCreationFlags, LPVOID lpEnvironment,
                               LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo,
                               LPPROCESS_INFORMATION lpProcessInformation) {
    // if (lpCommandLine && (wcscmp(lpCommandLine, L"chkdsk.exe") == 0 ||
    //                       wcscmp(lpCommandLine, L"ALLNetProc_Win.exe") == 0 ||
    //                       wcscmp(lpCommandLine, L"ALLNetProc_Ring.exe") == 0 ||
    //                       wcscmp(lpCommandLine, L"ALLNetProc.exe") == 0)) {
    if (lpCommandLine && (_wcsnicmp(lpCommandLine, L"chkdsk.exe", 10) == 0)) {
        if (lpProcessInformation) {
            lpProcessInformation->hProcess = CreateEventA(NULL, FALSE, TRUE, NULL);
            lpProcessInformation->hThread = CreateEventA(NULL, FALSE, TRUE, NULL);
        }
        return TRUE;
    }
    // Bind everything to our one single console
    dwCreationFlags &= ~CREATE_NEW_CONSOLE;
    dwCreationFlags |= CREATE_NO_WINDOW;

    int nMultiChars = WideCharToMultiByte(CP_ACP, 0, lpCommandLine, -1, NULL, 0, NULL, NULL);
    LPSTR commandLine = malloc(nMultiChars + 1);
    commandLine[0] = '\0';
    WideCharToMultiByte(CP_ACP, 0, lpCommandLine, -1, commandLine, nMultiChars, NULL, NULL);
    commandLine[nMultiChars] = '\0';

    CHAR szApplicationName[MAX_PATH + 1];
    LPCSTR lpApplicationNameA;
    if (lpApplicationName == NULL) {
        strcpy_s(szApplicationName, sizeof szApplicationName, commandLine);
        for (int i = 0; i < sizeof szApplicationName; i++) {
            if (szApplicationName[i] == ' ') {
                szApplicationName[i] = '\0';
                break;
            }
        }

        lpApplicationNameA = szApplicationName;
    } else {
        WideCharToMultiByte(CP_ACP, 0, lpApplicationName, -1, szApplicationName,
                            sizeof szApplicationName, NULL, NULL);

        lpApplicationNameA = szApplicationName;
    }

    MiceFSRedirectPathA(lpApplicationNameA, &lpApplicationNameA);

    if (!_miceIpcData->m_LauncherIsReady || _miceIpcData->m_MiceDll[0] == '\0') {
        log_error(plfProcesses, "MiceLIB not provided via IPC!");
        return FALSE;
    }

    log_info(plfProcesses, "Spawning \"%s\" %s", lpApplicationNameA, commandLine);
    return start_and_inject(INVALID_HANDLE_VALUE, lpApplicationNameA, commandLine,
                            _miceIpcData->m_MiceDll, FALSE, 0, NULL, dwCreationFlags,
                            lpProcessInformation);
}

BOOL WINAPI FakeGetExitCodeProcess(HANDLE hProcess, LPDWORD lpExitCode) {
    *lpExitCode = 0;
    return TRUE;
}

HINSTANCE WINAPI FakeShellExecuteA(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile,
                                   LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd) {
    MiceFSRedirectPathA(lpFile, &lpFile);
    if (lpDirectory) MiceFSRedirectPathA(lpDirectory, &lpDirectory);
    return TrueShellExecuteA(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
}

void hook_processes() {
    hook("Kernel32.dll", "CreateProcessW", FakeCreateProcessW, (void**)&TrueCreateProcessW);
    hook("Kernel32.dll", "CreateProcessA", FakeCreateProcessA, (void**)&TrueCreateProcessA);
    hook("Kernel32.dll", "GetExitCodeProcess", FakeGetExitCodeProcess, NULL);
    // hook("Shell32.dll", "ShellExecuteA", FakeShellExecuteA, (void**)TrueShellExecuteA);

    // TODO: ShellExecuteA
}
