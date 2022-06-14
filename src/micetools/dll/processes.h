#include <Windows.h>
#include <stdbool.h>
#include <stdio.h>

static BOOL(WINAPI* TrueCreateProcessW)(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
                                        LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                        LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles,
                                        DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
                                        LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);

void hook_processes();
