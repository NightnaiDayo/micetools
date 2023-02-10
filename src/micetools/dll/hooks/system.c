#include "system.h"

#include "./files.h"

// const char OS_VERSION[] = "Service Pack 3";

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
    log_trace("system", "GetVersionExA");
    memcpy(lpVersionInformation, &OS_VERSION, sizeof OS_VERSION);
    return TRUE;
}
BOOL WINAPI FakeGetVolumeInformationW(LPCWSTR lpRootPathName, LPWSTR lpVolumeNameBuffer,
                                      DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber,
                                      LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags,
                                      LPWSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize) {
    log_trace("system", "GetVolumeInformationW");
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

LONG WINAPI FakeChangeDisplaySettingsA(DEVMODEA* lpDevMode, DWORD dwFlags) { return 0; }
LONG WINAPI FakeChangeDisplaySettingsExA(LPCSTR lpszDeviceName, DEVMODEA* lpDevMode, HWND hwnd,
                                         DWORD dwflags, LPVOID lParam) {
    return 0;
}

void hook_system() {
    // TODO: This should be part of drives/hooks.c
    hook("Kernel32.dll", "GetVolumeInformationW", FakeGetVolumeInformationW, NULL);
    hook("Kernel32.dll", "GetTempPathW", FakeGetTempPathW, NULL);
    hook("Kernel32.dll", "GetVersionExA", FakeGetVersionExA, NULL);

    hook("User32.dll", "ChangeDisplaySettingsA", FakeChangeDisplaySettingsA, NULL);
    hook("User32.dll", "ChangeDisplaySettingsExA", FakeChangeDisplaySettingsExA, NULL);

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

    hook_file_with_buffer(L"C:\\System\\SystemVersion.txt", SystemVersion,
                          strlen(SystemVersion) + 1, GENERIC_READ);
    hook_file_with_buffer(L"C:\\System\\UpdateVersion.txt", UpdateVersion,
                          strlen(UpdateVersion) + 1, GENERIC_READ);
    hook_file_with_buffer(L"c:\\System\\Execute\\ringmaster_pub.pem", RingmasterPub,
                          strlen(RingmasterPub) + 1, GENERIC_READ);
}
