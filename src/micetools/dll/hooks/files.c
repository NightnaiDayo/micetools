#define _MICE_FILES
#include "files.h"

HANDLE open_hook(file_hook_t* file_hook) {
    open_hook_t* opened = (open_hook_t*)malloc(sizeof(open_hook_t));
    ZeroMemory(opened, sizeof *opened);

    opened->hook = file_hook;

    HANDLE handle = GetDummyHandle();
    opened->ctx.m_Handle = handle;
    opened->ctx.m_HookData = file_hook->hook_data;

    SetDataForHandle(handle, HDATA_FILE, opened, TRUE);

    return handle;
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

struct buffer_file {
    LPBYTE buffer;
    DWORD nBytes;
    DWORD access;
};
BOOL bf_ReadFile(file_context_t* ctx, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                 LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    struct buffer_file* pBF = (struct buffer_file*)ctx->m_HookData;
    if (!(pBF->access & GENERIC_READ)) {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    if (ctx->m_Pointer.QuadPart > pBF->nBytes) {
        *lpNumberOfBytesRead = 0;
        return TRUE;
    }
    if (ctx->m_Pointer.QuadPart + nNumberOfBytesToRead > pBF->nBytes) {
        nNumberOfBytesToRead = (pBF->nBytes - ctx->m_Pointer.QuadPart) & 0xffffffff;
    }
    *lpNumberOfBytesRead = nNumberOfBytesToRead;
    memcpy(lpBuffer, pBF->buffer + ctx->m_Pointer.QuadPart, nNumberOfBytesToRead);
    return TRUE;
}
BOOL bf_WriteFile(file_context_t* ctx, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                  LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    struct buffer_file* pBF = (struct buffer_file*)ctx->m_HookData;
    if (!(pBF->access & GENERIC_WRITE)) {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    if (ctx->m_Pointer.QuadPart > pBF->nBytes) {
        *lpNumberOfBytesWritten = 0;
        return TRUE;
    }
    if (ctx->m_Pointer.QuadPart + nNumberOfBytesToWrite > pBF->nBytes) {
        nNumberOfBytesToWrite = (pBF->nBytes - ctx->m_Pointer.QuadPart) & 0xffffffff;
    }
    *lpNumberOfBytesWritten = nNumberOfBytesToWrite;
    memcpy(pBF->buffer + ctx->m_Pointer.QuadPart, lpBuffer, nNumberOfBytesToWrite);
    return TRUE;
}

void hook_file_with_buffer(LPCWSTR filename, LPBYTE buffer, DWORD nBytes, DWORD access) {
    file_hook_t* hook = new_file_hook(filename);

    struct buffer_file** ppBF = &((struct buffer_file*)hook->hook_data);
    *ppBF = malloc(sizeof **ppBF);
    (*ppBF)->buffer = buffer;
    (*ppBF)->nBytes = nBytes;
    (*ppBF)->access = access;

    hook->ReadFile = bf_ReadFile;
    hook->WriteFile = bf_WriteFile;

    hook_file(hook);
}

drive_redirect_t DRIVE_REDIRECT_TABLE[] = {
    // Note: Had to create last_shime.log
    { .drive = "C:\\Documents and Settings\\AppUser\\temp\\", .path = ".\\dev\\temp\\" },
    // {.drive = "C:\\ProgramData/boost_interprocess/", .path = "\\\\.\\ipc\\"},
};

char _redirected_path[MAX_PATH];

file_hook_t* find_hook(LPCWSTR lpFileName) {
    file_hook_t* file_hook = file_hook_list;
    while (file_hook != NULL) {
        if (wcscmp(lpFileName, file_hook->filename) == 0 ||
            (file_hook->altFilename != NULL && wcscmp(lpFileName, file_hook->altFilename) == 0)) {
            return file_hook;
        }
        file_hook = file_hook->next;
    }
    return NULL;
};

char WORKING_DIR[MAX_PATH + 1] = { 0 };
char get_gamedata_drive() {
    if (WORKING_DIR[0] == 0x00) {
        GetCurrentDirectoryA(sizeof WORKING_DIR, WORKING_DIR);
    }
    return WORKING_DIR[0];
}

inline char char_lower(char value) {
    if ('A' <= value && value <= 'Z') return value - 'A' + 'a';
    return value;
}

BOOL redirect_path(LPCSTR path, LPCSTR* redirected) {
    for (int i = 0; i < sizeof DRIVE_REDIRECT_TABLE / sizeof DRIVE_REDIRECT_TABLE[0]; i++) {
        drive_redirect_t row = DRIVE_REDIRECT_TABLE[i];
        if (strncmp(path, row.drive, strlen(row.drive)) == 0) {
            log_trace(HOOKS_LOGGER, "Redirecting '%s' to '%s'", path, row.path);

            size_t new_len = strlen(path) - strlen(row.drive) + strlen(row.path);
            strcpy_s(_redirected_path, new_len + 1, row.path);

            char* dst = _redirected_path + strlen(row.path);
            size_t len = strlen(path) - strlen(row.drive);
            const char* src = path + strlen(row.drive);

            for (; len > 0; len--) (dst++)[0] = (src++)[0];
            dst[0] = 0;
            log_trace(HOOKS_LOGGER, "New filename: '%s'", _redirected_path);

            make_dirs(_redirected_path);
            *redirected = _redirected_path;
            return TRUE;
        }
    }

    // Don't redirect local paths
    GetCurrentDirectoryA(sizeof WORKING_DIR, WORKING_DIR);
    if (strstr(path, WORKING_DIR) == path) return FALSE;

    if ((('a' <= path[0] && path[0] <= 'z') || ('A' <= path[0] && path[0] <= 'Z')) &&
        path[1] == ':' && (path[2] == '/' || path[2] == '\\')) {
        ZeroMemory(_redirected_path, sizeof _redirected_path);
        snprintf(_redirected_path, sizeof _redirected_path, "dev\\%c\\%s", char_lower(path[0]),
                 path + 3);

        make_dirs(_redirected_path);
        *redirected = _redirected_path;
        return TRUE;
    }

    return FALSE;
}

BOOL redirect_path_w(LPCWSTR lpFileName, LPCSTR* redirected) {
    char cFileName[MAX_PATH] = { 0 };
    WideCharToMultiByte(CP_ACP, 0, lpFileName, wcslen(lpFileName), cFileName, sizeof cFileName,
                        NULL, NULL);
    return redirect_path(cFileName, redirected);
}

HANDLE WINAPI FakeCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                              LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                              DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                              HANDLE hTemplateFile) {
    file_hook_t* found_fh = find_hook(lpFileName);
    if (found_fh != NULL) {
        HANDLE handle = open_hook(found_fh);
        log_info(HOOKS_LOGGER, "CreateFileW(%ls) -> 0x%p", lpFileName, handle);
        return handle;
    }

    HANDLE handle;

    LPCSTR redirected;
    if (redirect_path_w(lpFileName, &redirected)) {
        handle = TrueCreateFileA(redirected, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
                                 dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
        log_misc(HOOKS_LOGGER, "CreateFileW(%s) -> 0x%p", redirected, handle);
    } else {
        handle = TrueCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
                                 dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
        log_misc(HOOKS_LOGGER, "CreateFileW(%ls) -> 0x%p", lpFileName, handle);
    }

    return handle;
}
HANDLE WINAPI FakeCreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                              LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                              DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                              HANDLE hTemplateFile) {
    WCHAR wideFileName[MAX_PATH + 1];
    MultiByteToWideChar(CP_ACP, 0, lpFileName, -1, (LPWSTR)&wideFileName, MAX_PATH + 1);

    file_hook_t* found_fh = find_hook(wideFileName);
    if (found_fh != NULL) {
        HANDLE handle = open_hook(found_fh);
        log_info(HOOKS_LOGGER, "CreateFileA(%s) -> 0x%p", lpFileName, handle);
        return handle;
    }

    redirect_path(lpFileName, &lpFileName);
    HANDLE handle = TrueCreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
                                    dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    log_misc(HOOKS_LOGGER, "CreateFileA(%s) -> 0x%p", lpFileName, handle);
    return handle;
}

BOOL WINAPI FakePathFileExistsA(LPCSTR pszPath) {
    log_misc(HOOKS_LOGGER, "PathFileExists(%s)", pszPath);
    redirect_path(pszPath, &pszPath);
    BOOL ret = TruePathFileExistsA(pszPath);
    return ret;
}
BOOL WINAPI FakePathFileExistsW(LPCWSTR pszPath) {
    log_misc(HOOKS_LOGGER, "PathFileExists(%ls)", pszPath);
    LPCSTR redirected;
    if (redirect_path_w(pszPath, &redirected)) {
        return TruePathFileExistsA(redirected);
    }
    return TruePathFileExistsW(pszPath);
}

BOOL WINAPI FakeDeleteFileA(LPCSTR pszPath) {
    LPCSTR redirected;
    if (redirect_path(pszPath, &redirected)) {
        return TrueDeleteFileA(redirected);
    }
    return TrueDeleteFileA(pszPath);
}
BOOL WINAPI FakeDeleteFileW(LPCWSTR pszPath) {
    LPCSTR redirected;
    if (redirect_path_w(pszPath, &redirected)) {
        return TrueDeleteFileA(redirected);
    }
    return TrueDeleteFileW(pszPath);
}

BOOL WINAPI FakeDeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer,
                                DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                                LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    open_hook_t* pHData = GetDataForHandle(hDevice, HDATA_FILE);
    if (pHData == NULL) {
        log_trace(HOOKS_LOGGER, "DeviceIoControl(0x%p, 0x%08x, 0x%p, 0x%x, -, 0x%x, 0, 0)", hDevice,
                  dwIoControlCode, lpInBuffer, nInBufferSize, nOutBufferSize);

        return TrueDeviceIoControl(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer,
                                   nOutBufferSize, lpBytesReturned, lpOverlapped);
    }

    file_hook_t* file_hook = pHData->hook;
    if (!file_hook->DeviceIoControl) {
        log_error(HOOKS_LOGGER, "DeviceIoControl(%ls) unimplemented", file_hook->filename);
        return FALSE;
    }

    BOOL ret =
        file_hook->DeviceIoControl(&(pHData->ctx), dwIoControlCode, lpInBuffer, nInBufferSize,
                                   lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);
    if (lpOverlapped) {
        SetEvent(lpOverlapped->hEvent);
        if (ret && lpBytesReturned) {
            lpOverlapped->InternalHigh = *lpBytesReturned;
        }
    }
    return ret;
}

DWORD WINAPI FakeSetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh,
                                DWORD dwMoveMethod) {
    open_hook_t* pHData = GetDataForHandle(hFile, HDATA_FILE);
    if (pHData == NULL)
        return TrueSetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);

    if (dwMoveMethod == FILE_BEGIN) {
        if (*lpDistanceToMoveHigh)
            pHData->ctx.m_Pointer.HighPart = *lpDistanceToMoveHigh;
        else
            pHData->ctx.m_Pointer.HighPart = 0;
        pHData->ctx.m_Pointer.LowPart = lDistanceToMove;
    } else if (dwMoveMethod == FILE_END) {
        log_error("files", "FILE_END unimplemented");
        return 0xFFFFFFFF;
    } else {
        if (lpDistanceToMoveHigh) pHData->ctx.m_Pointer.HighPart += *lpDistanceToMoveHigh;
        pHData->ctx.m_Pointer.LowPart += lDistanceToMove;
    }

    return pHData->ctx.m_Pointer.LowPart;
}

