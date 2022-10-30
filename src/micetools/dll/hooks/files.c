#include "files.h"

open_hook_t* open_hooks_list = NULL;
HANDLE open_hook(file_hook_t* file_hook, com_hook_t* com_hook) {
    open_hook_t* opened = (open_hook_t*)malloc(sizeof(open_hook_t));
    memset(opened, 0, sizeof *opened);

    opened->file_hook = file_hook;
    opened->com_hook = com_hook;

    CHAR path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    HANDLE handle =
        _CreateFileA(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        log_error(HOOKS_LOGGER, "Failed to create dummy handle: %03x", GetLastError());
        return INVALID_HANDLE_VALUE;
    }
    opened->handle = handle;
    opened->next = open_hooks_list;
    open_hooks_list = opened;

    return handle;
}
void close_hook(HANDLE handle) {
    if (handle == INVALID_HANDLE_VALUE) return;

    _CloseHandle(handle);

    open_hook_t* opened = NULL;
    open_hook_t* root = open_hooks_list;
    if (open_hooks_list->handle == handle) {
        open_hook_t* next = open_hooks_list->next;
        free(open_hooks_list);
        open_hooks_list = next;
        return;
    }

    while (root != NULL) {
        if (root->next && root->next->handle == handle) {
            opened = root->next;
            root->next = opened->next;
            free(opened);
            return;
        }
        root = root->next;
    }
}
file_hook_t* get_handle_file_hook(HANDLE handle) {
    open_hook_t* root = open_hooks_list;
    while (root != NULL) {
        if (root->handle == handle) return root->file_hook;
        root = root->next;
    }
    return NULL;
}
com_hook_t* get_handle_com_hook(HANDLE handle) {
    open_hook_t* root = open_hooks_list;
    while (root != NULL) {
        if (root->handle == handle) return root->com_hook;
        root = root->next;
    }
    return NULL;
}

file_hook_t* file_hook_list = NULL;
file_hook_t* new_file_hook(LPCWSTR filename) {
    file_hook_t* hook = (file_hook_t*)malloc(sizeof(file_hook_t));
    memset(hook, 0, sizeof *hook);

    hook->filename = filename;

    return hook;
}
void hook_file(file_hook_t* hook) {
    hook->next = NULL;
    if (file_hook_list == NULL) {
        file_hook_list = hook;
        return;
    }

    file_hook_t* hl = file_hook_list;
    while (hl->next != NULL) hl = hl->next;
    hl->next = hook;
};

drive_redirect_t DRIVE_REDIRECT_TABLE[] = {
    { .drive = "Y:\\", .path = ".\\dev\\Y\\" },
    // Note: Had tp create last_shime.log
    { .drive = "C:\\Documents and Settings\\AppUser\\temp\\", .path = ".\\dev\\temp\\" },
    // {.drive = "C:\\ProgramData/boost_interprocess/", .path = "\\\\.\\ipc\\"},

    { .drive = "C:\\\\Windows\\\\System32\\", .path = "Sys32" },
    { .drive = "C:\\\\WINDOWS\\\\system32\\", .path = "Sys32" },
};

LPCSTR redirect_path(LPCSTR path) {
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
            const char* src = path + strlen(row.drive);

            for (; len > 0; len--) (dst++)[0] = (src++)[0];
            dst[0] = 0;
            log_misc(HOOKS_LOGGER, "New filename: '%s'", new_str);
            return new_str;
        }
    }
    return path;
}
LPCWSTR redirect_path_w(LPCWSTR path) {
    return path;
    // TODO: THIS!!
    for (int i = 0; i < sizeof DRIVE_REDIRECT_TABLE / sizeof DRIVE_REDIRECT_TABLE[0]; i++) {
        drive_redirect_t row = DRIVE_REDIRECT_TABLE[i];
        if (wcsstr(path, row.drive)) {
            log_misc(HOOKS_LOGGER, "Redirecting '%ls' to '%ls'", path, row.path);

            size_t new_len = wcslen(path) - wcslen(row.drive) + wcslen(row.path);
            // TODO: Make this not leak memory!
            wchar_t* new_str = (wchar_t*)malloc((new_len + 1) * 2);
            wcscpy_s(new_str, new_len + 1, row.path);

            wchar_t* dst = new_str + wcslen(row.path) * 2;
            size_t len = wcslen(path) - wcslen(row.drive);
            const wchar_t* src = path + wcslen(row.drive) * 2;

            for (; len > 0; len--) {
                (dst++)[0] = (src++)[0];
                (dst++)[0] = (src++)[0];
            }
            dst[0] = 0;
            log_misc(HOOKS_LOGGER, "New filename: '%ls'", new_str);
            return new_str;
        }
    }
    return path;
}

