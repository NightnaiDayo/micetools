#pragma once

#include "../common.h"

FARPROC (*TrueGetProcAddress)(HMODULE hModule, LPCSTR lpProcName);
HMODULE (*TrueGetModuleHandleA)(LPCSTR lpModuleName);

void hook_system();
