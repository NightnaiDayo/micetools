#include "registry.h"

LSTATUS WINAPI FakeRegCloseKey(HKEY hKey) {
    log_trace("registry", "RegCloseKey %08x", hKey);
    return ERROR_SUCCESS;
}
LSTATUS WINAPI FakeRegCreateKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass,
                                   DWORD dwOptions, REGSAM samDesired,
                                   const LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                   PHKEY phkResult, LPDWORD lpdwDisposition) {
    log_trace("registry", "RegCreateKeyExA %08x %s", hKey, lpSubKey);
    return ERROR_SUCCESS;
}
LSTATUS WINAPI FakeRegDeleteKeyA(HKEY hKey, LPCSTR lpSubKey) {
    log_trace("registry", "RegDeleteKeyA %08x %s", hKey, lpSubKey);
    return ERROR_SUCCESS;
}
LSTATUS WINAPI FakeRegDeleteKeyValueA(HKEY hKey, LPCSTR lpSubKey, LPCSTR lpValueName) {
    log_trace("registry", "RegDeleteKeyValueA %08x %s %s", hKey, lpSubKey, lpValueName);
    return ERROR_SUCCESS;
}
LSTATUS WINAPI FakeRegEnumKeyExA(HKEY hKey, DWORD dwIndex, LPSTR lpName, LPDWORD lpcchName,
                                 LPDWORD lpReserved, LPSTR lpClass, LPDWORD lpcchClass,
                                 PFILETIME lpftLastWriteTime) {
    log_trace("registry", "RegEnumKeyExA %08x[%d]", hKey, dwIndex);
    if (dwIndex == 0) {
        strcpy(lpName, "Direct3D HAL");
        return ERROR_SUCCESS;
    }
    return ERROR_NO_MORE_ITEMS;
}
LSTATUS WINAPI FakeRegEnumValueA(HKEY hKey, DWORD dwIndex, LPSTR lpValueName,
                                 LPDWORD lpcchValueName, LPDWORD lpReserved, LPDWORD lpType,
                                 LPBYTE lpData, LPDWORD lpcbData) {
    log_trace("registry", "RegEnumValueA %08x %s", hKey, lpValueName);
    return ERROR_NO_MORE_ITEMS;
}

void hook_registry() {
    hook("Advapi32.dll", "RegCloseKey", FakeRegCloseKey, (void**)&TrueRegCloseKey);
    hook("Advapi32.dll", "RegCreateKeyExA", FakeRegCreateKeyExA, (void**)&TrueRegCreateKeyExA);
    hook("Advapi32.dll", "RegDeleteKeyA", FakeRegDeleteKeyA, (void**)&TrueRegDeleteKeyA);
    hook("Advapi32.dll", "RegDeleteKeyValueA", FakeRegDeleteKeyValueA,
         (void**)&TrueRegDeleteKeyValueA);
    hook("Advapi32.dll", "RegEnumKeyExA", FakeRegEnumKeyExA, (void**)&TrueRegEnumKeyExA);
    // TODO: Passthrough
    // hook("Advapi32.dll", "RegEnumValueA", FakeRegEnumValueA, (void**)&TrueRegEnumValueA);
}