BOOL WINAPI FakeSetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove,
                                 PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod) {
    open_hook_t* pHData = GetDataForHandle(hFile, HDATA_FILE);
    if (pHData != NULL) {
        if (dwMoveMethod == FILE_BEGIN) {
            pHData->ctx.m_Pointer = liDistanceToMove;
        } else if (dwMoveMethod == FILE_END) {
            log_error("files", "FILE_END unimplemented");
            return FALSE;
        } else {
            pHData->ctx.m_Pointer.QuadPart += liDistanceToMove.QuadPart;
        }
        if (lpNewFilePointer) lpNewFilePointer->QuadPart = pHData->ctx.m_Pointer.QuadPart;

        return TRUE;
    }

    return TrueSetFilePointerEx(hFile, liDistanceToMove, lpNewFilePointer, dwMoveMethod);
}

DWORD WINAPI FakeGetFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize) {
    open_hook_t* pHData = GetDataForHandle(hFile, HDATA_FILE);
    if (pHData == NULL) {
        return TrueGetFileSizeEx(hFile, lpFileSize);
    }
    file_hook_t* file_hook = pHData->hook;
    if (!file_hook->GetFileSizeEx) {
        log_error(HOOKS_LOGGER, "GetFileSizeEx(%ls) unimplemented", file_hook->filename);
        return FALSE;
    }
    return file_hook->GetFileSizeEx(&(pHData->ctx), lpFileSize);
}

