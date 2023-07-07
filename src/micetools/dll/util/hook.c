#include "hook.h"

#include <detours.h>
#include <memory.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../../lib/mice/mice.h"

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

DWORD MiceGetImageBase(void) {
    static DWORD imageBase = 0;
    if (imageBase != 0) return imageBase;

    WCHAR sModulePath[MAX_PATH];
    GetModuleFileNameW(NULL, sModulePath, MAX_PATH);

    HANDLE hObject =
        CreateFileW(sModulePath, FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hObject == INVALID_HANDLE_VALUE) {
        log_error(plfBoot, "Failed to open %ls %03x", sModulePath, GetLastError());
        return 0;
    }

    IMAGE_DOS_HEADER dosHeader = { 0 };
    DWORD nRead;
    SetFilePointer(hObject, 0, NULL, FILE_BEGIN);
    ReadFile(hObject, &dosHeader, sizeof dosHeader, &nRead, NULL);
    if (nRead != sizeof dosHeader) {
        log_error(plfBoot, "Failed to read DOS header %03x", GetLastError());
        return 0;
    }
    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
        log_error(plfBoot, "Invalid DOS magic number: %04x", dosHeader.e_magic);
        return 0;
    }

    IMAGE_NT_HEADERS32 ntHeaders32 = { 0 };
    SetFilePointer(hObject, dosHeader.e_lfanew, NULL, FILE_BEGIN);
    ReadFile(hObject, &ntHeaders32, sizeof ntHeaders32, &nRead, NULL);
    if (nRead != sizeof ntHeaders32) {
        log_error(plfBoot, "Failed to read NT header %03x", GetLastError());
        return 0;
    }
    if (ntHeaders32.Signature != IMAGE_NT_SIGNATURE) {
        log_error(plfBoot, "Invalid NT signature: %08x", ntHeaders32.Signature);
        return 0;
    }

    CloseHandle(hObject);
    return (imageBase = ntHeaders32.OptionalHeader.ImageBase);
}

void MiceHookPatchAt(PVOID addr, const void* patch, DWORD length) {
    DWORD oldProt;
    VirtualProtect(addr, length, PAGE_EXECUTE_READWRITE, &oldProt);
    memcpy(addr, patch, length);
    VirtualProtect(addr, length, oldProt, &oldProt);
}
void MiceHookClearAt(PVOID addr, BYTE clearVal, DWORD length) {
    DWORD oldProt;
    VirtualProtect(addr, length, PAGE_EXECUTE_READWRITE, &oldProt);
    memset(addr, clearVal, length);
    VirtualProtect(addr, length, oldProt, &oldProt);
}

// BOOL Detour(PVOID src, PVOID dst, const intptr_t len) {
//     if (len < 5) return FALSE;

//     DWORD oldProt;
//     VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &oldProt);

//     *(PCHAR)src = '\xE9';
//     *(PINT)((size_t)src + 1) = size2int((size_t)dst - (size_t)src - 5);

//     VirtualProtect(src, len, oldProt, &oldProt);
//     return TRUE;
// }

// void* Win32HotpatchHook(PVOID src, PVOID dst) {
//     LPBYTE bSrc = (LPBYTE)src;

//     DWORD oldProt;
//     VirtualProtect(bSrc - 5, 7, PAGE_EXECUTE_READWRITE, &oldProt);

//     // relative JMP to dst
//     bSrc[-5] = 0xE9;
//     size_t relJump = (size_t)dst - (size_t)src;
//     bSrc[-1] = (relJump >> 24) & 0xff;
//     bSrc[-2] = (relJump >> 16) & 0xff;
//     bSrc[-3] = (relJump >> 8) & 0xff;
//     bSrc[-4] = relJump & 0xff;
//     // JMP $-5
//     bSrc[0] = 0xEB;
//     bSrc[1] = 0xF9;

//     VirtualProtect(bSrc - 5, 7, oldProt, &oldProt);

//     // We don't need a gateway; we can just jump right in
//     return bSrc + 2;
// }

// #define CMP2(x, a, b) (x[0] == 0x##a && x[1] == 0x##b)
// #define CMP3(x, a, b, c) (CMP2(x, a, b) && x[2] == 0x##c)
// #define CMP4(x, a, b, c, d) (CMP3(x, a, b, c) && x[3] == 0x##d)
// #define CMP5(x, a, b, c, d, e) (CMP4(x, a, b, c, d) && x[4] == 0x##e)
// void* CreateHook32(PVOID src, PVOID dst) {
//     LPBYTE bSrc = (LPBYTE)src;

