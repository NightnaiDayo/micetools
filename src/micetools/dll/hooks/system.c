#include "system.h"

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
    *lpVolumeSerialNumber = 0x1c0fd22b;
    return TRUE;
};

DWORD WINAPI FakeGetTempPathW(DWORD nBufferLength, LPWSTR lpBuffer) {
    memcpy(lpBuffer, TEMP_PATH, sizeof TEMP_PATH);
    return wcslen(TEMP_PATH);
};

HCURSOR WINAPI FakeLoadCursorFromFileA(LPCSTR lpFileName) { return (HANDLE)1; }
BOOL FakeSetSystemCursor(HCURSOR hcur, DWORD id) { return TRUE; }
BOOL FakeDeleteObject(HGDIOBJ ho) { return TRUE; }

LONG WINAPI FakeChangeDisplaySettingsA(DEVMODEA* lpDevMode, DWORD dwFlags) { return 0; }
LONG WINAPI FakeChangeDisplaySettingsExA(LPCSTR lpszDeviceName, DEVMODEA* lpDevMode, HWND hwnd,
                                         DWORD dwflags, LPVOID lParam) {
    return 0;
}

void hook_system() {
    hook("Kernel32.dll", "GetVolumeInformationW", FakeGetVolumeInformationW, NULL, 5);
    hook("Kernel32.dll", "GetTempPathW", FakeGetTempPathW, NULL, 5);
    hook("Kernel32.dll", "GetVersionExA", FakeGetVersionExA, NULL, 5);

    hook("User32.dll", "ChangeDisplaySettingsA", FakeChangeDisplaySettingsA, NULL, 4);
    hook("User32.dll", "ChangeDisplaySettingsExA", FakeChangeDisplaySettingsExA, NULL, 6);

    // hook("User32.dll", "LoadCursorFromFileA", FakeLoadCursorFromFileA, NULL, 5);
    // hook("User32.dll", "SetSystemCursor", FakeSetSystemCursor, NULL, 5);
    // hook("User32.dll", "DeleteObject", FakeDeleteObject, NULL, 5);
}
