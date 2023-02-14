#include "hook.h"

#include <memory.h>
#include <stdbool.h>
#include <stdlib.h>

#include "log.h"

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
void hook(LPCSTR dll, LPCSTR name, void* patch, void** store) {
    function_hook_t* hook = (function_hook_t*)malloc(sizeof(struct function_hook));
    hook->dll = dll;
    hook->name = name;
    hook->patch = patch;
    hook->store = store;
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

void* Win32HotpatchHook(PVOID src, PVOID dst) {
    LPBYTE bSrc = (LPBYTE)src;

    DWORD oldProt;
    VirtualProtect(bSrc - 5, 7, PAGE_EXECUTE_READWRITE, &oldProt);

    // relative JMP to dst
    bSrc[-5] = 0xE9;
    DWORD relJump = (DWORD)dst - (DWORD)src;
    bSrc[-1] = (relJump >> 24) & 0xff;
    bSrc[-2] = (relJump >> 16) & 0xff;
    bSrc[-3] = (relJump >> 8) & 0xff;
    bSrc[-4] = relJump & 0xff;
    // JMP $-5
    bSrc[0] = 0xEB;
    bSrc[1] = 0xF9;

    VirtualProtect(bSrc - 5, 7, oldProt, &oldProt);

    // We don't need a gateway; we can just jump right in
    return bSrc + 2;
}
void* CreateHook32(PVOID src, PVOID dst) {
    LPBYTE bSrc = (LPBYTE)src;

    // If this DLL is hotpatchable, sieze the opportunity
    if (bSrc[0] == 0x8b && bSrc[1] == 0xff && bSrc[-1] == 0xCC && bSrc[-2] == 0xCC &&
        bSrc[-3] == 0xCC && bSrc[-4] == 0xCC && bSrc[-5] == 0xCC) {
        return Win32HotpatchHook(src, dst);
    }

    intptr_t len = 5;

    // This is a very crude way to identify common instruction patterns
    // to select or patch length.
    if (bSrc[0] == 0xff && bSrc[1] == 0x25) {
        // jmp  DWORD PTR ds:0x........
        len = 6;
    } else if (bSrc[0] == 0x6a && bSrc[2] == 0x68) {
        // push 0x...       (byte)
        // push 0x...       (dword)
        len = 7;
    } else if (bSrc[0] == 0x6a && bSrc[2] == 0xb8) {
        // push 0x...       (byte)
        // mov  eax,0x...   (dword)
        len = 7;
    } else if (bSrc[0] == 0x68) {
        // push 0x...       (dword)
        len = 5;
    } else if (bSrc[0] == 0x55 && bSrc[1] == 0x8B && bSrc[2] == 0xEC && bSrc[3] == 0x83 &&
                         bSrc[4] == 0xE4) {
        // pusb ebp
        // mov ebp,esp
        // and esp,ffffff**
        len = 6;
    } else if (bSrc[0] == 0x55 && bSrc[1] == 0x8B && bSrc[2] == 0xEC && bSrc[3] == 0x8b &&
                         bSrc[4] == 0x45) {
        // pusb ebp
        // mov ebp,esp
        // mov eax,DWORD PTR [...]
        len = 6;
    } else if (bSrc[0] == 0x55 && bSrc[1] == 0x8B && bSrc[2] == 0xEC && bSrc[3] == 0x80 &&
                         bSrc[4] == 0x3D) {
        // pusb ebp
        // mov ebp,esp
        // cmd BYTE PTR ds:0x...,0x...
        len = 10;
    } else {
        log_error(HOOKS_LOGGER, "Unable to identify gateway length! Function peek:");
        for (int i = 0; i < 16; i++) {
            printf("%02x ", ((LPBYTE)src)[i]);
        }
        puts("");
        log_error(HOOKS_LOGGER, "Unsafely defaulting to 5!");
        len = 5;
    }

    PVOID gateway = VirtualAlloc(0, len + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!gateway) return NULL;

    memcpy(gateway, src, len);

    *(PCHAR)((int)gateway + len) = '\xE9';
    *(PINT)((int)gateway + len + 1) = (int)src - (int)gateway - 5;

    Detour(src, dst, len);

    return gateway;
}

extern FARPROC (*TrueGetProcAddress)(HMODULE hModule, LPCSTR lpProcName);

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
            log_error(HOOKS_LOGGER, "failed to load dll %s (%03x). %s skipped", hook->dll,
                      GetLastError(), hook->name);
            hook = hook->next;
            continue;
        }

        // void* original =
        //     (TrueGetProcAddress ? TrueGetProcAddress : GetProcAddress)(dll, hook->name);
        void* original = GetProcAddress(dll, hook->name);
        if (original == NULL) {
            log_warning(HOOKS_LOGGER, "failed to get original %s (%03x)", hook->name,
                        GetLastError());
        } else {
            void* gateway = CreateHook32(original, hook->patch);
            if (hook->store != NULL) *hook->store = gateway;
            log_misc(HOOKS_LOGGER, "hooked %s", hook->name);
        }

        hook = hook->next;
    } while (hook != NULL);

    log_info(HOOKS_LOGGER, "attach complete");
}
