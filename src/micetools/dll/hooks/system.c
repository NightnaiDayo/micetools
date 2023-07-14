#include "system.h"

#include "./files.h"

OSVERSIONINFOA OS_VERSION = {
    .dwOSVersionInfoSize = 148,
    .dwMajorVersion = 5,
    .dwMinorVersion = 1,
    .dwBuildNumber = 2600,
    .dwPlatformId = VER_PLATFORM_WIN32_NT,
    .szCSDVersion = "Service Pack 3",
};
WCHAR TEMP_PATH[] = L"C:\\DOCUME~1\\SYSTEM~1\\LOCALS~1\\Temp\\";

BOOL WINAPI FakeGetVersionExA(LPOSVERSIONINFOA lpVersionInformation) {
    log_trace(plfSystem, "GetVersionExA");
    memcpy(lpVersionInformation, &OS_VERSION, sizeof OS_VERSION);
    return TRUE;
}
BOOL WINAPI FakeGetVolumeInformationW(LPCWSTR lpRootPathName, LPWSTR lpVolumeNameBuffer,
                                      DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber,
                                      LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags,
                                      LPWSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize) {
    log_trace(plfSystem, "GetVolumeInformationW");
    if (lpVolumeNameBuffer && nVolumeNameSize) lpVolumeNameBuffer[0] = '\0';
    if (lpVolumeSerialNumber) *lpVolumeSerialNumber = 0x00144db0;
    if (lpMaximumComponentLength) *lpMaximumComponentLength = 0xff;
    if (lpFileSystemFlags) *lpFileSystemFlags = 0x700ff;
    if (lpFileSystemNameBuffer) wcsncpy_s(lpFileSystemNameBuffer, nFileSystemNameSize, L"NTFS", 5);
    return TRUE;
}

DWORD WINAPI FakeGetTempPathW(DWORD nBufferLength, LPWSTR lpBuffer) {
    memcpy(lpBuffer, TEMP_PATH, sizeof TEMP_PATH);
    return wcslen(TEMP_PATH);
}

HCURSOR WINAPI FakeLoadCursorFromFileA(LPCSTR lpFileName) { return (HANDLE)1; }
BOOL WINAPI FakeSetSystemCursor(HCURSOR hcur, DWORD id) { return TRUE; }
BOOL WINAPI FakeDeleteObject(HGDIOBJ ho) { return TRUE; }

FARPROC WINAPI FakeGetProcAddress(HMODULE hModule, LPCSTR lpProcName) {
    // log_error(plfSystem, "GetProcAddress(%s)", lpProcName);
    return TrueGetProcAddress(hModule, lpProcName);
}
HMODULE WINAPI FakeGetModuleHandleA(LPCSTR lpModuleName) {
    // log_game(plfSystem, "GetModuleHandleA(%s)", lpModuleName);
    return TrueGetModuleHandleA(lpModuleName);
}
HMODULE WINAPI FakeGetModuleHandleW(LPCWSTR lpModuleName) {
    // log_game(plfSystem, "GetModuleHandleW(%ls)", lpModuleName);
    return TrueGetModuleHandleW(lpModuleName);
}

LONG WINAPI FakeRtlGetVersion(PRTL_OSVERSIONINFOW lpVersionInformation) {
    log_trace(plfSystem, "RtlGetVersion(%p)", lpVersionInformation);

    if (lpVersionInformation->dwOSVersionInfoSize >= sizeof(OSVERSIONINFOW)) {
        lpVersionInformation->dwMajorVersion = OS_VERSION.dwMajorVersion;
        lpVersionInformation->dwMinorVersion = OS_VERSION.dwMinorVersion;
        lpVersionInformation->dwBuildNumber = OS_VERSION.dwBuildNumber;
    }
    if (lpVersionInformation->dwOSVersionInfoSize >= sizeof(OSVERSIONINFOEXW)) {
        PRTL_OSVERSIONINFOEXW lpVersionInformationEx = (PRTL_OSVERSIONINFOEXW)lpVersionInformation;
        lpVersionInformationEx->wServicePackMajor = 3;
        lpVersionInformationEx->wServicePackMinor = 0;
    }
    return 0;
}

// TODO: We should probably handle libamv_amd.dll at some point too
HMODULE WINAPI FakeLoadLibraryA(LPCSTR lpLibFileName) {
    if (_stricmp(lpLibFileName, "libamv_nvidia.dll") == 0 ||
        _stricmp(lpLibFileName, "libamv_amd.dll") == 0 ||
        _stricmp(lpLibFileName, "atipdlxx.dll") == 0) {
        return TrueLoadLibraryA(MICELIB);
    }
    return TrueLoadLibraryA(lpLibFileName);
}
#define WIDEN2(x) L##x
#define WIDEN(x) WIDEN2(x)
HMODULE WINAPI FakeLoadLibraryW(LPCWSTR lpLibFileName) {
    if (_wcsicmp(lpLibFileName, L"libamv_nvidia.dll") == 0 ||
        _wcsicmp(lpLibFileName, L"libamv_amd.dll") == 0 ||
        _wcsicmp(lpLibFileName, L"atipdlxx.dll") == 0) {
        return TrueLoadLibraryW(WIDEN(MICELIB));
    }
    return TrueLoadLibraryW(lpLibFileName);
}

