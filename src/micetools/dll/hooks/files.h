#pragma once
#include "com.h"

#ifndef _MICE_FILES
#define _MICE_FILES extern
#endif

typedef struct {
    unsigned int st_dev;
    unsigned short st_ino;
    unsigned short st_mode;
    short st_nlink;
    short st_uid;
    short st_gid;
    unsigned int st_rdev;
    long st_size;
    __time64_t st_atime;
    __time64_t st_mtime;
    __time64_t st_ctime;
} _stat64i32_t;

_MICE_FILES HANDLE(WINAPI* TrueCreateFileA)(LPCSTR lpFileName, DWORD dwDesiredAccess,
                                            DWORD dwShareMode,
                                            LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                            DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                                            HANDLE hTemplateFile);
_MICE_FILES HANDLE(WINAPI* TrueCreateFileW)(LPCWSTR lpFileName, DWORD dwDesiredAccess,
                                            DWORD dwShareMode,
                                            LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                            DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                                            HANDLE hTemplateFile);

_MICE_FILES BOOL(WINAPI* TrueDeviceIoControl)(HANDLE hDevice, DWORD dwIoControlCode,
                                              LPVOID lpInBuffer, DWORD nInBufferSize,
                                              LPVOID lpOutBuffer, DWORD nOutBufferSize,
                                              LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped);

_MICE_FILES DWORD(WINAPI* TrueSetFilePointer)(HANDLE hFile, LONG lDistanceToMove,
                                              PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod);
_MICE_FILES BOOL(WINAPI* TrueSetFilePointerEx)(HANDLE hFile, LARGE_INTEGER liDistanceToMove,
                                               PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod);
// logging needs access to WriteFile, so we can't static it!
BOOL(WINAPI* TrueWriteFile)
(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten,
 LPOVERLAPPED lpOverlapped);
_MICE_FILES BOOL(WINAPI* TrueReadFile)(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                                       LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
_MICE_FILES BOOL(WINAPI* TrueGetFileSizeEx)(HANDLE hFile, PLARGE_INTEGER lpFileSize);
_MICE_FILES BOOL(WINAPI* TrueCloseHandle)(HANDLE hObject);
_MICE_FILES BOOL(WINAPI* TrueDeleteFileA)(LPCSTR lpFileName);
_MICE_FILES BOOL(WINAPI* TrueDeleteFileW)(LPCWSTR lpFileName);
_MICE_FILES HANDLE(WINAPI* TrueFindFirstFileA)(LPCSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData);
_MICE_FILES DWORD(WINAPI* TrueGetFileAttributesA)(LPCSTR lpFileName);
_MICE_FILES DWORD(WINAPI* TrueGetFileAttributesW)(LPCWSTR lpFileName);
_MICE_FILES BOOL(WINAPI* TrueCreateDirectoryA)(LPCSTR lpFileName,
                                               LPSECURITY_ATTRIBUTES lpSecurityAttributes);
_MICE_FILES BOOL(WINAPI* TrueCreateDirectoryW)(LPCWSTR lpFileName,
                                               LPSECURITY_ATTRIBUTES lpSecurityAttributes);

_MICE_FILES DWORD(WINAPI* TrueGetCurrentDirectoryA)(DWORD nBufferLength, LPSTR lpBuffer);
_MICE_FILES DWORD(WINAPI* TrueGetCurrentDirectoryW)(DWORD nBufferLength, LPWSTR lpBuffer);

typedef struct {
    HANDLE m_Handle;
    LARGE_INTEGER m_Pointer;
    void* m_HookData;
} file_context_t;

typedef BOOL(WINAPI FnDeviceIoControl)(file_context_t* ctx, DWORD dwIoControlCode,
                                       LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer,
                                       DWORD nOutBufferSize, LPDWORD lpBytesReturned,
                                       LPOVERLAPPED lpOverlapped);

typedef DWORD(FnSetFilePointer)(file_context_t* ctx, LONG lDistanceToMove,
                                PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod);
typedef BOOL(FnSetFilePointerEx)(file_context_t* ctx, LARGE_INTEGER liDistanceToMove,
                                 PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod);
typedef BOOL(FnWriteFile)(file_context_t* ctx, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                          LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
typedef BOOL(FnReadFile)(file_context_t* ctx, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                         LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);

typedef BOOL(FnGetFileSizeEx)(file_context_t* ctx, PLARGE_INTEGER lpFileSize);

static int(WINAPIV* True_stat64i32)(const char* path, _stat64i32_t* buffer);

#define _WriteFile (TrueWriteFile ? TrueWriteFile : WriteFile)
#define _ReadFile (TrueReadFile ? TrueReadFile : ReadFile)
#define _CloseHandle (TrueCloseHandle ? TrueCloseHandle : CloseHandle)
#define _CreateFileW (TrueCreateFileW ? TrueCreateFileW : CreateFileW)
#define _CreateFileA (TrueCreateFileA ? TrueCreateFileA : CreateFileA)
#define _SetFilePointer (TrueSetFilePointer ? TrueSetFilePointer : SetFilePointer)
#define _SetFilePointerEx (TrueSetFilePointerEx ? TrueSetFilePointerEx : SetFilePointerEx)
#define _GetFileAttributesW (TrueGetFileAttributesW ? TrueGetFileAttributesW : GetFileAttributesW)
// GetFileAttributesA internally calls (hooked)GetFileAttributesW
#define _GetCurrentDirectoryA \
    (TrueGetCurrentDirectoryA ? TrueGetCurrentDirectoryA : GetCurrentDirectoryA)
#define _GetCurrentDirectoryW \
    (TrueGetCurrentDirectoryW ? TrueGetCurrentDirectoryA : GetCurrentDirectoryA)

typedef struct drive_redirect {
    const CHAR* drive;
    const CHAR* path;
} drive_redirect_t;

typedef struct drive_redirect_w {
    const WCHAR* drive;
    const WCHAR* path;
} drive_redirect_w_t;

typedef struct file_hook {
    LPCWSTR filename;
    LPCWSTR altFilename;

    FnDeviceIoControl* DeviceIoControl;
    FnWriteFile* WriteFile;
    FnReadFile* ReadFile;
    FnGetFileSizeEx* GetFileSizeEx;

    com_hook_t* com_hook;

    void* hook_data;
    LPHANDLE virtual_handle;
    struct file_hook* next;
} file_hook_t;

typedef struct open_hook {
    file_context_t ctx;
    file_hook_t* hook;
    struct open_hook* next;
} open_hook_t;

extern file_hook_t* file_hook_list;

file_hook_t* new_file_hook(LPCWSTR filename);
void hook_file(file_hook_t* hook);
void hook_io();
void hook_file_with_buffer(LPCWSTR filename, LPBYTE buffer, DWORD nBytes, DWORD access);
file_hook_t* unhook_file(LPCWSTR lpFilename);
