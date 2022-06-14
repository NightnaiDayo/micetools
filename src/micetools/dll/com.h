#include <Windows.h>
#include <stdbool.h>
#include <stdio.h>

static BOOL(WINAPI* TrueGetCommState)(HANDLE hFile, LPDCB lpDCB);
static BOOL(WINAPI* TrueSetCommState)(HANDLE hFile, LPDCB lpDCB);
static BOOL(WINAPI* TrueGetCommTimeouts)(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts);
static BOOL(WINAPI* TrueSetCommTimeouts)(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts);
static BOOL(WINAPI* TrueSetupComm)(HANDLE hFile, DWORD dwInQueue, DWORD dwOutQueue);
static BOOL(WINAPI* TruePurgeComm)(HANDLE hFile, DWORD dwFlags);
static BOOL(WINAPI* TrueGetCommModemStatus)(HANDLE hFile, LPDWORD lpModelStat);
static BOOL(WINAPI* TrueWaitCommEvent)(HANDLE hFile, LPDWORD lpEvtMask, LPOVERLAPPED lpOverlapped);
static BOOL(WINAPI* TrueClearCommError)(HANDLE hFile, LPDWORD lpErrors, LPCOMSTAT lpStat);

typedef BOOL(FnGetCommState)(void* com, LPDCB lpDCB);
typedef BOOL(FnSetCommState)(void* com, LPDCB lpDCB);
typedef BOOL(FnGetCommTimeouts)(void* com, LPCOMMTIMEOUTS lpCommTimeouts);
typedef BOOL(FnSetCommTimeouts)(void* com, LPCOMMTIMEOUTS lpCommTimeouts);
typedef BOOL(FnSetupComm)(void* com, DWORD dwInQueue, DWORD dwOutQueue);
typedef BOOL(FnPurgeComm)(void* com, DWORD dwFlags);
typedef BOOL(FnGetCommModemStatus)(void* com, LPDWORD lpModelStat);
typedef BOOL(FnWaitCommEvent)(void* com, LPDWORD lpEvtMask, LPOVERLAPPED lpOverlapped);
typedef BOOL(FnClearCommError)(void* com, LPDWORD lpErrors, LPCOMSTAT lpStat);

typedef struct com_hook {
    LPHANDLE virtual_handle;
    WCHAR wName[7];  // max is COM255
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

    void* data;

    struct com_hook* next;
} com_hook_t;

com_hook_t* new_com_hook(BYTE port);
void hook_com(com_hook_t* hook);
void hook_com();