DWORD WINAPI FakeWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                           LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    if (hFile == GetStdHandle(STD_OUTPUT_HANDLE) || hFile == GetStdHandle(STD_ERROR_HANDLE)) {
        return TrueWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten,
                             lpOverlapped);
    }

    open_hook_t* pHData = GetDataForHandle(hFile, HDATA_FILE);
    if (pHData == NULL) {
        return TrueWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten,
                             lpOverlapped);
    }

    file_hook_t* file_hook = pHData->hook;
    if (!file_hook->WriteFile) {
        log_error(HOOKS_LOGGER, "WriteFile(%ls) unimplemented", file_hook->filename);
        return FALSE;
    }
    BOOL ret = file_hook->WriteFile(&(pHData->ctx), lpBuffer, nNumberOfBytesToWrite,
                                    lpNumberOfBytesWritten, lpOverlapped);
    if (ret) pHData->ctx.m_Pointer.QuadPart += *lpNumberOfBytesWritten;
    return ret;
}

BOOL WINAPI FakeReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                         LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    if (hFile == GetStdHandle(STD_INPUT_HANDLE)) {
        return TrueReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead,
                            lpOverlapped);
    }
    // log_misc(HOOKS_LOGGER, "ReadFile(%d) %d", hFile, nNumberOfBytesToRead);

    open_hook_t* pHData = GetDataForHandle(hFile, HDATA_FILE);
    if (pHData == NULL) {
        return TrueReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead,
                            lpOverlapped);
    }

    file_hook_t* file_hook = pHData->hook;

    if (!file_hook->ReadFile) {
        log_error(HOOKS_LOGGER, "ReadFile(%ls) unimplemented", file_hook->filename);
        return FALSE;
    }
    BOOL ret = file_hook->ReadFile(&(pHData->ctx), lpBuffer, nNumberOfBytesToRead,
                                   lpNumberOfBytesRead, lpOverlapped);
    if (ret) pHData->ctx.m_Pointer.QuadPart += *lpNumberOfBytesRead;
    return ret;
}

