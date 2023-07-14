#define _MICE_FILES
#include "files.h"

#include "../util/_util.h"

static const bool ALLOW_FILE_PASSTHROUGH = true;

// TODO: This should be part of MiceFS I think
file_hook_t* MiceFSLocateHookW(_In_ LPCWSTR lpFileName);
file_hook_t* MiceFSLocateHookW(LPCWSTR lpFileName) {
    file_hook_t* file_hook = file_hook_list;
    while (file_hook != NULL) {
        if (_wcsicmp(lpFileName, file_hook->filename) == 0 ||
            (file_hook->altFilename != NULL && _wcsicmp(lpFileName, file_hook->altFilename) == 0)) {
            return file_hook;
        }
        file_hook = file_hook->next;
    }
    return NULL;
}

HANDLE open_hook(file_hook_t* file_hook) {
    open_hook_t* opened = (open_hook_t*)malloc(sizeof(open_hook_t));
    if (opened == NULL) return INVALID_HANDLE_VALUE;
    ZeroMemory(opened, sizeof *opened);

    opened->hook = file_hook;

    HANDLE handle = GetDummyHandle();
    opened->ctx.m_Handle = handle;
    opened->ctx.m_HookData = file_hook->hook_data;

    SetDataForHandle(handle, opened, TRUE);

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
}
file_hook_t* unhook_file(LPCWSTR lpFilename) {
    file_hook_t* hl = file_hook_list;
    if (hl == NULL) return NULL;
    file_hook_t* previous = NULL;
    do {
        if (_wcsicmp(hl->filename, lpFilename) == 0 || _wcsicmp(hl->altFilename, lpFilename) == 0) {
            if (previous == NULL)
                file_hook_list = hl->next;
            else
                previous->next = hl->next;
            return hl;
        }

        previous = hl;
        hl = hl->next;
    } while (hl != NULL);
    return NULL;
}

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

HANDLE WINAPI FakeCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                              LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                              DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                              HANDLE hTemplateFile) {
    size_t len = wcslen(lpFileName);
    if (wcscmp(&lpFileName[len - 4], L".dbg") == 0 || wcscmp(&lpFileName[len - 4], L".pdb") == 0) {
        return TrueCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
                               dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    }

    file_hook_t* found_fh = MiceFSLocateHookW(lpFileName);
    if (found_fh != NULL) {
        HANDLE handle = open_hook(found_fh);
        log_info(plfHooks, "CreateFileW(%ls) -> 0x%p", lpFileName, handle);
        return handle;
    }
    if (!ALLOW_FILE_PASSTHROUGH) {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
    }

    HANDLE handle;

    MiceFSRedirectPathW(lpFileName, &lpFileName);
    handle = TrueCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
                             dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    if (handle == INVALID_HANDLE_VALUE) {
        if (!(_wcsicmp(lpFileName, L"C:\\windows\\system32\\atipblag.dat") == 0 ||
              wcsncmp(lpFileName, L"\\\\?\\hid#", 8) == 0))
            log_warning(plfHooks, "CreateFileW(%ls) failed", lpFileName);
    } else {
        log_info(plfHooks, "CreateFileW(%ls) -> 0x%p", lpFileName, handle);
    }
    return handle;
}
HANDLE WINAPI FakeCreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                              LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                              DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                              HANDLE hTemplateFile) {
    WCHAR wideFileName[MAX_PATH + 1];
    MultiByteToWideChar(CP_ACP, 0, lpFileName, -1, (LPWSTR)&wideFileName, MAX_PATH + 1);

    file_hook_t* found_fh = MiceFSLocateHookW(wideFileName);
    if (found_fh != NULL) {
        HANDLE handle = open_hook(found_fh);
        log_info(plfHooks, "CreateFileA(%s) -> 0x%p", lpFileName, handle);
        return handle;
    }
    if (!ALLOW_FILE_PASSTHROUGH) {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
    }

    MiceFSRedirectPathA(lpFileName, &lpFileName);
    HANDLE handle = TrueCreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
                                    dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    if (handle == INVALID_HANDLE_VALUE) {
        if (_stricmp(lpFileName, "C:\\windows\\system32\\atipblag.dat") != 0)
            log_warning(plfHooks, "CreateFileA(%s) failed", lpFileName);
    } else
        log_misc(plfHooks, "CreateFileA(%s) -> 0x%p", lpFileName, handle);
    return handle;
}

BOOL WINAPI FakeDeleteFileA(LPCSTR pszPath) {
    if (!ALLOW_FILE_PASSTHROUGH) return FALSE;

    MiceFSRedirectPathA(pszPath, &pszPath);
    return TrueDeleteFileA(pszPath);
}
BOOL WINAPI FakeDeleteFileW(LPCWSTR pszPath) {
    if (!ALLOW_FILE_PASSTHROUGH) return FALSE;

    MiceFSRedirectPathW(pszPath, &pszPath);
    return TrueDeleteFileW(pszPath);
}

