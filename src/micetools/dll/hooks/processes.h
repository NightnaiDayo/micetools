#pragma once
#include "../common.h"

static BOOL(WINAPI* TrueCreateProcessW)(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
                                        LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                        LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                        BOOL bInheritHandles, DWORD dwCreationFlags,
                                        LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
                                        LPSTARTUPINFOW lpStartupInfo,
                                        LPPROCESS_INFORMATION lpProcessInformation);
static BOOL(WINAPI* TrueCreateProcessA)(LPCSTR lpApplicationName, LPSTR lpCommandLine,
                                        LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                        LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                        BOOL bInheritHandles, DWORD dwCreationFlags,
                                        LPVOID lpEnvironment, LPCSTR lpCurrentDirectory,
                                        LPSTARTUPINFOW lpStartupInfo,
                                        LPPROCESS_INFORMATION lpProcessInformation);
static HINSTANCE(WINAPI* TrueShellExecuteA)(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile,
                                            LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);

void hook_processes();
