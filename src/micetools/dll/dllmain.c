#include "common.h"
#include "devices/_devices.h"
#include "drivers/mx.h"
#include "hooks/_hooks.h"

WCHAR exePath[MAX_PATH + 1];
DWORD imageOffset;

#define WIN32_EXE_BASE 0x00400000

DWORD GetImageBase(LPCWSTR sModulePath) {
    HANDLE hObject =
        _CreateFileW(sModulePath, FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hObject == INVALID_HANDLE_VALUE) return 0;

    IMAGE_DOS_HEADER dosHeader = { 0 };
    DWORD nRead;
    _SetFilePointer(hObject, 0, NULL, FILE_BEGIN);
    _ReadFile(hObject, &dosHeader, sizeof dosHeader, &nRead, NULL);
    if (nRead != sizeof dosHeader) {
        log_error(BOOT_LOGGER, "Failed to read DOS header %03x", GetLastError());
        return 0;
    }
    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
        log_error(BOOT_LOGGER, "Invalid DOS magic number: %04x", dosHeader.e_magic);
        return 0;
    }

    IMAGE_NT_HEADERS32 ntHeaders32 = { 0 };
    _SetFilePointer(hObject, dosHeader.e_lfanew, NULL, FILE_BEGIN);
    _ReadFile(hObject, &ntHeaders32, sizeof ntHeaders32, &nRead, NULL);
    if (nRead != sizeof ntHeaders32) {
        log_error(BOOT_LOGGER, "Failed to read NT header %03x", GetLastError());
        return 0;
    }
    if (ntHeaders32.Signature != IMAGE_NT_SIGNATURE) {
        log_error(BOOT_LOGGER, "Invalid NT signature: %08x", ntHeaders32.Signature);
        return 0;
    }

    _CloseHandle(hObject);
    return ntHeaders32.OptionalHeader.ImageBase;
}

void apply_patches(HMODULE hModule) {
    void* baseAddress = (void*)hModule;

    DWORD imageBase = GetImageBase(exePath);
    if (imageBase == 0) {
        log_error(BOOT_LOGGER, "Failed to locate image base. Patches will not be applied.");
        return;
    }
    imageOffset = (DWORD)hModule - imageBase;

    char exePathC[MAX_PATH + 1];
    WideCharToMultiByte(CP_ACP, 0, exePath, -1, exePathC, sizeof exePathC, NULL, NULL);

    if (!load_patches(MiceConfig.mice.patches_file, exePathC)) {
        log_error(BOOT_LOGGER, "Failed to load patches file %s", MiceConfig.mice.patches_file);
        return;
    }

    patch_t* patch = patch_list->next;
    if (patch == NULL) {
        log_warning(BOOT_LOGGER, "No patches to apply. Did you forgot an amiDebug patch file?");
    }
    while (patch) {
        DWORD oldProt;
        VirtualProtect((void*)((DWORD)patch->address + imageOffset), patch->count,
                       PAGE_EXECUTE_READWRITE, &oldProt);

        if (memcmp(patch->match, (void*)((DWORD)patch->address + imageOffset), patch->count) != 0) {
            log_error(BOOT_LOGGER, "Patch %s failed! from-value missmatch", patch->name);
            VirtualProtect((void*)((DWORD)patch->address + imageOffset), patch->count, oldProt,
                           &oldProt);
            patch = patch->next;
            continue;
        }

        memcpy((void*)((DWORD)patch->address + imageOffset), patch->replace, patch->count);
        log_misc(BOOT_LOGGER, "Patched %d bytes at %08x (%s)", patch->count, patch->address,
                 patch->name);

        patch = patch->next;
    }
}

void prebind_hooks() {
    hook_all();
    install_devices();
    // TODO: Figure out why we're needing to call this manually (medium priority)
    if (wcscmp(exePath, L"ALLNetProc.exe") == 0) {
        log_warning(BOOT_LOGGER, "Making explicit call to OPENSSL_add_all_algorithms_noconf");

        // OPENSSL_add_all_algorithms_noconf
        ((void (*)(void))(0x00459770))();
    }
}

void init_injection(HMODULE hModule) {
    // Make sure our CRC32 tables are ready for anything that might want to use them
    amCrc32RCreateTable();

    load_mice_config();

    // We're in a new context now, so need to reconfigure
    setup_logging();
    log_info(BOOT_LOGGER, "Handover complete. Now executing within %ls", exePath);

    if (MiceConfig.mice.apply_patches) apply_patches(hModule);

    // Columba: Driver-level memory access, used to read the DMI tables
    if (MiceConfig.drivers.columba) setup_columba();
    // MX SRAM: SRAM-based nv memory
    if (MiceConfig.drivers.mxsram) setup_mxsram();
    // MX SuperIO: Communicate with the HW monitor chip
    if (MiceConfig.drivers.mxsuperio) setup_mxsuperio();
    // MX JVS: Interacting with JVS-based devices
    if (MiceConfig.drivers.mxjvs) setup_mxjvs();
    // MX HW Reset: Forcibly reboot the machine
    if (MiceConfig.drivers.mxhwreset) setup_mxhwreset();
    // MX SMBus: Communicate over the LPC bus. This contains the EEPROM, and PCA9535
    if (MiceConfig.drivers.mxsmbus) setup_mxsmbus();
    // MX Parallel: The parallel port (i.e. keychip)
    if (MiceConfig.drivers.mxparallel) setup_mxparallel();

    if (MiceConfig.drivers.platform) {
        if (!add_fake_device(&PLATFORM_GUID, L"\\\\.\\platform")) {
            log_error("platform", "failed to install platform device");
        }
    }

    // Must be the last thing called!
    prebind_hooks();
    setup_hooks();
}

void tea_hook_test(char* fmt, ...) {
    va_list argp;
    va_start(argp, fmt);
    vlog_game("tea", fmt, argp);
    va_end(argp);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call != DLL_PROCESS_ATTACH) return TRUE;

    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    wcscpy_s(exePath, MAX_PATH + 1, PathFindFileNameW(exePath));

    HMODULE exeModule = GetModuleHandleA(NULL);
    init_injection(exeModule);

    if (wcscmp(exePath, L"InitialD8_GLW_RE_SBZZ_dumped_.exe") == 0) {
        CreateHook((void*)(0x00407850), &tea_hook_test, 5);
        // *((DWORD*)(0x00407850)) = (DWORD)(&logcb);
    }

    if (wcscmp(exePath, L"RingGame.exe") == 0) {
        log_warning(BOOT_LOGGER, "Bodge hook goo!");
        CreateHook((void*)(0x005f2580), &tea_hook_test, 5);
    }

    return TRUE;
}
