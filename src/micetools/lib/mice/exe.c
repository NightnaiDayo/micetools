#include "exe.h"

bool inject_debug_wait(HANDLE process) {
    BOOL present;

    fprintf(stderr, "Waiting for debugger to attach.\n");
    do {
        Sleep(1000);
        if (FAILED(CheckRemoteDebuggerPresent(process, &present))) {
            fprintf(stderr, "Fatal: CheckRemoteDebuggerPresent failed: %03x\n", GetLastError());
            return false;
        }
    } while (!present);
    fprintf(stderr, "Debugger attached, resuming\n");
    return true;
}
bool remote_call(HANDLE process, LPVOID function, LPCSTR argument) {
    int nchars = strlen(argument);

    LPVOID arg_addr = VirtualAllocEx(process, NULL, nchars + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (arg_addr == NULL) {
        fprintf(stderr, "Fatal: VirtualAllocEx failed: %03x\n", GetLastError());
        return false;
    }

    if (FAILED(WriteProcessMemory(process, arg_addr, argument, nchars + 1, NULL))) {
        fprintf(stderr, "Fatal: WriteProcessMemory failed: %03x\n", GetLastError());
        return false;
    }

    HANDLE remote_thread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)function, arg_addr, 0, NULL);
    if (remote_thread == NULL) {
        fprintf(stderr, "Fatal: CreateRemoteThread failed: %03x\n", GetLastError());
        return false;
    }

    if (WaitForSingleObject(remote_thread, INFINITE) != WAIT_OBJECT_0) {
        fprintf(stderr, "Fatal: WaitForSingleObject failed: %03x\n", GetLastError());
        return false;
    }

    DWORD result;
    if (FAILED(GetExitCodeThread(remote_thread, &result))) {
        fprintf(stderr, "Fatal: GetExitCodeThread failed: %03x\n", GetLastError());
        return false;
    }
    if (result == 0) {
        fprintf(stderr, "Fatal: GetExitCodeThread failed: result == 0\n");
        return false;
    }

    return true;
}
bool inject_dll(HANDLE process, LPCSTR inject) {
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    if (kernel32 == NULL) {
        fprintf(stderr, "Fatal: GetModuleHandleA failed: %03x\n", GetLastError());
        return false;
    }

    LPVOID addr_LoadLibraryA = (LPVOID)GetProcAddress(kernel32, "LoadLibraryA");
    if (addr_LoadLibraryA == NULL) {
        fprintf(stderr, "Fatal: GetProcAddress failed: %03x\n", GetLastError());
        return false;
    }

    return remote_call(process, addr_LoadLibraryA, inject);
}

HANDLE start_and_inject(LPCSTR path, LPSTR cmdline, LPCSTR inject, BOOL delay) {
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
        fprintf(stderr, "Fatal: Cannot start %s: not found\n", path);
        goto abort;
    }
    // Does the DLL we want to inject exist?
    found = SearchPathA(NULL, inject, NULL, 0, NULL, NULL);
    if (found == 0) {
        fprintf(stderr, "Fatal: Cannot inject %s: not found\n", inject);
        goto abort;
    }

    // Start the binary
    if (!CreateProcessA(path, cmdline, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &startupInfo,
                              &processInformation)) {
        fprintf(stderr, "Fatal: CreateProcessA failed: %03x\n", GetLastError());
        goto abort;
    }

    if (delay) {
        if (!inject_debug_wait(processInformation.hProcess)) goto abort;
    }
    if (!inject_dll(processInformation.hProcess, inject)) goto abort;

    // Injection completed, let the program continue execution
    if (FAILED(ResumeThread(processInformation.hThread))) {
        fprintf(stderr, "Fatal: ResumeThread failed: %03x\n", GetLastError());
        goto abort;
    }

    return processInformation.hProcess;

abort:
    if (processInformation.hProcess) {
        if (!CloseHandle(processInformation.hThread))
            fprintf(stderr, "Fatal: CloseHandle(hProcess) failed: %03x\n", GetLastError());

        if (!TerminateProcess(processInformation.hProcess, 1))
            fprintf(stderr, "Fatal: TerminateProcess failed: %03x\n", GetLastError());
    }
    if (processInformation.hThread) {
        if (!CloseHandle(processInformation.hThread))
            fprintf(stderr, "Fatal: CloseHandle(hThread) failed: %03x\n", GetLastError());
    }

    return INVALID_HANDLE_VALUE;
}
