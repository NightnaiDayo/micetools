#include <detours.h>
#include <signal.h>

#include "../lib/util/pid.h"
#include "common.h"
#include "devices/_devices.h"
#include "drivers/mx.h"
#include "hooks/_hooks.h"

WCHAR exeName[MAX_PATH + 1];
char exeNameC[MAX_PATH + 1];
size_t imageOffset;

void apply_patches(HMODULE hModule) {
    void* baseAddress = (void*)hModule;

    DWORD imageBase = MiceGetImageBase();
    if (imageBase == 0) {
        log_error(plfBoot, "Failed to locate image base. Patches will not be applied.");
        return;
    }
    imageOffset = (DWORD)hModule - imageBase;

    if (!load_patches(MiceConfig.mice.patches_file, exeNameC)) {
        log_error(plfBoot, "Failed to load patches file %s", MiceConfig.mice.patches_file);
        return;
    }

    patch_t* patch = patch_list->next;
    if (patch == NULL) {
        log_info(plfBoot, "No patches to apply. Skipping.");
        return;
    }
    while (patch) {
        DWORD oldProt;
        VirtualProtect((void*)(patch->address + imageOffset), patch->count, PAGE_EXECUTE_READWRITE,
                       &oldProt);

        if (memcmp(patch->match, (void*)(patch->address + imageOffset), patch->count) != 0) {
            log_error(plfBoot, "Patch %s failed! from-value missmatch", patch->name);
            VirtualProtect((void*)(patch->address + imageOffset), patch->count, oldProt, &oldProt);
            patch = patch->next;
            continue;
        }

        memcpy((void*)(patch->address + imageOffset), patch->replace, patch->count);
        log_misc(plfBoot, "Patched %d bytes at %08x (%s)", patch->count, patch->address,
                 patch->name);

        patch = patch->next;
    }
}

void prebind_hooks() {
    hook_all();
    install_devices();
}

void init_injection(HMODULE hModule) {
    WideCharToMultiByte(CP_ACP, 0, exeName, -1, exeNameC, sizeof exeNameC, NULL, NULL);

    // We're in a new context now, so need to reconfigure
    setup_logging();
    // Make sure our CRC32 tables are ready for anything that might want to use them
    amiCrc32RInit();

    load_mice_config();
    load_keybind_config();

    MiceSetLogBasename(exeNameC);
    log_info(plfBoot, "Handover complete. Now executing within %ls", exeName);

    // Start Mice FS
    if (!MiceFSInit()) {
        log_error(plfFile, "Failed to initialise Mice FS stack: %d", GetLastError());
        exit(1);
    }

    BOOL success = TRUE;
    success &= MiceFSAddDevLayers();

    CHAR szWorkPath[MAX_PATH + 1];
    if (!MiceConfig.mice.original || MiceConfig.mice.original[0] == '\0') {
        _GetCurrentDirectoryA(sizeof szWorkPath, szWorkPath);
        success &= MiceFSAddLayer(RING_MOUNT_ORIGINAL, szWorkPath);
        success &= MiceFSAddLayer(RING_MOUNT_GAME, szWorkPath);
    } else {
        success &= MiceFSAddLayer(RING_MOUNT_ORIGINAL, MiceConfig.mice.original);
        success &= MiceFSAddLayer(RING_MOUNT_GAME, MiceConfig.mice.original);
    }
    if (MiceConfig.mice.patch && MiceConfig.mice.patch[0] != '\0') {
        success &= MiceFSAddLayer(RING_MOUNT_PATCH, MiceConfig.mice.patch);
        success &= MiceFSAddLayer(RING_MOUNT_GAME, MiceConfig.mice.patch);
    }
    if (MiceConfig.mice.extend && MiceConfig.mice.extend[0] != '\0') {
        success &= MiceFSAddLayer(RING_MOUNT_EXTEND_VOL, MiceConfig.mice.extend);
        success &= MiceFSAddLayer(RING_MOUNT_GAME, MiceConfig.mice.extend);
    }
    if (MiceConfig.mice.extend2 && MiceConfig.mice.extend2[0] != '\0') {
        MiceFSAddLayer(RING_MOUNT_EXTEND2_VOL, MiceConfig.mice.extend2);
        MiceFSAddLayer(RING_MOUNT_GAME, MiceConfig.mice.extend2);
    }

    success &= MiceFSAddRingedgeLayers(FALSE);

    if (!success) {
        log_error(plfFile, "Failed to add Mice FS layers: %d", GetLastError());
        exit(1);
    }

    szWorkPath[0] = '\0';
    strcat_s(szWorkPath, sizeof szWorkPath, RING_MOUNT_GAME "\\");
    if (_miceIpcData && _miceIpcData->m_LauncherIsReady)
        strcat_s(szWorkPath, sizeof szWorkPath, _miceIpcData->m_PathPrefix);
    MiceFSSetCwd(szWorkPath);

    // Setup default COM devices
    init_com_devices();

    // Apply any in-memory patches requested
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
    // CrackProof
    if (MiceConfig.drivers.htsysmnt) setup_htsysmnt();

    if (MiceConfig.drivers.platform) {
        if (!add_fake_device(&PLATFORM_GUID, L"\\\\.\\platform")) {
            log_error(plfPlatform, "failed to install platform device");
        }
    }

    // Must be the last thing called!
    prebind_hooks();

    setup_hooks();

    start_devices();
}

