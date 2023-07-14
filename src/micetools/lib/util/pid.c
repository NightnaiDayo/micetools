#include <Windows.h>
#include <psapi.h>
#include <shlwapi.h>
#include <tlhelp32.h>

#include "../mice/exe.h"

#pragma comment(lib, "psapi.lib")

DWORD GetParentProcessIdFor(DWORD pid) {
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe = { 0 };
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(h, &pe)) {
        do {
            if (pe.th32ProcessID == pid) {
                CloseHandle(h);
                return pe.th32ParentProcessID;
            }
        } while (Process32Next(h, &pe));
    }
    CloseHandle(h);
    return (DWORD)-1;
}
DWORD GetParentProcessId(void) { return GetParentProcessIdFor(GetCurrentProcessId()); }
DWORD GetOutermostMiceId(void) {
    DWORD pid = GetCurrentProcessId();
    do {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (hProcess) {
            CHAR baseName[MAX_PATH + 1];
            if (GetModuleFileNameEx(hProcess, NULL, baseName, sizeof baseName)) {
                // TODO: Better
                if (_stricmp(PathFindFileNameA(baseName), MICEEXE) == 0)
                    return pid;
            }
            CloseHandle(hProcess);
        }

        pid = GetParentProcessIdFor(pid);
    } while (pid != (DWORD)-1);
    fprintf(stderr, "Likely fatal: Failed to identify parent mouse!");
    return (DWORD)-1;
}
