#pragma once

#include "hook.h"

BOOL FileOrFolderExistsW(const wchar_t* szPath);
BOOL FileExistsW(const wchar_t* szPath);
BOOL FileExistsA(const char* szPath);
PVOID GetDataForHandle(HANDLE hObject);
void SetDataForHandle(HANDLE hObject, PVOID pData, BOOL isHeap);
BOOL RemoveDataForHandle(HANDLE hObject);
HANDLE GetDummyHandle();
void BytesToHex(char* hex_buffer, BYTE* bytes, DWORD nbytes);

/**
 * WARNING: This function will not work on XP machines!!
 */
void PrintStack(void);

BOOL PathEqual(LPCSTR path1, LPCSTR path2);
BOOL PathPrefix(LPCSTR path, LPCSTR prefix);
char GetGamedataDrive(void);
BOOL IsGamedataLocalPath(LPCSTR path);

void make_dirs(const char* path);
void* open_mapped_file(LPCSTR path, DWORD size, HANDLE* file, HANDLE* file_mapping);

#define char_lower(value) (('A' <= (value) && (value) <= 'Z') ? ((value) - 'A' + 'a') : (value))
#define char_upper(value) (('a' <= (value) && (value) <= 'z') ? ((value) - 'a' + 'A') : (value))

// Use the second tag bit to indicate a mice handle
#define MICE_HANDLE_MASK 0x00000002