BOOL WINAPI FakeCloseHandle(HANDLE hObject) {
    RemoveDataForHandle(hObject, HDATA_ANY);
    return TrueCloseHandle(hObject);
}

int WINAPIV Fake_stat64i32(const char* path, struct _stat64i32* buffer) {
    redirect_path((char*)path, &path);
    return True_stat64i32(path, buffer);
};

DWORD WINAPI FakeGetFileAttributesA(LPCSTR lpFileName) {
    // The game quits out if MiniDump is present!
    if (strcmp(lpFileName, "Y:\\MiniDump\\") == 0) {
        return 0;
    }

    LPCSTR redirected;
    if (redirect_path(lpFileName, &redirected)) {
        return TrueGetFileAttributesA(redirected);
    }
    return TrueGetFileAttributesA(lpFileName);
}
DWORD WINAPI FakeGetFileAttributesW(LPCWSTR lpFileName) {
    LPCSTR redirected;
    if (redirect_path_w(lpFileName, &redirected)) {
        return TrueGetFileAttributesA(redirected);
    }
    return TrueGetFileAttributesW(lpFileName);
}

void hook_io() {
    hook("Kernel32.dll", "DeviceIoControl", FakeDeviceIoControl, (void**)&TrueDeviceIoControl);

    hook("Kernel32.dll", "CreateFileA", FakeCreateFileA, (void**)&TrueCreateFileA);
    hook("Kernel32.dll", "CreateFileW", FakeCreateFileW, (void**)&TrueCreateFileW);

    hook("Kernel32.dll", "CloseHandle", FakeCloseHandle, (void**)&TrueCloseHandle);
    hook("Kernel32.dll", "SetFilePointer", FakeSetFilePointer, (void**)&TrueSetFilePointer);
    hook("Kernel32.dll", "SetFilePointerEx", FakeSetFilePointerEx, (void**)&TrueSetFilePointerEx);
    hook("Kernel32.dll", "WriteFile", FakeWriteFile, (void**)&TrueWriteFile);
    hook("Kernel32.dll", "ReadFile", FakeReadFile, (void**)&TrueReadFile);
    hook("Kernel32.dll", "GetFileSizeEx", FakeGetFileSizeEx, (void**)&TrueGetFileSizeEx);

    hook("Shlwapi.dll", "PathFileExistsA", FakePathFileExistsA, (void**)&TruePathFileExistsA);
    hook("Shlwapi.dll", "PathFileExistsW", FakePathFileExistsW, (void**)&TruePathFileExistsW);
    hook("Kernel32.dll", "DeleteFileA", FakeDeleteFileA, (void**)&TrueDeleteFileA);
    hook("Kernel32.dll", "DeleteFileW", FakeDeleteFileW, (void**)&TrueDeleteFileW);

    hook("Kernel32.dll", "GetFileAttributesA", FakeGetFileAttributesA,
         (void**)&TrueGetFileAttributesA);
    hook("Kernel32.dll", "GetFileAttributesW", FakeGetFileAttributesW,
         (void**)&TrueGetFileAttributesW);

    hook("MSVCR90.DLL", "_stat64i32", Fake_stat64i32, (void**)&True_stat64i32);
}
