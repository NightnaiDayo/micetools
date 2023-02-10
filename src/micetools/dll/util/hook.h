#pragma once

#include <Windows.h>

typedef struct function_hook {
    LPCSTR dll;
    LPCSTR name;
    PVOID patch;
    PVOID* store;
    struct function_hook* next;
} function_hook_t;

static BOOL Detour(PVOID src, PVOID dst, const intptr_t len);

void patch_at(PVOID addr, const char* patch, DWORD length);
void clear_at(PVOID addr, BYTE clearVal, DWORD length);
void* CreateHook32(PVOID src, PVOID dst);

static void append_hook(function_hook_t* hook);
void hook(LPCSTR dll, LPCSTR name, void* patch, void** store);

void setup_hooks();