void tea_hook_test(char* fmt, ...) {
    va_list argp;
    va_start(argp, fmt);
    vlog_game(plfTea, fmt, argp);
    va_end(argp);
}

BOOL g_bIsInDllMain = TRUE;
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call != DLL_PROCESS_ATTACH) return TRUE;
    g_bIsInDllMain = TRUE;

    GetModuleFileNameW(NULL, exeName, MAX_PATH);
    wcscpy_s(exeName, MAX_PATH + 1, PathFindFileNameW(exeName));

    if (!MiceIpcSetup(FALSE)) {
        fprintf(stderr, "Failed to initialise IPC from within micelib. Bailing hard!");
        TerminateProcess(GetCurrentProcess(), 0);
        return FALSE;
    }

    HMODULE exeModule = GetModuleHandleA(NULL);
    init_injection(exeModule);

    _MiceGotGameId(_miceIpcData->m_GameId);

    // InitialD 8
    if (false) {
        DWORD dword = 0;
        MiceHookPatchAtImage(exeModule, (PVOID)0x0126867c, &dword, 4);

        BYTE movEax1[] = { 0xb8, 0x01, 0x00, 0x00, 0x00 };
        // Bypass cgGcIsProfileSupported
        MiceHookNopAtImage(exeModule, (PVOID)0xb4c9ac, 6);
        MiceHookPatchAtImage(exeModule, (PVOID)0xb4c9ac, movEax1, sizeof movEax1);
        MiceHookNopAtImage(exeModule, (PVOID)0xb4ca9f, 6);
        MiceHookPatchAtImage(exeModule, (PVOID)0xb4ca9f, movEax1, sizeof movEax1);

        // change glProgramEnvParameters4fvNV to EXT version that works with any GPUs
        static const char* arb = "glProgramEnvParameters4fvEXT";
        MiceHookPatchAtImage(exeModule, (PVOID)0xB70E26, &arb, 4);
        // force volatile texture (driver bug(?) check) presence
        MiceHookNopAtImage(exeModule, (PVOID)(0xB6BC20 + 0x5667), 2);

        DWORD profile;
        BOOL amd = TRUE;
        if (amd)
            profile = 6151;  // CG_PROFILE_FP40
        else
            profile = 7000;  // CG_PROFILE_ARBFP1

        // Vertex shader is compiled with profile 6150
        // Pixel shader is compiled with profile 7000

        // Change shader profile for glareGenerator
        MiceHookPatchAtImage(exeModule, (PVOID)0x7D06A8, &profile, 4);

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        void* original = (void*)0x00407850;
        DetourAttach(&original, tea_hook_test);
        DetourTransactionCommit();
    }

    g_bIsInDllMain = FALSE;
    return TRUE;
}
