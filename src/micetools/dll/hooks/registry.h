#pragma once
#include "common.h"

static LSTATUS (WINAPI *TrueRegCloseKey)(HKEY hKey);
static LSTATUS (WINAPI *TrueRegCreateKeyExA)(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass,
                                  DWORD dwOptions, REGSAM samDesired,
                                  const LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult,
                                  LPDWORD lpdwDisposition);
static LSTATUS (WINAPI *TrueRegDeleteKeyA)(HKEY hKey, LPCSTR lpSubKey);
static LSTATUS (WINAPI *TrueRegDeleteKeyValueA)(HKEY hKey, LPCSTR lpSubKey, LPCSTR lpValueName);
static LSTATUS (WINAPI *TrueRegEnumKeyExA)(HKEY hKey, DWORD dwIndex, LPSTR lpName, LPDWORD lpcchName,
                                LPDWORD lpReserved, LPSTR lpClass, LPDWORD lpcchClass,
                                PFILETIME lpftLastWriteTime);
static LSTATUS (WINAPI *TrueRegEnumValueA)(HKEY hKey, DWORD dwIndex, LPSTR lpValueName, LPDWORD lpcchValueName,
                                LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData,
                                LPDWORD lpcbData);

void hook_registry();
