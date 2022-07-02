#include "hook.h"
#include "log.h"

#include <memory.h>
#include <stdlib.h>

function_hook_t* hook_list = NULL;

void append_hook(function_hook_t* hook) {
    hook->next = NULL;
    if (hook_list == NULL) {
        hook_list = hook;
        return;
    }

    function_hook_t* hl = hook_list;
    while (hl->next != NULL) hl = hl->next;
    hl->next = hook;
}
void hook(LPCSTR dll, LPCSTR name, void* patch, void** store, UINT length) {
    function_hook_t* hook = (function_hook_t*)malloc(sizeof(struct function_hook));
    hook->dll = dll;
    hook->name = name;
    hook->patch = patch;
    hook->store = store;
    hook->length = length;
    append_hook(hook);
}

void patch_at(PVOID addr, const char* patch, DWORD length) {
    DWORD oldProt;
    VirtualProtect(addr, length, PAGE_EXECUTE_READWRITE, &oldProt);
    memcpy(addr, patch, length);
    VirtualProtect(addr, length, oldProt, &oldProt);
}
void clear_at(PVOID addr, BYTE clearVal, DWORD length) {
    DWORD oldProt;
    VirtualProtect(addr, length, PAGE_EXECUTE_READWRITE, &oldProt);
    memset(addr, clearVal, length);
    VirtualProtect(addr, length, oldProt, &oldProt);
}

BOOL Detour(PVOID src, PVOID dst, const intptr_t len) {
    if (len < 5) return FALSE;

    DWORD oldProt;
    VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &oldProt);

    *(PCHAR)src = '\xE9';
    *(PINT)((int)src + 1) = (int)dst - (int)src - 5;

    VirtualProtect(src, len, oldProt, &oldProt);
    return TRUE;
}

void* CreateHook(PVOID src, PVOID dst, const intptr_t len) {
    if (len < 5) return NULL;

    PVOID gateway = VirtualAlloc(0, len + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!gateway) return NULL;
    memcpy(gateway, src, len);

    *(PCHAR)((int)gateway + len) = '\xE9';
    *(PINT)((int)gateway + len + 1) = (int)src - (int)gateway - 5;

    Detour(src, dst, len);

    return gateway;
}

void setup_hooks() {
    log_info(HOOKS_LOGGER, "attaching");

    if (hook_list == NULL) {
        log_warning(HOOKS_LOGGER, "No hooks to register!");
        return;
    }
    function_hook_t* hook = hook_list;
    do {
        if (hook->dll == NULL) continue;

        HMODULE dll = LoadLibraryA(hook->dll);
        if (dll == NULL) {
            log_error(HOOKS_LOGGER, "failed to load dll %s. %s skipped", hook->dll, hook->name);
            hook = hook->next;
            continue;
        }

        if ((*(hook->store) = GetProcAddress(dll, hook->name)) == NULL) {
            log_warning(HOOKS_LOGGER, "failed to get original %s", hook->name);
        } else {
            *((void**)hook->store) = CreateHook(*hook->store, hook->patch, hook->length);
            log_misc(HOOKS_LOGGER, "hooked %s", hook->name);
        }

        hook = hook->next;
    } while (hook != NULL);

    log_info(HOOKS_LOGGER, "attach complete");
}