void hook_system() {
    // TODO: This should be part of drives/hooks.c
    hook("Kernel32.dll", "GetVolumeInformationW", FakeGetVolumeInformationW, NULL);
    hook("Kernel32.dll", "GetTempPathW", FakeGetTempPathW, NULL);
    // hook("Kernel32.dll", "GetVersionExA", FakeGetVersionExA, NULL);
    hook("Kernel32.dll", "GetProcAddress", FakeGetProcAddress, (void*)&TrueGetProcAddress);
    hook("Kernel32.dll", "GetModuleHandleA", FakeGetModuleHandleA, (void*)&TrueGetModuleHandleA);
    hook("Kernel32.dll", "GetModuleHandleW", FakeGetModuleHandleW, (void*)&TrueGetModuleHandleW);
    hook("Kernel32.dll", "LoadLibraryA", FakeLoadLibraryA, (void*)&TrueLoadLibraryA);
    hook("Kernel32.dll", "LoadLibraryW", FakeLoadLibraryW, (void*)&TrueLoadLibraryW);

    // hook("ntdll.dll", "RtlGetVersion", FakeRtlGetVersion, NULL);

    // hook("User32.dll", "LoadCursorFromFileA", FakeLoadCursorFromFileA, NULL);
    // hook("User32.dll", "SetSystemCursor", FakeSetSystemCursor, NULL);
    // hook("User32.dll", "DeleteObject", FakeDeleteObject, NULL);

    const char* SystemVersion = "00691001\r\n";
    const char* UpdateVersion = "0000\r\n";

    HANDLE hGM = CreateFileA("GrooveMaster.ini", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    BOOL bGmIs1P = FALSE;
    if (hGM != INVALID_HANDLE_VALUE) {
        CHAR line[256];
        DWORD_PTR lidx = 0;
        DWORD nRead = 0;
        while (1) {
            // Literally not worth recovering; we're working with something stupid.
            // Play stupid games, win stupid prizes.
            if (lidx == _countof(line)) break;

            ReadFile(hGM, &(line[lidx]), 1, &nRead, NULL);
            if (nRead != 1) break;
            if (line[lidx] == '\n') {
                if (strncmp(line, "1P_ONLY 1\r\n", lidx) == 0) {
                    bGmIs1P = TRUE;
                    break;
                }
                lidx = 0;
            } else
                lidx++;
        }
        CloseHandle(hGM);
    }

    // You can have any configuration you want as long as it's black
    if (bGmIs1P) {
        const char* GrooveMaster1P =
            ("# TODO: Get maimai_deliver not rebooting the cab\r\n"
             "NO_DELIVER 1\r\n"
             "# TODO: figure out how to do this without also needing a USB\r\n"
             "USB_DL_DISABLE 1\r\n"
             "# These are just easier to do via GM than a patch\r\n"
             "1P_ONLY 1\r\n"
             "NO_REBOOT 1\r\n"
             "ROTATE 1\r\n");
        hook_file_with_buffer(L"GrooveMaster.ini", (LPBYTE)GrooveMaster1P,
                              strlen(GrooveMaster1P) + 1, GENERIC_READ);
    } else {
        const char* GrooveMaster =
            ("# TODO: Get maimai_deliver not rebooting the cab\r\n"
             "NO_DELIVER 1\r\n"
             "# TODO: figure out how to do this without also needing a USB\r\n"
             "USB_DL_DISABLE 1\r\n"
             "# These are just easier to do via GM than a patch\r\n"
             "NO_REBOOT 1\r\n"
             "ROTATE 1\r\n");
        hook_file_with_buffer(L"GrooveMaster.ini", (LPBYTE)GrooveMaster,
                              strlen(GrooveMaster) + 1, GENERIC_READ);
    }

    const char* RingmasterPub =
        ("-----BEGIN PUBLIC KEY-----\r\n"
         "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDH/y0LFuiVonnU+7fKLEOhfQoi\r\n"
         "uElB6f9+MVc+VwLzCNV/xU05TWJgm82m/lsmtYwArrA9gHHCB7ExgkaH3kDmd4l6\r\n"
         "FumWIRCO/7Z4pbIFSb9xvPYWn7GJJvtJKn2OU/t7zt4nP3MiR0J4lqtT88x6F4Ui\r\n"
         "UeI3d2jT+Fw1dgRn7wIDAQAB\r\n"
         "-----END PUBLIC KEY-----\r\n");

    // We're going to violate the non-const data requirements for these files,
    // because we're only allowing GENERIC_READ. In the future this may be made
    // more generic.
    hook_file_with_buffer(L"C:\\System\\SystemVersion.txt", (LPBYTE)SystemVersion,
                          strlen(SystemVersion) + 1, GENERIC_READ);
    hook_file_with_buffer(L"C:\\System\\UpdateVersion.txt", (LPBYTE)UpdateVersion,
                          strlen(UpdateVersion) + 1, GENERIC_READ);
    hook_file_with_buffer(L"c:\\System\\Execute\\ringmaster_pub.pem", (LPBYTE)RingmasterPub,
                          strlen(RingmasterPub) + 1, GENERIC_READ);
}