BOOL WINAPI FakeDeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer,
                                DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                                LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    open_hook_t* pHData = GetDataForHandle(hDevice);
    if (pHData == NULL) {
        return TrueDeviceIoControl(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer,
                                   nOutBufferSize, lpBytesReturned, lpOverlapped);
    }

    file_hook_t* file_hook = pHData->hook;
    if (!file_hook->DeviceIoControl) {
        log_error(plfHooks, "DeviceIoControl(%ls) unimplemented", file_hook->filename);
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
    open_hook_t* pHData = GetDataForHandle(hFile);
    if (pHData == NULL)
        return TrueSetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);

    if (dwMoveMethod == FILE_BEGIN) {
        if (*lpDistanceToMoveHigh)
            pHData->ctx.m_Pointer.HighPart = *lpDistanceToMoveHigh;
        else
            pHData->ctx.m_Pointer.HighPart = 0;
        pHData->ctx.m_Pointer.LowPart = lDistanceToMove;
    } else if (dwMoveMethod == FILE_END) {
        log_error(plfFile, "FILE_END unimplemented");
        return 0xFFFFFFFF;
    } else {
        if (lpDistanceToMoveHigh) pHData->ctx.m_Pointer.HighPart += *lpDistanceToMoveHigh;
        pHData->ctx.m_Pointer.LowPart += lDistanceToMove;
    }

    return pHData->ctx.m_Pointer.LowPart;
}

BOOL WINAPI FakeSetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove,
                                 PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod) {
    open_hook_t* pHData = GetDataForHandle(hFile);
    if (pHData != NULL) {
        if (dwMoveMethod == FILE_BEGIN) {
            pHData->ctx.m_Pointer = liDistanceToMove;
        } else if (dwMoveMethod == FILE_END) {
            log_error(plfFile, "FILE_END unimplemented");
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
    open_hook_t* pHData = GetDataForHandle(hFile);
    if (pHData == NULL) {
        return TrueGetFileSizeEx(hFile, lpFileSize);
    }
    file_hook_t* file_hook = pHData->hook;
    if (!file_hook->GetFileSizeEx) {
        log_error(plfHooks, "GetFileSizeEx(%ls) unimplemented", file_hook->filename);
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

    open_hook_t* pHData = GetDataForHandle(hFile);
    if (pHData == NULL) {
        return TrueWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten,
                             lpOverlapped);
    }

    file_hook_t* file_hook = pHData->hook;
    if (!file_hook->WriteFile) {
        log_error(plfHooks, "WriteFile(%ls) unimplemented", file_hook->filename);
        return FALSE;
    }
    DWORD wrote;
    BOOL ret =
        file_hook->WriteFile(&(pHData->ctx), lpBuffer, nNumberOfBytesToWrite, &wrote, lpOverlapped);
    if (ret) pHData->ctx.m_Pointer.QuadPart += wrote;
    if (lpNumberOfBytesWritten) *lpNumberOfBytesWritten = wrote;
    return ret;
}

BOOL WINAPI FakeReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                         LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    if (hFile == GetStdHandle(STD_INPUT_HANDLE)) {
        return TrueReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead,
                            lpOverlapped);
    }
    // log_misc(plfHooks, "ReadFile(%d) %d", hFile, nNumberOfBytesToRead);

    open_hook_t* pHData = GetDataForHandle(hFile);
    if (pHData == NULL) {
        return TrueReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead,
                            lpOverlapped);
    }

    file_hook_t* file_hook = pHData->hook;

    if (!file_hook->ReadFile) {
        log_error(plfHooks, "ReadFile(%ls) unimplemented", file_hook->filename);
        return FALSE;
    }
    DWORD read;
    BOOL ret =
        file_hook->ReadFile(&(pHData->ctx), lpBuffer, nNumberOfBytesToRead, &read, lpOverlapped);
    if (ret) pHData->ctx.m_Pointer.QuadPart += read;
    if (lpNumberOfBytesRead) *lpNumberOfBytesRead = read;
    return ret;
}

BOOL WINAPI FakeCloseHandle(HANDLE hObject) {
    if ((DWORD_PTR)hObject & MICE_HANDLE_MASK) {
        RemoveDataForHandle(hObject);
        return TRUE;
    }
    return TrueCloseHandle(hObject);
}

void FileTimeToTimet(__time64_t* t, LPFILETIME pft) {
    ULARGE_INTEGER time_value;
    time_value.HighPart = pft->dwHighDateTime;
    time_value.LowPart = pft->dwLowDateTime;
    *t = (time_value.QuadPart - 116444736000000000LL) / 10000000LL;
}

