#include <Windows.h>
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#include <stdio.h>

#include "../lib/mice/mice.h"
#include "drivers/mx.h"
#include "files.h"
#include "com.h"
#include "micesetupapi.h"

WCHAR exePath[MAX_PATH + 1];

void enable_traces() {
    if (wcscmp(exePath, L"Game.exe") == 0) {
        *(DWORD*)(0x008717a0) = 1;
        *(DWORD*)(0x00871728) = 1;
        *(DWORD*)(0x00871cd8) = 1;
        *(DWORD*)(0x00872660) = 1;
        *(DWORD*)(0x0087297c) = 1;  // LOG_EN_EEPROM
        *(DWORD*)(0x00872980) = 1;
        *(DWORD*)(0x00872988) = 1;
        *(DWORD*)(0x00873540) = 1;
        *(DWORD*)(0x00873538) = 1;
        *(DWORD*)(0x0087f890) = 1;
        *(DWORD*)(0x00882ce4) = 1;
        *(DWORD*)(0x00882cec) = 1;
        *(DWORD*)(0x00883018) = 1;
        *(DWORD*)(0x00886ff8) = 1;
        *(DWORD*)(0x0088b1b8) = 1;
        *(DWORD*)(0x0088b1c0) = 1;
    } else if (wcscmp(exePath, L"ALLNetProc_Win.exe") == 0) {
        *(DWORD*)(0x004f48b0) = 1;  // LOG_EN_DONGLE
        *(DWORD*)(0x004f989c) = 1;  // LOG_EN_PCP
        *(DWORD*)(0x004f7d04) = 1;  // LOG_EN_EEPROM
        *(DWORD*)(0x004f7d08) = 1;  // LOG_EN_BACKUP
        *(DWORD*)(0x004f8118) = 1;  // LOG_EN_RTC
        *(DWORD*)(0x004f96dc) = 1;  // LOG_EN_GFETCHER
        *(DWORD*)(0x004f8338) = 1;  // LOG_EN_NETWORK
        *(DWORD*)(0x004f88c0) = 1;  // LOG_EN_MASTER
        *(DWORD*)(0x004f88b0) = 1;  // LOG_EN_HM
        *(DWORD*)(0x004f8330) = 1;  // LOG_EN_SRAM
        *(DWORD*)(0x004f9728) = 1;  // LOG_EN_PLATFORM

        //*(DWORD*)(0x004f9e7c) = 5;  // LOG_EN_ASSERTS (handled by the bellow
        // instead)
        *(DWORD*)(0x004e884c) = 10;  // ALPB_LOG_LEVEL (0=None, 1=Error,
                                     // 2=Warning, 3=Most, 4=Debug)
    } else if (wcscmp(exePath, L"ORIG_mxsegaboot.exe") == 0 || wcscmp(exePath, L"mxsegaboot.exe") == 0) {
        // start of am
        *(DWORD*)(0x0054fcd8) = 1;  // LOG_EN_JVS_DRIVER
        *(DWORD*)(0x00538810) = 1;  // LOG_EN_DIPSW
        *(DWORD*)(0x0054afc8) = 1;  // LOG_EN_ATA
        *(DWORD*)(0x00544b78) = 1;  // LOG_EN_DONGLE
        *(DWORD*)(0x00536c98) = 1;  // LOG_EN_EEPROM
        *(DWORD*)(0x00537834) = 1;  // LOG_EN_GCATCHER
        *(DWORD*)(0x00537d4c) = 1;  // LOG_EN_GDELIVER
        *(DWORD*)(0x0053828c) = 1;  // LOG_EN_GFETCHER
        *(DWORD*)(0x00547fd0) = 1;  // LOG_EN_HWRESET
        *(DWORD*)(0x00547fd8) = 1;  // LOG_EN_PLATFORM
        *(DWORD*)(0x00548268) = 1;  // LOG_EN_HM
        *(DWORD*)(0x00536f30) = 1;  // LOG_EN_BACKUP
        *(DWORD*)(0x00547fcc) = 1;  // LOG_EN_CMOS
        *(DWORD*)(0x005382d8) = 1;  // LOG_EN_INSTALL
        *(DWORD*)(0x00538818) = 1;  // LOG_EN_JVS
        *(DWORD*)(0x00544b68) = 1;  // LOG_EN_JVSP
        *(DWORD*)(0x0054b3d8) = 1;  // LOG_EN_MASTER
        *(DWORD*)(0x00548280) = 1;  // LOG_EN_NETWORK
        *(DWORD*)(0x00548050) = 1;  // LOG_EN_RTC
        *(DWORD*)(0x00536c94) = 1;  // LOG_EN_SRAM
        *(DWORD*)(0x005487f8) = 1;  // LOG_EN_STORAGE
        *(DWORD*)(0x00536c90) = 1;  // LOG_EN_SYS
        // end of am
        *(DWORD*)(0x00536c70) = 1;  // LOG_EN_PCPT
    } else if (wcscmp(exePath, L"maimai_dump_.exe") == 0) {
        *(DWORD*)(0x00c8ab7c) = 1;  // LOG_EN_SRAM
        *(DWORD*)(0x00f406f0) = 1;  // LOG_EN_CABINET_DL
        *(DWORD*)(0x00c88680) = 1;  // LOG_EN_INSTALL
        *(DWORD*)(0x00c8ab78) = 1;  // LOG_EN_EEPROM
        *(DWORD*)(0x00c88608) = 1;  // LOG_EN_PLATFORM
        *(DWORD*)(0x00c88dd0) = 1;  // LOG_EN_STORAGE
        *(DWORD*)(0x00ca25ec) = 1;  // LOG_EN_PCP
        *(DWORD*)(0x00c96ed0) = 1;  // LOG_EN_HM
        *(DWORD*)(0x00f406ec) = 1;  // LOG_EN_HTTP
        *(DWORD*)(0x00c89ca8) = 1;  // LOG_EN_DIPSW
        *(DWORD*)(0x00c96ed8) = 1;  // LOG_EN_DONGLE
        *(DWORD*)(0x00c9a83c) = 1;  // LOG_EN_JVSP
        *(DWORD*)(0x00c89730) = 1;  // LOG_EN_NETWORK
        *(DWORD*)(0x00c9a440) = 1;  // LOG_EN_JVST_THREAD
        *(DWORD*)(0x00c8ab80) = 1;  // LOG_EN_JVS
        *(DWORD*)(0x00c89fc8) = 1;  // LOG_EN_MASTER
        *(DWORD*)(0x00c88bb8) = 1;  // LOG_EN_RTC
        *(DWORD*)(0x00c9e420) = 1;  // LOG_EN_JVST_DRIVER
        *(DWORD*)(0x00c89320) = 1;  // LOG_EN_BACKUP
        *(DWORD*)(0x00c88bb4) = 1;  // LOG_EN_HWRESET
        *(DWORD*)(0x00c830e8) = 1;  // LOG_EN
    } else if (wcscmp(exePath, L"mxnetwork.exe") == 0) {
        *(DWORD*)(0x004438e0) = 1;  // LOG_EN_PCP
        *(DWORD*)(0x004433f8) = 1;  // LOG_EN_UTL
    } else {
        log_warning("traces", "No known traces for %ls", exePath);
    }
}

void prebind_hooks() {
    hook_setupapi();
    hook_commio();
    hook_io();
}

void init_injection() {
    // We're in a new context now, so need to reconfigure
    setup_logging();
    log_info(BOOT_LOGGER, "Handover complete. Now executing within %ls", exePath);

    dmi_build_default();

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
