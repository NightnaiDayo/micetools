#include "exe.h"
#include "log.h"

bool inject_debug_wait(HANDLE process) {
    BOOL present;

    log_info(BOOT_LOGGER, "Waiting for debugger to attach.");
    do {
        Sleep(1000);
        if (FAILED(CheckRemoteDebuggerPresent(process, &present))) {
            log_error(BOOT_LOGGER, "CheckRemoteDebuggerPresent failed: %03x", GetLastError());
            return false;
        }
    } while (!present);
    log_info(BOOT_LOGGER, "Debugger attached, resuming");
    return true;
}
bool remote_call(HANDLE process, LPVOID function, LPCSTR argument) {
    int nchars = strlen(argument);

    LPVOID arg_addr = VirtualAllocEx(process, NULL, nchars + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (arg_addr == NULL) {
        log_error(BOOT_LOGGER, "VirtualAllocEx failed: %03x", GetLastError());
        return false;
    }

    if (FAILED(WriteProcessMemory(process, arg_addr, argument, nchars + 1, NULL))) {
        log_error(BOOT_LOGGER, "WriteProcessMemory failed: %03x", GetLastError());
        return false;
    }

    HANDLE remote_thread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)function, arg_addr, 0, NULL);
    if (remote_thread == NULL) {
        log_error(BOOT_LOGGER, "CreateRemoteThread failed: %03x", GetLastError());
        return false;
    }

    if (WaitForSingleObject(remote_thread, INFINITE) != WAIT_OBJECT_0) {
        log_error(BOOT_LOGGER, "WaitForSingleObject failed: %03x", GetLastError());
        return false;
    }

    DWORD result;
    if (FAILED(GetExitCodeThread(remote_thread, &result))) {
        log_error(BOOT_LOGGER, "GetExitCodeThread failed: %03x", GetLastError());
        return false;
    }
    if (result == 0) {
        log_error(BOOT_LOGGER, "GetExitCodeThread failed: result == 0");
        return false;
    }

    return true;
}
bool inject_dll(HANDLE process, LPCSTR inject) {
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    if (kernel32 == NULL) {
        log_error(BOOT_LOGGER, "GetModuleHandleA failed: %03x", GetLastError());
        return false;
    }

    LPVOID addr_LoadLibraryA = (LPVOID)GetProcAddress(kernel32, "LoadLibraryA");
    if (addr_LoadLibraryA == NULL) {
        log_error(BOOT_LOGGER, "GetProcAddress failed: %03x", GetLastError());
        return false;
    }

    return remote_call(process, addr_LoadLibraryA, inject);
}

HANDLE start_and_inject(LPCSTR path, LPSTR cmdline, LPCSTR inject, BOOL delay) {
    log_misc(BOOT_LOGGER, "Using %s for hooks", inject);
    STARTUPINFOA startupInfo;
    PROCESS_INFORMATION processInformation;

    memset(&startupInfo, 0, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    // char real_inject_path[MAX_PATH + 1];
    // snprintf(real_inject_path, MAX_PATH + 1, ".\\%s", inject);
    // GetFullPathNameA(real_inject_path, MAX_PATH + 1, &real_inject_path, NULL);

    // Validate that we're not about to try something insane
    DWORD found = SearchPathA(NULL, inject, NULL, 0, NULL, NULL);
    if (found == 0) {
        log_error(BOOT_LOGGER, "Cannot inject %s: not found: %03x", inject, GetLastError());
        goto abort;
    }

    // Start the binary
    if (FAILED(CreateProcessA(path, cmdline, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &startupInfo,
                              &processInformation))) {
        log_error(BOOT_LOGGER, "CreateProcessA failed: %03x", GetLastError());
        goto abort;
    }

    if (delay) {
        if (!inject_debug_wait(processInformation.hProcess)) goto abort;
    }
    if (!inject_dll(processInformation.hProcess, inject)) goto abort;

    // Injection completed, let the program continue execution
    if (FAILED(ResumeThread(processInformation.hThread))) {
        log_error(BOOT_LOGGER, "ResumeThread failed: %03x", GetLastError());
        goto abort;
    }

    return processInformation.hProcess;

abort:
    if (processInformation.hProcess) {
        if (FAILED(CloseHandle(processInformation.hThread)))
            log_error(BOOT_LOGGER, "CloseHandle(hProcess) failed: %03x", GetLastError());

        if (FAILED(TerminateProcess(processInformation.hProcess, 1)))
            log_error(BOOT_LOGGER, "TerminateProcess failed: %03x", GetLastError());
    }
    if (processInformation.hThread) {
        if (FAILED(CloseHandle(processInformation.hThread)))
            log_error(BOOT_LOGGER, "CloseHandle(hThread) failed: %03x", GetLastError());
    }

    return NULL;
}
