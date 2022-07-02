#include "common.h"
#include "devices/_devices.h"
#include "drivers/mx.h"
#include "hooks/_hooks.h"

WCHAR exePath[MAX_PATH + 1];

void enable_traces() {
    patches_t patches;
    char error[256];
    if (!load_patches(&patches, "patches.json", error)) {
        log_error(BOOT_LOGGER, "Failed to load patches file: %s", error);
    } else {
        char exePathC[MAX_PATH + 1];
        WideCharToMultiByte(CP_ACP, 0, exePath, -1, exePathC, sizeof exePathC, NULL, NULL);

        for (size_t i = 0; i < patches.nopatchsets; i++) {
            patchset_t* patchset = patches.patchsets[i];

            // Require the binary explicitly named
            if (patchset->binary_name == NULL || strcmp(patchset->binary_name, exePathC) != 0) {
                continue;
            }
            if (!patchset->apply) continue;

            for (size_t j = 0; j < patchset->nopatches; j++) {
                patch_t patch = patchset->patches[j];

                if (memcmp(patch.from, (void*)patch.offset, patch.count) != 0) {
                    log_error(BOOT_LOGGER, "Patch %s[%d] failed! from-value missmatch", patchset->name, j);
                    continue;
                }

                memcpy((void*)patch.offset, patch.to, patch.count);
                log_misc(BOOT_LOGGER, "Patched %d bytes at %08x", patch.count, patch.offset);
            }
        }
    }
    free_patches(&patches);
}

void prebind_hooks() {
    hook_all();
    install_devices();

    // TODO: Figure out why we're needing to call this manually (medium priority)
    if (wcscmp(exePath, L"ALLNetProc.exe") == 0) {
        // OPENSSL_add_all_algorithms_noconf
        ((void (*)(void))(0x00459770))();
    }
}

void init_injection() {
    // We're in a new context now, so need to reconfigure
    setup_logging();
    log_info(BOOT_LOGGER, "Handover complete. Now executing within %ls", exePath);

    enable_traces();

    setup_columba();
    setup_mxsram();
    setup_mxsuperio();
    setup_mxjvs();
    setup_mxhwreset();
    setup_mxsmbus();

    if (!add_fake_device(&PLATFORM_GUID, L"\\\\.\\platform")) {
        log_error("platform", "failed to install platform device");
    }

    // Must be the last thing called!
    // register_devices();
    prebind_hooks();
    setup_hooks();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call != DLL_PROCESS_ATTACH) return TRUE;

    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    wcscpy_s(exePath, MAX_PATH + 1, PathFindFileNameW(exePath));

    init_injection();

    return TRUE;
}
