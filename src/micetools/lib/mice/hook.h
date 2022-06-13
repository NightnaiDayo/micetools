#pragma once

#include <Windows.h>

#include "./mice.h"

typedef struct function_hook {
    LPCSTR dll;
    LPCSTR name;
    PVOID patch;
    PVOID* store;
    UINT length;
    struct function_hook* next;
} function_hook_t;

void patch_at(PVOID addr, const char* patch, DWORD length);
void clear_at(PVOID addr, BYTE clearVal, DWORD length);
void* CreateHook(PVOID src, PVOID dst, const intptr_t len);

void hook(LPCSTR dll, LPCSTR name, void* patch, void** store, UINT length);

void setup_hooks();
