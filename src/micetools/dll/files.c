#include "files.h"

#include "../lib/mice/mice.h"

HANDLE fake_handle = (HANDLE)0x10000000;

file_hook_t* file_hook_list = NULL;
file_hook_t* new_file_hook(LPCWSTR filename) {
    file_hook_t* hook = (file_hook_t*)malloc(sizeof(file_hook_t));

    hook->filename = filename;

    hook->DeviceIoControl = NULL;
    hook->SetFilePointer = NULL;
    hook->WriteFile = NULL;
    hook->ReadFile = NULL;

    return hook;
}
void hook_file(file_hook_t* hook) {
    hook->next = NULL;
    hook->virtual_handle = (LPHANDLE)malloc(sizeof(HANDLE));
    *hook->virtual_handle = NULL;
    if (file_hook_list == NULL) {
        file_hook_list = hook;
        return;
    }

    file_hook_t* hl = file_hook_list;
    while (hl->next != NULL) hl = hl->next;
    hl->next = hook;
};

drive_redirect_t DRIVE_REDIRECT_TABLE[] = {
    {.drive = "Y:\\", .path = ".\\dev\\Y\\"},
    {.drive = "C:\\Documents and Settings\\AppUser\\temp\\", .path = ".\\dev\\temp\\"}};

char* redirect_path(char* path) {
    for (int i = 0; i < sizeof DRIVE_REDIRECT_TABLE / sizeof DRIVE_REDIRECT_TABLE[0]; i++) {
        drive_redirect_t row = DRIVE_REDIRECT_TABLE[i];
        if (strstr(path, row.drive)) {
            log_misc(HOOKS_LOGGER, "Redirecting '%s' to '%s'", path, row.path);

            size_t new_len = strlen(path) - strlen(row.drive) + strlen(row.path);
            // TODO: Make this not leak memory!
            char* new_str = (char*)malloc(new_len + 1);
            strcpy_s(new_str, new_len + 1, row.path);

            char* dst = new_str + strlen(row.path);
            size_t len = strlen(path) - strlen(row.drive);
            char* src = path + strlen(row.drive);

            for (; len > 0; len--) (dst++)[0] = (src++)[0];
            dst[0] = 0;
            log_misc(HOOKS_LOGGER, "New filename: '%s'", new_str);
            return new_str;
        }
    }
    return path;
}

HANDLE WINAPI FakeCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                              LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
                              DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
    HANDLE handle = NULL;

    file_hook_t* hook = file_hook_list;
    while (hook != NULL) {
        if (wcscmp(lpFileName, hook->filename) == 0) {
            if (*hook->virtual_handle == NULL) {
                // TODO: Assign handles better!
                *hook->virtual_handle = fake_handle;
                ((size_t)fake_handle)++;
            }
            handle = *hook->virtual_handle;
            break;
        }
        hook = hook->next;
    }

    if (handle == NULL) {
        handle = TrueCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition,
                                 dwFlagsAndAttributes, hTemplateFile);
    }

    log_misc(HOOKS_LOGGER, "CreateFileW(%ls) -> 0x%p", lpFileName, handle);
    return handle;
}
HANDLE WINAPI FakeCreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                              LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
                              DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
    lpFileName = redirect_path((char*)lpFileName);

    WCHAR wideFileName[MAX_PATH + 1];
    MultiByteToWideChar(CP_ACP, 0, lpFileName, -1, (LPWSTR)&wideFileName, MAX_PATH + 1);

    HANDLE result = FakeCreateFileW((LPCWSTR)&wideFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
                                    dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    log_misc(HOOKS_LOGGER, "^-> CreateFileA(%s) -> 0x%p", lpFileName, result);
    return result;
}

