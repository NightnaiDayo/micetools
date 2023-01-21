#pragma once

#ifndef _MICE_COM
#define _MICE_COM extern
#endif

#include "../common.h"

_MICE_COM BOOL(WINAPI* TrueGetCommState)(HANDLE hFile, LPDCB lpDCB);
_MICE_COM BOOL(WINAPI* TrueSetCommState)(HANDLE hFile, LPDCB lpDCB);
_MICE_COM BOOL(WINAPI* TrueGetCommTimeouts)(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts);
_MICE_COM BOOL(WINAPI* TrueSetCommTimeouts)(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts);
_MICE_COM BOOL(WINAPI* TrueSetupComm)(HANDLE hFile, DWORD dwInQueue, DWORD dwOutQueue);
_MICE_COM BOOL(WINAPI* TruePurgeComm)(HANDLE hFile, DWORD dwFlags);
_MICE_COM BOOL(WINAPI* TrueGetCommModemStatus)(HANDLE hFile, LPDWORD lpModelStat);
_MICE_COM BOOL(WINAPI* TrueWaitCommEvent)(HANDLE hFile, LPDWORD lpEvtMask, LPOVERLAPPED lpOverlapped);
_MICE_COM BOOL(WINAPI* TrueClearCommError)(HANDLE hFile, LPDWORD lpErrors, LPCOMSTAT lpStat);

typedef BOOL(FnGetCommState)(HANDLE hFile, LPDCB lpDCB);
typedef BOOL(FnSetCommState)(HANDLE hFile, LPDCB lpDCB);
typedef BOOL(FnGetCommTimeouts)(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts);
typedef BOOL(FnSetCommTimeouts)(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts);
typedef BOOL(FnSetupComm)(HANDLE hFile, DWORD dwInQueue, DWORD dwOutQueue);
typedef BOOL(FnPurgeComm)(HANDLE hFile, DWORD dwFlags);
typedef BOOL(FnGetCommModemStatus)(HANDLE hFile, LPDWORD lpModelStat);
typedef BOOL(FnWaitCommEvent)(HANDLE hFile, LPDWORD lpEvtMask, LPOVERLAPPED lpOverlapped);
typedef BOOL(FnClearCommError)(HANDLE hFile, LPDWORD lpErrors, LPCOMSTAT lpStat);

typedef struct com_hook {
    LPHANDLE virtual_handle;
    WCHAR wName[16];
    WCHAR wDosName[16];
    BYTE com;

    FnGetCommState* GetCommState;
    FnSetCommState* SetCommState;
    FnGetCommTimeouts* GetCommTimeouts;
    FnSetCommTimeouts* SetCommTimeouts;
    FnSetupComm* SetupComm;
    FnPurgeComm* PurgeComm;
    FnGetCommModemStatus* GetCommModemStatus;
    FnWaitCommEvent* WaitCommEvent;
    FnClearCommError* ClearCommError;

    void* com_device;

    struct com_hook* next;
} com_hook_t;

com_hook_t* new_com_hook(BYTE port);
void hook_com(com_hook_t* hook);

void hook_commio();