void find_hooks(LPCWSTR lpFileName, file_hook_t** found_fh, com_hook_t** found_ch) {
    file_hook_t* file_hook = file_hook_list;
    while (file_hook != NULL) {
        if (wcscmp(lpFileName, file_hook->filename) == 0 ||
            (file_hook->altFilename != NULL && wcscmp(lpFileName, file_hook->altFilename) == 0)) {
            *found_fh = file_hook;
            break;
        }
        file_hook = file_hook->next;
    }

    com_hook_t* com_hook = com_hook_list;
    while (com_hook != NULL) {
        if (wcscmp(lpFileName, com_hook->wName) == 0 ||
            wcscmp(lpFileName, com_hook->wDosName) == 0) {
            *found_ch = com_hook;
            break;
        }
        com_hook = com_hook->next;
    }
};

HANDLE WINAPI FakeCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                              LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                              DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                              HANDLE hTemplateFile) {
    file_hook_t* found_fh = NULL;
    com_hook_t* found_ch = NULL;
    find_hooks(lpFileName, &found_fh, &found_ch);
    if (found_fh != NULL || found_ch != NULL) {
        HANDLE handle = open_hook(found_fh, found_ch);
        log_info(HOOKS_LOGGER, "CreateFileW(%ls) -> 0x%p", lpFileName, handle);
        return handle;
    }

    lpFileName = redirect_path_w(lpFileName);

    HANDLE handle = TrueCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
                                    dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    log_misc(HOOKS_LOGGER, "CreateFileW(%ls) -> 0x%p", lpFileName, handle);
    return handle;
}
HANDLE WINAPI FakeCreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                              LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                              DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                              HANDLE hTemplateFile) {
    WCHAR wideFileName[MAX_PATH + 1];
    MultiByteToWideChar(CP_ACP, 0, lpFileName, -1, (LPWSTR)&wideFileName, MAX_PATH + 1);

    file_hook_t* found_fh = NULL;
    com_hook_t* found_ch = NULL;
    find_hooks(wideFileName, &found_fh, &found_ch);
    if (found_fh != NULL || found_ch != NULL) {
        HANDLE handle = open_hook(found_fh, found_ch);
        log_info(HOOKS_LOGGER, "CreateFileA(%s) -> 0x%p", lpFileName, handle);
        return handle;
    }

    lpFileName = redirect_path(lpFileName);

    HANDLE handle = TrueCreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
                                    dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    log_misc(HOOKS_LOGGER, "CreateFileA(%s) -> 0x%p", lpFileName, handle);
    return handle;

    // lpFileName = redirect_path(lpFileName);

    // HANDLE result =
    //     FakeCreateFileW((LPCWSTR)&wideFileName, dwDesiredAccess, dwShareMode,
    //     lpSecurityAttributes,
    //                     dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    // log_trace(HOOKS_LOGGER, "^-> CreateFileA(%s) -> 0x%p", lpFileName, result);
    // return result;
}

BOOL WINAPI FakeDeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer,
                                DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                                LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    file_hook_t* hook = get_handle_file_hook(hDevice);
    if (hook != NULL) {
        if (hook->DeviceIoControl) {
            // TODO: Less jank
            if (lpOverlapped != NULL) SetEvent(lpOverlapped->hEvent);

            BOOL ret =
                hook->DeviceIoControl(hook->data, dwIoControlCode, lpInBuffer, nInBufferSize,
                                      lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);
            if (ret && lpOverlapped && lpBytesReturned) {
                lpOverlapped->InternalHigh = *lpBytesReturned;
            }
            return ret;
        } else {
            log_error(HOOKS_LOGGER, "DeviceIoControl(%ls) unimplemented", hook->filename);
            return FALSE;
        }
    }

    log_trace(HOOKS_LOGGER, "DeviceIoControl(0x%p, 0x%08x, 0x%p, 0x%x, -, 0x%x, 0, 0)", hDevice,
              dwIoControlCode, lpInBuffer, nInBufferSize, nOutBufferSize);

    return TrueDeviceIoControl(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer,
                               nOutBufferSize, lpBytesReturned, lpOverlapped);
}