BOOL WINAPI FakeDeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize,
                                LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned,
                                LPOVERLAPPED lpOverlapped) {
    log_misc(HOOKS_LOGGER, "DeviceIoControl(0x%p, 0x%08x, 0x%p, 0x%x, -, 0x%x, 0, 0)", hDevice, dwIoControlCode,
             lpInBuffer, nInBufferSize, nOutBufferSize);

    file_hook_t* hook = file_hook_list;
    while (hook != NULL) {
        if (*hook->virtual_handle == hDevice) {
            if (hook->DeviceIoControl) {
                // TODO: Less jank
                if (lpOverlapped != NULL) SetEvent(lpOverlapped->hEvent);

                return hook->DeviceIoControl(dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize,
                                             lpBytesReturned, lpOverlapped);
            } else {
                log_error(HOOKS_LOGGER, "DeviceIoControl(%ls) unimplemented", hook->filename);
                return FALSE;
            }
        }
        hook = hook->next;
    }

    // log_warning(HOOKS_LOGGER, "DeviceIoControl(0x%p, 0x%08x, -, -, -, -, 0, 0)", hDevice, dwIoControlCode);

    return TrueDeviceIoControl(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize,
                               lpBytesReturned, lpOverlapped);
}

DWORD WINAPI FakeSetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod) {
    file_hook_t* hook = file_hook_list;
    while (hook != NULL) {
        if (*hook->virtual_handle == hFile) {
            if (hook->SetFilePointer) {
                return hook->SetFilePointer(lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
            } else {
                log_error(HOOKS_LOGGER, "SetFilePointer(%ls) unimplemented", hook->filename);
                return FALSE;
            }
        }
        hook = hook->next;
    }

    return TrueSetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
}

DWORD WINAPI FakeWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten,
                           LPOVERLAPPED lpOverlapped) {
    file_hook_t* hook = file_hook_list;
    while (hook != NULL) {
        if (*hook->virtual_handle == hFile) {
            if (hook->WriteFile) {
                return hook->WriteFile(lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
            } else {
                log_error(HOOKS_LOGGER, "WriteFile(%ls) unimplemented", hook->filename);
                return FALSE;
            }
        }
        hook = hook->next;
    }

    return TrueWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

BOOL WINAPI FakeReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead,
                         LPOVERLAPPED lpOverlapped) {
    file_hook_t* hook = file_hook_list;
    while (hook != NULL) {
        if (*hook->virtual_handle == hFile) {
            if (hook->ReadFile) {
                return hook->ReadFile(lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
            } else {
                log_error(HOOKS_LOGGER, "ReadFile(%ls) unimplemented", hook->filename);
                return FALSE;
            }
        }
        hook = hook->next;
    }

    return TrueReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}

BOOL WINAPI FakeCloseHandle(HANDLE hObject) {
    file_hook_t* hook = file_hook_list;
    while (hook != NULL) {
        if (hook->virtual_handle == hObject) {
            return TRUE;
        }
        hook = hook->next;
    }
    return TrueCloseHandle(hObject);
}

int WINAPIV Fake_stat64i32(const char* path, struct _stat64i32* buffer) {
    path = redirect_path((char*)path);
    return True_stat64i32(path, buffer);
};

void hook_io() {
    hook("Kernel32.dll", "DeviceIoControl", FakeDeviceIoControl, (void**)&TrueDeviceIoControl, 5);

    hook("Kernel32.dll", "CreateFileA", FakeCreateFileA, (void**)&TrueCreateFileA, 6);
    hook("Kernel32.dll", "CreateFileW", FakeCreateFileW, (void**)&TrueCreateFileW, 6);

    hook("Kernel32.dll", "CloseHandle", FakeCloseHandle, (void**)&TrueCloseHandle, 6);
    hook("Kernel32.dll", "SetFilePointer", FakeSetFilePointer, (void**)&TrueSetFilePointer, 6);
    hook("Kernel32.dll", "WriteFile", FakeWriteFile, (void**)&TrueWriteFile, 6);
    hook("Kernel32.dll", "ReadFile", FakeReadFile, (void**)&TrueReadFile, 6);

    hook("MSVCR90.DLL", "_stat64i32", Fake_stat64i32, (void**)&True_stat64i32, 5);
}
