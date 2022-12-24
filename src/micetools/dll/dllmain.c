#include "common.h"
#include "devices/_devices.h"
#include "drivers/mx.h"
#include "hooks/_hooks.h"

WCHAR exePath[MAX_PATH + 1];

void apply_patches() {
    char exePathC[MAX_PATH + 1];
    WideCharToMultiByte(CP_ACP, 0, exePath, -1, exePathC, sizeof exePathC, NULL, NULL);

    patches_t patches;
    char error[256];
    if (!load_patches(&patches, MiceConfig.mice.patches_file, error, exePathC)) {
        log_error(BOOT_LOGGER, "Failed to load patches file: %s", error);
        return;
    }

    for (size_t i = 0; i < patches.nopatchsets; i++) {
        patchset_t* patchset = patches.patchsets[i];

        // Require the binary explicitly named
        if (patchset->binary_name == NULL || strcmp(patchset->binary_name, exePathC) != 0) {
            continue;
        }
        if (!patchset->apply) continue;

        for (size_t j = 0; j < patchset->nopatches; j++) {
            patch_t patch = patchset->patches[j];

            DWORD oldProt;
            VirtualProtect((void*)patch.offset, patch.count, PAGE_EXECUTE_READWRITE, &oldProt);

            if (memcmp(patch.from, (void*)patch.offset, patch.count) != 0) {
                log_error(BOOT_LOGGER, "Patch %s[%d] failed! from-value missmatch", patchset->name,
                          j);
                VirtualProtect((void*)patch.offset, patch.count, oldProt, &oldProt);
                continue;
            }

            memcpy((void*)patch.offset, patch.to, patch.count);
            log_misc(BOOT_LOGGER, "Patched %d bytes at %08x", patch.count, patch.offset);
            VirtualProtect((void*)patch.offset, patch.count, oldProt, &oldProt);
        }
    }
    free_patches(&patches);
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

void init_injection() {
    load_mice_config();

    // We're in a new context now, so need to reconfigure
    setup_logging();
    log_info(BOOT_LOGGER, "Handover complete. Now executing within %ls", exePath);

    if (MiceConfig.mice.apply_patches) apply_patches();

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

    init_injection();

    if (wcscmp(exePath, L"InitialD8_GLW_RE_SBZZ_dumped_.exe") == 0) {
        CreateHook((void*)(0x00407850), &tea_hook_test, 5);
        // *((DWORD*)(0x00407850)) = (DWORD)(&logcb);
    }

    return TRUE;
}