DWORD WINAPI FakeSetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh,
                                DWORD dwMoveMethod) {
    file_hook_t* hook = get_handle_file_hook(hFile);
    if (hook != NULL) {
        if (hook->SetFilePointer) {
            return hook->SetFilePointer(hook->data, lDistanceToMove, lpDistanceToMoveHigh,
                                        dwMoveMethod);
        } else {
            log_error(HOOKS_LOGGER, "SetFilePointer(%ls) unimplemented", hook->filename);
            return FALSE;
        }
    }

    return TrueSetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
}

BOOL WINAPI FakeSetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove,
                                 PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod) {
    file_hook_t* hook = get_handle_file_hook(hFile);
    if (hook != NULL) {
        if (hook->SetFilePointerEx) {
            return hook->SetFilePointerEx(hook->data, liDistanceToMove, lpNewFilePointer,
                                          dwMoveMethod);
        } else {
            log_error(HOOKS_LOGGER, "SetFilePointerEx(%ls) unimplemented", hook->filename);
            return FALSE;
        }
    }

    return TrueSetFilePointerEx(hFile, liDistanceToMove, lpNewFilePointer, dwMoveMethod);
}

DWORD WINAPI FakeGetFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize) {
    file_hook_t* hook = get_handle_file_hook(hFile);
    if (hook != NULL) {
        if (hook->GetFileSizeEx) {
            return hook->GetFileSizeEx(hook->data, lpFileSize);
        } else {
            log_error(HOOKS_LOGGER, "GetFileSizeEx(%ls) unimplemented", hook->filename);
            return FALSE;
        }
    }

    return TrueGetFileSizeEx(hFile, lpFileSize);
}

DWORD WINAPI FakeWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                           LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    log_trace("file", "WriteFile(%08x)", hFile);
    file_hook_t* hook = get_handle_file_hook(hFile);
    if (hook != NULL) {
        if (hook->WriteFile) {
            return hook->WriteFile(hook->data, lpBuffer, nNumberOfBytesToWrite,
                                   lpNumberOfBytesWritten, lpOverlapped);
        } else {
            log_error(HOOKS_LOGGER, "WriteFile(%ls) unimplemented", hook->filename);
            return FALSE;
        }
    }

    return FALSE;

    return TrueWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten,
                         lpOverlapped);
}

BOOL WINAPI FakeReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                         LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    file_hook_t* hook = get_handle_file_hook(hFile);
    if (hook != NULL) {
        if (hook->ReadFile) {
            return hook->ReadFile(hook->data, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead,
                                  lpOverlapped);
        } else {
            log_error(HOOKS_LOGGER, "ReadFile(%ls) unimplemented", hook->filename);
            return FALSE;
        }
    }

    return TrueReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}

BOOL WINAPI FakeCloseHandle(HANDLE hObject) {
    file_hook_t* hook = get_handle_file_hook(hObject);
    if (hook != NULL) {
        log_misc("file", "close %08x", hObject);
        close_hook(hObject);
        return TRUE;
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
    hook("Kernel32.dll", "SetFilePointerEx", FakeSetFilePointerEx, (void**)&TrueSetFilePointerEx,
         6);
    hook("Kernel32.dll", "WriteFile", FakeWriteFile, (void**)&TrueWriteFile, 6);
    hook("Kernel32.dll", "ReadFile", FakeReadFile, (void**)&TrueReadFile, 6);
    hook("Kernel32.dll", "GetFileSizeEx", FakeGetFileSizeEx, (void**)&TrueGetFileSizeEx, 6);

    hook("MSVCR90.DLL", "_stat64i32", Fake_stat64i32, (void**)&True_stat64i32, 5);
}
