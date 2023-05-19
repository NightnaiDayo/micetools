#include "exe.h"

#include <stdlib.h>

#include "log.h"

bool inject_debug_wait(HANDLE process) {
    BOOL present;

    log_info(plfBoot, "Waiting for debugger to attach.");
    do {
        Sleep(1000);
        if (FAILED(CheckRemoteDebuggerPresent(process, &present))) {
            log_error(plfBoot, "CheckRemoteDebuggerPresent failed: %d", GetLastError());
            return false;
        }
    } while (!present);
    log_info(plfBoot, "Debugger attached, resuming");
    return true;
}
bool remote_call(HANDLE process, LPVOID function, LPCSTR argument, DWORD* result) {
    int nchars = strlen(argument);

    LPVOID arg_addr =
        VirtualAllocEx(process, NULL, nchars + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (arg_addr == NULL) {
        log_error(plfProcesses, "VirtualAllocEx failed: %d", GetLastError());
        return false;
    }

    if (FAILED(WriteProcessMemory(process, arg_addr, argument, nchars + 1, NULL))) {
        log_error(plfProcesses, "WriteProcessMemory failed: %d", GetLastError());
        return false;
    }

    HANDLE remote_thread =
        CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)function, arg_addr, 0, NULL);
    if (remote_thread == INVALID_HANDLE_VALUE) {
        log_error(plfProcesses, "CreateRemoteThread failed: %d", GetLastError());
        return false;
    }

    if (WaitForSingleObject(remote_thread, INFINITE) != WAIT_OBJECT_0) {
        log_error(plfProcesses, "WaitForSingleObject failed: %d", GetLastError());
        return false;
    }

    if (FAILED(GetExitCodeThread(remote_thread, result))) {
        log_error(plfProcesses, "GetExitCodeThread failed: %d", GetLastError());
        return false;
    }
    if (*result == 0) {
        log_error(plfProcesses, "GetExitCodeThread failed: result == 0");
        return false;
    }

    return true;
}
bool inject_dll(HANDLE process, LPCSTR inject) {
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    if (kernel32 == NULL) {
        log_error(plfProcesses, "GetModuleHandleA failed: %d", GetLastError());
        return false;
    }

    LPVOID addr_LoadLibraryA = (LPVOID)GetProcAddress(kernel32, "LoadLibraryA");
    if (addr_LoadLibraryA == NULL) {
        log_error(plfProcesses, "GetProcAddress failed: %d", GetLastError());
        return false;
    }

    DWORD result;
    return remote_call(process, addr_LoadLibraryA, inject, &result);
}

BOOL start_and_inject(HANDLE hJob, LPCSTR path, LPSTR cmdline, LPCSTR inject, BOOL debug_wait,
                      DWORD delay, LPCSTR extra_injections, DWORD flags,
                      LPPROCESS_INFORMATION lpProcessInformation) {
    STARTUPINFOA startupInfo;
    PROCESS_INFORMATION processInformation = { 0 };

    memset(&startupInfo, 0, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    // char real_inject_path[MAX_PATH + 1];
    // snprintf(real_inject_path, MAX_PATH + 1, ".\\%s", inject);
    // GetFullPathNameA(real_inject_path, MAX_PATH + 1, &real_inject_path, NULL);

    DWORD found;
    // Does the exe we're starting exist?
    found = SearchPathA(NULL, path, NULL, 0, NULL, NULL);
    if (found == 0) {
        log_error(plfProcesses, "Cannot start %s: not found", path);
        goto abort;
    }
    if (inject != NULL) {
        // Does the DLL we want to inject exist?
        found = SearchPathA(NULL, inject, NULL, 0, NULL, NULL);
        if (found == 0) {
            log_error(plfProcesses, "Cannot inject %s: not found", inject);
            goto abort;
        }
    }

    // Start the binary
    flags |= CREATE_SUSPENDED;
    if (!CreateProcessA(path, cmdline, NULL, NULL, FALSE, flags, NULL, NULL, &startupInfo,
                        &processInformation)) {
        log_error(plfProcesses, "CreateProcessA(%s, %s) failed: %d", path, cmdline,
                  GetLastError());
        goto abort;
    }

    if (hJob != INVALID_HANDLE_VALUE) AssignProcessToJobObject(hJob, processInformation.hProcess);

    if (debug_wait)
        if (!inject_debug_wait(processInformation.hProcess)) goto abort;
    if (inject != NULL)
        if (!inject_dll(processInformation.hProcess, inject)) goto abort;

    if (extra_injections != NULL && extra_injections[0] != '\0') {
        char* copy = (char*)malloc(strlen(extra_injections) + 1);
        memcpy_s(copy, strlen(extra_injections) + 1, extra_injections,
                 strlen(extra_injections) + 1);

        char* next_token;
        char* token = strtok_s(copy, "\n", &next_token);
        while (token != NULL) {
            if (!inject_dll(processInformation.hProcess, token)) goto abort;
            token = strtok_s(NULL, "\n", &next_token);
        }

        free(copy);
    }

    if (delay) {
        log_info(plfBoot, "Delaying for %dms", delay);
        Sleep(delay);
    }

    // Injection completed, let the program continue execution
    if (FAILED(ResumeThread(processInformation.hThread))) {
        log_error(plfProcesses, "ResumeThread failed: %d", GetLastError());
        goto abort;
    }

    if (lpProcessInformation) {
        memcpy(lpProcessInformation, &processInformation, sizeof processInformation);
    }

    return TRUE;

abort:
    if (processInformation.hProcess && processInformation.hProcess != INVALID_HANDLE_VALUE) {
        if (!CloseHandle(processInformation.hThread) && GetLastError() != ERROR_INVALID_HANDLE)
            log_error(plfProcesses, "CloseHandle(hProcess) failed: %d", GetLastError());

        if (!TerminateProcess(processInformation.hProcess, 1))
            log_error(plfProcesses, "TerminateProcess failed: %d", GetLastError());
    }
    if (processInformation.hThread && processInformation.hThread != INVALID_HANDLE_VALUE) {
        if (!CloseHandle(processInformation.hThread) && GetLastError() != ERROR_INVALID_HANDLE)
            log_error(plfProcesses, "CloseHandle(hThread) failed: %d", GetLastError());
    }

    return FALSE;
}
