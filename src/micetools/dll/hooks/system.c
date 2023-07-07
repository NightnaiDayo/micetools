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
BOOL FakeSetSystemCursor(HCURSOR hcur, DWORD id) { return TRUE; }
BOOL FakeDeleteObject(HGDIOBJ ho) { return TRUE; }

FARPROC FakeGetProcAddress(HMODULE hModule, LPCSTR lpProcName) {
    log_trace(plfSystem, "GetProcAddress(%s)", lpProcName);
    return TrueGetProcAddress(hModule, lpProcName);
}
HMODULE FakeGetModuleHandleA(LPCSTR lpModuleName) {
    log_trace(plfSystem, "GetModuleHandleA(%s)", lpModuleName);
    return TrueGetModuleHandleA(lpModuleName);
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
    // hook("Kernel32.dll", "GetProcAddress", FakeGetProcAddress, (void*)&TrueGetProcAddress);
    // hook("Kernel32.dll", "GetModuleHandleA", FakeGetModuleHandleA, (void*)&TrueGetModuleHandleA);
    hook("Kernel32.dll", "LoadLibraryA", FakeLoadLibraryA, (void*)&TrueLoadLibraryA);
    hook("Kernel32.dll", "LoadLibraryW", FakeLoadLibraryW, (void*)&TrueLoadLibraryW);

    // hook("ntdll.dll", "RtlGetVersion", FakeRtlGetVersion, NULL);

    // hook("User32.dll", "LoadCursorFromFileA", FakeLoadCursorFromFileA, NULL);
    // hook("User32.dll", "SetSystemCursor", FakeSetSystemCursor, NULL);
    // hook("User32.dll", "DeleteObject", FakeDeleteObject, NULL);

    const char* SystemVersion = "00691001\r\n";
    const char* UpdateVersion = "0000\r\n";

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
