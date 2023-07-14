// Windows XP does not have RtlCaptureStackBackTrace. This is an awful workaround.
// TODO: Reimplement this ourselves so we can print stacks on XP
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#include <Windows.h>
#include <dbghelp.h>

#include "../hooks/files.h"

void PrintStack(void) {
    unsigned int i;
    void* stack[100];
    unsigned short frames;
    SYMBOL_INFO* symbol;
    HANDLE process;

    process = GetCurrentProcess();

    SymInitialize(process, NULL, TRUE);

    frames = CaptureStackBackTrace(0, 100, stack, NULL);
    symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    for (i = 0; i < frames; i++) {
        SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
        printf("%i: %s - 0x%0X\n", frames - i - 1, symbol->Name, (unsigned int)symbol->Address);
    }

    free(symbol);
}

BOOL FileExistsW(const wchar_t* szPath) {
    DWORD dwAttrib = _GetFileAttributesW(szPath);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}
BOOL FileExistsA(const char* szPath) {
    wchar_t szPathW[MAX_PATH + 1];
    MultiByteToWideChar(0, 0, szPath, -1, szPathW, _countof(szPathW));
    DWORD dwAttrib = _GetFileAttributesW(szPathW);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

typedef struct HANDLE_DATA {
    HANDLE m_Handle;
    PVOID m_pData;
    DWORD m_Type;
    BOOL m_IsOnHeap;
} HANDLE_DATA;
MICE_DA_NEW(g_handleData, HANDLE_DATA)

PVOID GetDataForHandle(HANDLE hObject, DWORD type) {
    if (hObject == INVALID_HANDLE_VALUE) return NULL;

    MICE_DA_ITER(g_handleData, HANDLE_DATA, i) {
        if (i->m_Handle == hObject && (type == HDATA_ANY || i->m_Type == type)) {
            return i->m_pData;
        }
    }
    MICE_DA_ITER_END
    return NULL;
}
void SetDataForHandle(HANDLE hObject, DWORD type, PVOID pData, BOOL isHeap) {
    if (hObject == INVALID_HANDLE_VALUE) return;

    MICE_DA_ITER(g_handleData, HANDLE_DATA, i) {
        if (i->m_Handle == hObject && (type == HDATA_ANY || i->m_Type == type)) {
            if (i->m_IsOnHeap) free(i->m_pData);
            i->m_pData = pData;
            i->m_IsOnHeap = isHeap;
            return;
        }
    }
    MICE_DA_ITER_END

    HANDLE_DATA newHandleData = {
        .m_Handle = hObject,
        .m_pData = pData,
        .m_IsOnHeap = isHeap,
        .m_Type = type,
    };
    MiceDAPush(g_handleData, &newHandleData);
}
BOOL RemoveDataForHandle(HANDLE hObject, DWORD type) {
    if (hObject == INVALID_HANDLE_VALUE) return FALSE;

    MICE_DA_ITER(g_handleData, HANDLE_DATA, i) {
        if (i->m_Handle == hObject && (type == HDATA_ANY || i->m_Type == type)) {
            if (i->m_IsOnHeap) free(i->m_pData);
            MICE_DA_REMOVE_CURRENT(g_handleData);
            return TRUE;
        }
    }
    MICE_DA_ITER_END
    return FALSE;
}

HANDLE GetDummyHandle() {
    CHAR path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    HANDLE hObject =
        _CreateFileA(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if (hObject == INVALID_HANDLE_VALUE) {
        log_error(plfMisc, "Failed to create dummy handle: %03x", GetLastError());
    }

    return hObject;
}

void BytesToHex(char* hex_buffer, BYTE* bytes, DWORD nbytes) {
    for (size_t i = 0; i < nbytes; i++) {
        sprintf_s(hex_buffer + i * 3, RSIZE_MAX, "%02x ", bytes[i]);
    }
    hex_buffer[nbytes * 3] = '\0';
}

void make_dirs(const char* path) {
    int count = 0;
    size_t i;
    size_t len = strlen(path);
    char* temp = malloc(len + 1);
    memcpy(temp, path, len + 1);

    for (i = 0; i < len; i++) {
        if (temp[i] == '/' || temp[i] == '\\') count++;
    }
    // Don't create directories for trailing slashes
    if (count && i && (temp[i - 1] == '/' || temp[i - 1] == '\\')) {
        count--;
    }
    if (count == 0) {
        free(temp);
        return;
    }

    for (i = 0; i < len && count; i++) {
        if (temp[i] == '/' || temp[i] == '\\') {
            temp[i] = '\0';
            CreateDirectory(temp, NULL);
            temp[i] = '\\';
            count--;
        }
    }
    free(temp);
}

void* open_mapped_file(LPCSTR lpFilename, DWORD size, HANDLE* file, HANDLE* file_mapping) {
    // char szFileName[MAX_PATH] = { 0 };
    // WideCharToMultiByte(CP_ACP, 0, lpFilename, wcslen(lpFilename), szFileName,
    //                     size2int(sizeof szFileName), NULL, NULL);
    make_dirs(lpFilename);

    *file = _CreateFileA(lpFilename, GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, NULL);
    if (*file == INVALID_HANDLE_VALUE) {
        log_error(plfMisc, "Failed to CreateFileW(%s): %d", lpFilename, GetLastError());
        return NULL;
    }

    *file_mapping = CreateFileMappingW(*file, NULL, PAGE_READWRITE, 0, size, NULL);

    if (*file_mapping == INVALID_HANDLE_VALUE) {
        CloseHandle(*file);
        *file = INVALID_HANDLE_VALUE;

        log_error(plfMisc, "Failed to CreateFileMappingW: %d", GetLastError());
        return NULL;
    }

    SetLastError(0);
    void* mapping = MapViewOfFile(*file_mapping, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (mapping == NULL || GetLastError()) {
        log_error(plfMisc, "Failed to MapViewOfFileEx: %d", GetLastError());
        CloseHandle(*file);
        CloseHandle(*file_mapping);
        *file = INVALID_HANDLE_VALUE;
        *file_mapping = INVALID_HANDLE_VALUE;
        return NULL;
    }
    return mapping;
}
