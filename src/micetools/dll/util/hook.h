#pragma once

#include <Windows.h>

typedef struct function_hook {
    LPCSTR dll;
    LPCSTR name;
    PVOID patch;
    PVOID* store;
    struct function_hook* next;
} function_hook_t;

#define NOP 0x90

DWORD MiceGetImageBase(void);

void MiceHookPatchAt(PVOID addr, const void* patch, DWORD length);
#define MiceHookPatchAtImage(hModule, addr, patch, length)                                   \
    MiceHookPatchAt((PVOID)((DWORD)(addr) + ((DWORD)(hModule)-MiceGetImageBase())), (patch), \
                    (length))
void MiceHookClearAt(PVOID addr, BYTE clearVal, DWORD length);
#define MiceHookClearAtImage(hModule, addr, clearVal, length)                                   \
    MiceHookClearAt((PVOID)((DWORD)(addr) + ((DWORD)(hModule)-MiceGetImageBase())), (clearVal), \
                    (length))
#define MiceHookNopAt(addr, length) MiceHookClearAt((addr), NOP, (length))
#define MiceHookNopAtImage(hModule, addr, length) \
    MiceHookClearAtImage((hModule), (addr), NOP, (length))

static void append_hook(function_hook_t* hook);
void hook(LPCSTR dll, LPCSTR name, void* patch, void** store);

void setup_hooks();
