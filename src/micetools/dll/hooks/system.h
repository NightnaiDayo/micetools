#pragma once

#include "../common.h"

FARPROC(WINAPI* TrueGetProcAddress)(HMODULE hModule, LPCSTR lpProcName);
HMODULE(WINAPI* TrueGetModuleHandleA)(LPCSTR lpModuleName);
HMODULE(WINAPI* TrueGetModuleHandleW)(LPCWSTR lpModuleName);
HMODULE(WINAPI* TrueLoadLibraryA)(LPCSTR lpLibFileName);
HMODULE(WINAPI* TrueLoadLibraryW)(LPCWSTR lpLibFileName);

void hook_system();
