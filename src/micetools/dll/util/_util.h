#pragma once

#include "log.h"
#include "hook.h"

#define HDATA_FILE 0
#define HDATA_FIND_VOLUME 1
#define HDATA_ANY 0xFFFFFFFF

BOOL FileExists(wchar_t* szPath);
PVOID GetDataForHandle(HANDLE hObject, DWORD type);
void SetDataForHandle(HANDLE hObject, DWORD type, PVOID pData, BOOL isHeap);
BOOL RemoveDataForHandle(HANDLE hObject, DWORD type);
HANDLE GetDummyHandle();
void BytesToHex(char* hex_buffer, BYTE* bytes, DWORD nbytes);
void PrintStack(void);

BOOL PathEqual(LPCSTR path1, LPCSTR path2);
BOOL PathPrefix(LPCSTR path, LPCSTR prefix);

void make_dirs(const char* path);
void* open_mapped_file(LPCWSTR path, DWORD size, HANDLE* file, HANDLE* file_mapping);
