#include <Windows.h>

#include "../hooks/files.h"

BOOL FileExists(wchar_t* szPath) {
    DWORD dwAttrib = GetFileAttributesW(szPath);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

typedef struct _handle_list {
    HANDLE m_Handle;
    PVOID m_pData;
    DWORD m_Type;
    BOOL m_IsOnHeap;
    struct _handle_list* m_Next;
} handle_list_t;
handle_list_t handle_list_root = {
    .m_Handle = INVALID_HANDLE_VALUE,
    .m_pData = NULL,
    .m_Type = 0xffffffff,
    .m_IsOnHeap = FALSE,
    .m_Next = NULL,
};

PVOID GetDataForHandle(HANDLE hObject, DWORD type) {
    if (hObject == INVALID_HANDLE_VALUE) return NULL;

    handle_list_t* head = &handle_list_root;
    while (head) {
        if (head->m_Handle == hObject && (type == HDATA_ANY || head->m_Type == type))
            return head->m_pData;
        head = head->m_Next;
    }
    return NULL;
}
void SetDataForHandle(HANDLE hObject, DWORD type, PVOID pData, BOOL isHeap) {
    if (hObject == INVALID_HANDLE_VALUE) return;

    handle_list_t* head = &handle_list_root;
    while (1) {
        if (head->m_Handle == hObject && (type == HDATA_ANY || head->m_Type == type)) {
            if (head->m_IsOnHeap) free(head->m_pData);
            head->m_pData = pData;
            head->m_IsOnHeap = isHeap;
            return;
        }
        if (head->m_Next == NULL) break;
        head = head->m_Next;
    }
    head->m_Next = malloc(sizeof *head);
    head->m_Next->m_Handle = hObject;
    head->m_Next->m_pData = pData;
    head->m_Next->m_Next = NULL;
    head->m_Next->m_IsOnHeap = isHeap;
    head->m_Next->m_Type = type;
}
BOOL RemoveDataForHandle(HANDLE hObject, DWORD type) {
    if (hObject == INVALID_HANDLE_VALUE) return FALSE;

    handle_list_t* head = &handle_list_root;
    handle_list_t* previous = &handle_list_root;
    BOOL ret = FALSE;
    while (head) {
        if (head->m_Handle == hObject && (type == HDATA_ANY || head->m_Type == type)) {
            previous->m_Next = head->m_Next;
            if (head->m_IsOnHeap) free(head->m_pData);
            handle_list_t* next = head->m_Next;
            free(head);
            head = next;
            if (type != HDATA_ANY) return TRUE;
            ret = TRUE;
        } else {
            previous = head;
            head = head->m_Next;
        }
    }
    return ret;
}

HANDLE GetDummyHandle() {
    CHAR path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    HANDLE hObject =
        _CreateFileA(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if (hObject == INVALID_HANDLE_VALUE) {
        log_error(HOOKS_LOGGER, "Failed to create dummy handle: %03x", GetLastError());
    }

    return hObject;
}

void BytesToHex(char* hex_buffer, BYTE* bytes, DWORD nbytes) {
    for (size_t i = 0; i < nbytes; i++) {
        sprintf(hex_buffer + i * 3, "%02x ", bytes[i]);
    }
    hex_buffer[nbytes * 3] = '\0';
}
