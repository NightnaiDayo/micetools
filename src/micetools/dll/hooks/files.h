#pragma once
#include "../common.h"
#include "com.h"

static HANDLE(WINAPI* TrueCreateFileA)(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                                       LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                       DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                                       HANDLE hTemplateFile);
static HANDLE(WINAPI* TrueCreateFileW)(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                                       LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                       DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                                       HANDLE hTemplateFile);

static BOOL(WINAPI* TrueDeviceIoControl)(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer,
                                         DWORD nInBufferSize, LPVOID lpOutBuffer,
                                         DWORD nOutBufferSize, LPDWORD lpBytesReturned,
                                         LPOVERLAPPED lpOverlapped);

static DWORD(WINAPI* TrueSetFilePointer)(HANDLE hFile, LONG lDistanceToMove,
                                         PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod);
static BOOL(WINAPI* TrueSetFilePointerEx)(HANDLE hFile, LARGE_INTEGER liDistanceToMove,
                                          PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod);
// logging needs access to WriteFile, so we can't static it!
BOOL(WINAPI* TrueWriteFile)
(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten,
 LPOVERLAPPED lpOverlapped);
static BOOL(WINAPI* TrueReadFile)(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                                  LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
static BOOL(WINAPI* TrueGetFileSizeEx)(HANDLE hFile, PLARGE_INTEGER lpFileSize);
static BOOL(WINAPI* TrueCloseHandle)(HANDLE hObject);
static BOOL(WINAPI* TruePathFileExistsA)(LPCSTR pszPath);
static BOOL(WINAPI* TruePathFileExistsW)(LPCWSTR pszPath);
static BOOL(WINAPI* TrueDeleteFileA)(LPCSTR lpFileName);
static BOOL(WINAPI* TrueDeleteFileW)(LPCWSTR lpFileName);

typedef BOOL(FnDeviceIoControl)(void* file, DWORD dwIoControlCode, LPVOID lpInBuffer,
                                DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                                LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped);

typedef DWORD(FnSetFilePointer)(void* file, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh,
                                DWORD dwMoveMethod);
typedef BOOL(FnSetFilePointerEx)(void* file, LARGE_INTEGER liDistanceToMove,
                                 PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod);
typedef BOOL(FnWriteFile)(void* file, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                          LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
typedef BOOL(FnReadFile)(void* file, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                         LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);

typedef BOOL(FnGetFileSizeEx)(void* file, PLARGE_INTEGER lpFileSize);

typedef struct _stat64i32 _stat64i32_t;
static int(WINAPIV* True_stat64i32)(const char* path, _stat64i32_t* buffer);

#define _WriteFile (TrueWriteFile ? TrueWriteFile : WriteFile)
#define _ReadFile (TrueReadFile ? TrueReadFile : ReadFile)
#define _CloseHandle (TrueCloseHandle ? TrueCloseHandle : CloseHandle)
#define _CreateFileW (TrueCreateFileW ? TrueCreateFileW : CreateFileW)
#define _CreateFileA (TrueCreateFileA ? TrueCreateFileA : CreateFileA)

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
    FnSetFilePointer* SetFilePointer;
    FnSetFilePointerEx* SetFilePointerEx;
    FnWriteFile* WriteFile;
    FnReadFile* ReadFile;
    FnGetFileSizeEx* GetFileSizeEx;

    void* data;

    LPHANDLE virtual_handle;
    struct file_hook* next;
} file_hook_t;

typedef struct open_hook {
    HANDLE handle;
    file_hook_t* file_hook;
    com_hook_t* com_hook;
    struct open_hook* next;
} open_hook_t;

extern file_hook_t* file_hook_list;

file_hook_t* new_file_hook(LPCWSTR filename);
void hook_file(file_hook_t* hook);
void hook_io();

void close_hook(HANDLE handle);
file_hook_t* get_handle_file_hook(HANDLE handle);
com_hook_t* get_handle_com_hook(HANDLE handle);