int WINAPIV Fake_stat64i32(const char* path, _stat64i32_t* buffer) {
    MiceFSRedirectPathA(path, &path);

    // NOTE: We can't use True_stat64i32 because it has issues with another other than dir-relative
    // child paths

    WIN32_FILE_ATTRIBUTE_DATA fileInformation;
    if (!GetFileAttributesEx(path, GetFileExInfoStandard, &fileInformation)) return -1;

    // TODO: Populate other values. ALLNetProc and OpenSSL only need st_size. atime/ctime are free
    // so were added too.
    buffer->st_size = fileInformation.nFileSizeLow;
    FileTimeToTimet(&buffer->st_atime, &fileInformation.ftLastAccessTime);
    FileTimeToTimet(&buffer->st_ctime, &fileInformation.ftCreationTime);

    return 0x800;
}

DWORD WINAPI FakeGetFileAttributesA(LPCSTR lpFileName) {
    MiceFSRedirectPathA(lpFileName, &lpFileName);
    return TrueGetFileAttributesA(lpFileName);
}
DWORD WINAPI FakeGetFileAttributesW(LPCWSTR lpFileName) {
    MiceFSRedirectPathW(lpFileName, &lpFileName);
    return TrueGetFileAttributesW(lpFileName);
}
BOOL WINAPI FakeCreateDirectoryA(LPCSTR lpFileName, LPSECURITY_ATTRIBUTES lpSecurityAttributes) {
    MiceFSRedirectPathA(lpFileName, &lpFileName);
    return TrueCreateDirectoryA(lpFileName, lpSecurityAttributes);
}
BOOL WINAPI FakeCreateDirectoryW(LPCWSTR lpFileName, LPSECURITY_ATTRIBUTES lpSecurityAttributes) {
    MiceFSRedirectPathW(lpFileName, &lpFileName);
    return TrueCreateDirectoryW(lpFileName, lpSecurityAttributes);
}
HANDLE WINAPI FakeFindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData) {
    MiceFSRedirectPathA(lpFileName, &lpFileName);
    return TrueFindFirstFileA(lpFileName, lpFindFileData);
}

DWORD WINAPI FakeGetCurrentDirectoryA(DWORD nBufferLength, LPSTR lpBuffer) {
    LPCSTR cwd = MiceFSGetCwd();
    DWORD length = strlen(cwd);
    if (nBufferLength < length + 1) {
        return length + 1;
    }
    strcpy_s(lpBuffer, nBufferLength, cwd);
    return length;
}
DWORD WINAPI FakeGetCurrentDirectoryW(DWORD nBufferLength, LPWSTR lpBuffer) {
    LPCSTR cwd = MiceFSGetCwd();
    DWORD length = strlen(cwd);
    if (nBufferLength < (length + 1) * 2) {
        return (length + 1) * 2;
    }
    MultiByteToWideChar(CP_ACP, 0, cwd, -1, lpBuffer, nBufferLength);
    return length;
}

DWORD WINAPI FakeGetFileType(HANDLE hFile) {
    if ((DWORD_PTR)hFile & MICE_HANDLE_MASK) {
        // TODO: This is a lie
        return FILE_TYPE_CHAR;
    }
    return TrueGetFileType(hFile);
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
    hook("Kernel32.dll", "GetFileType", FakeGetFileType, (void**)&TrueGetFileType);

    // PathIsDirectory, PathFileExists, etc just call GetFileAttributes under the hood
    hook("Kernel32.dll", "DeleteFileA", FakeDeleteFileA, (void**)&TrueDeleteFileA);
    hook("Kernel32.dll", "DeleteFileW", FakeDeleteFileW, (void**)&TrueDeleteFileW);
    hook("Kernel32.dll", "GetCurrentDirectoryA", FakeGetCurrentDirectoryA,
         (void**)&TrueGetCurrentDirectoryA);
    hook("Kernel32.dll", "GetCurrentDirectoryW", FakeGetCurrentDirectoryW,
         (void**)&TrueGetCurrentDirectoryW);

    hook("Kernel32.dll", "FindFirstFileA", FakeFindFirstFileA, (void**)&TrueFindFirstFileA);

    hook("Kernel32.dll", "GetFileAttributesA", FakeGetFileAttributesA,
         (void**)&TrueGetFileAttributesA);
    hook("Kernel32.dll", "GetFileAttributesW", FakeGetFileAttributesW,
         (void**)&TrueGetFileAttributesW);
    hook("Kernel32.dll", "CreateDirectoryA", FakeCreateDirectoryA, (void**)&TrueCreateDirectoryA);
    hook("Kernel32.dll", "CreateDirectoryW", FakeCreateDirectoryW, (void**)&TrueCreateDirectoryW);

    hook("MSVCR90.DLL", "_stat64i32", Fake_stat64i32, (void**)&True_stat64i32);
}