//     // If this DLL is hotpatchable, sieze the opportunity
//     if (bSrc[0] == 0x8b && bSrc[1] == 0xff && bSrc[-1] == 0xCC && bSrc[-2] == 0xCC &&
//         bSrc[-3] == 0xCC && bSrc[-4] == 0xCC && bSrc[-5] == 0xCC) {
//         return Win32HotpatchHook(src, dst);
//     }

//     intptr_t len = 5;

//     // This is a very crude way to identify common instruction patterns
//     // to select or patch length.
//     if (CMP2(bSrc, ff, 25)) {
//         // jmp  DWORD PTR ds:0x........
//         len = 6;
//     } else if (bSrc[0] == 0x6a && bSrc[2] == 0x68) {
//         // push 0x...       (byte)
//         // push 0x...       (dword)
//         len = 7;
//     } else if (bSrc[0] == 0x6a && bSrc[2] == 0xb8) {
//         // push 0x...       (byte)
//         // mov  eax,0x...   (dword)
//         len = 7;
//     } else if (bSrc[0] == 0x68) {
//         // push 0x...       (dword)
//         len = 5;
//     } else if (CMP5(bSrc, 55, 8B, EC, 83, E4)) {
//         // pusb ebp
//         // mov ebp,esp
//         // and esp,ffffff**
//         len = 6;
//     } else if (CMP5(bSrc, 55, 8B, EC, 8B, 45)) {
//         // pusb ebp
//         // mov ebp,esp
//         // mov eax,DWORD PTR [...]
//         len = 6;
//     } else if (CMP5(bSrc, 55, 8B, EC, 80, 3D)) {
//         // pusb ebp
//         // mov ebp,esp
//         // cmd BYTE PTR ds:0x...,0x...
//         len = 10;
//     } else if (CMP5(bSrc, B8, 00, 10, 00, 00)) {
//         // mov eax,0x1000
//         len = 5;
//     } else {
//         char hex_str[16 * 3 + 1];
//         hex_str[0] = 0;
//         for (int i = 0; i < 16; i++) {
//             snprintf(&hex_str[i * 3], 4, "%02x ", ((LPBYTE)src)[i]);
//         }
//         hex_str[sizeof hex_str - 1] = 0;
//         log_error(plfHooks, "Unable to identify gateway length! Function peek: %s", hex_str);
//         log_error(plfHooks, "Unsafely defaulting to 5!");
//         len = 5;
//     }

//     PVOID gateway = VirtualAlloc(0, len + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
//     if (!gateway) return NULL;

//     memcpy(gateway, src, len);

//     *(PCHAR)((size_t)gateway + len) = '\xE9';
//     *(PINT)((size_t)gateway + len + 1) = size2int((size_t)src - (size_t)gateway - 5);

//     Detour(src, dst, len);

//     return gateway;
// }

// extern FARPROC (*TrueGetProcAddress)(HMODULE hModule, LPCSTR lpProcName);

void setup_hooks() {
    LONG detourError;
    if (hook_list == NULL) {
        log_warning(plfHooks, "No hooks to register!");
        return;
    }
    function_hook_t* hook = hook_list;
    do {
        if (hook->dll == NULL) continue;

        HMODULE dll = LoadLibraryA(hook->dll);
        if (dll == NULL) {
            log_error(plfHooks, "failed to load dll %s (%03x). %s skipped", hook->dll,
                      GetLastError(), hook->name);
            hook = hook->next;
            continue;
        }

        // void* original =
        //     (TrueGetProcAddress ? TrueGetProcAddress : GetProcAddress)(dll, hook->name);
        void* original = GetProcAddress(dll, hook->name);
        if (original == NULL) {
            log_warning(plfHooks, "failed to get original %s (%03x)", hook->name, GetLastError());
        } else {
            // void* gateway = CreateHook(original, hook->patch);
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());

            detourError = DetourAttach(&original, hook->patch);
            if (detourError != NO_ERROR) {
                log_error(plfHooks, "DetourAttach failed: %d", detourError);
                DetourTransactionAbort();
                continue;
            }

            detourError = DetourTransactionCommit();
            if (detourError != NO_ERROR) {
                log_error(plfHooks, "DetourTransactionCommit failed: %d", detourError);
                continue;
            }

            if (hook->store != NULL) *hook->store = original;
            log_misc(plfHooks, "hooked %s", hook->name);
        }

        hook = hook->next;
    } while (hook != NULL);
}
