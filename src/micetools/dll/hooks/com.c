#define _MICE_COM
#include "com.h"

#include "files.h"

com_hook_t* new_com_hook(BYTE port) {
    com_hook_t* hook = (com_hook_t*)malloc(sizeof *hook);

    memset(hook->wName, 0, sizeof hook->wName);
    swprintf(hook->wName, (sizeof hook->wName) / (sizeof hook->wName[0]), L"COM%d", port);
    memset(hook->wDosName, 0, sizeof hook->wDosName);
    swprintf(hook->wDosName, (sizeof hook->wDosName) / (sizeof hook->wDosName[0]), L"\\\\.\\COM%d",
             port);
    hook->com = port;

    hook->GetCommState = NULL;
    hook->SetCommState = NULL;
    hook->GetCommTimeouts = NULL;
    hook->SetCommTimeouts = NULL;
    hook->SetupComm = NULL;
    hook->PurgeComm = NULL;
    hook->GetCommModemStatus = NULL;
    hook->WaitCommEvent = NULL;
    hook->ClearCommError = NULL;

    return hook;
}

BOOL WINAPI FakeGetCommState(HANDLE hFile, LPDCB lpDCB) {
    open_hook_t* pHData = GetDataForHandle(hFile, HDATA_FILE);
    log_misc(COMM_LOGGER, "GetCommState(0x%p, 0x%p) (%08x)", hFile, lpDCB, hook);

    if (pHData == NULL || pHData->hook->com_hook == NULL) return TrueGetCommState(hFile, lpDCB);
    com_hook_t* com_hook = pHData->hook->com_hook;

    if (com_hook->GetCommState == NULL) {
        log_error(COMM_LOGGER, "GetCommState(%ls) unimplemented", com_hook->wName);
        return FALSE;
    }
    return com_hook->GetCommState(hFile, lpDCB);
}

BOOL WINAPI FakeSetCommState(HANDLE hFile, LPDCB lpDCB) {
    log_misc(COMM_LOGGER, "SetCommState(0x%p, 0x%p)", hFile, lpDCB);

    open_hook_t* pHData = GetDataForHandle(hFile, HDATA_FILE);
    if (pHData == NULL || pHData->hook->com_hook == NULL) return TrueSetCommState(hFile, lpDCB);

    com_hook_t* com_hook = pHData->hook->com_hook;
    if (com_hook->SetCommState == NULL) {
        log_error(COMM_LOGGER, "SetCommState(%ls) unimplemented", com_hook->wName);
        return FALSE;
    }
    return com_hook->SetCommState(hFile, lpDCB);
}

BOOL WINAPI FakeGetCommTimeouts(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts) {
    log_misc(COMM_LOGGER, "GetCommTimeouts(0x%p, 0x%p)", hFile, lpCommTimeouts);

    open_hook_t* pHData = GetDataForHandle(hFile, HDATA_FILE);
    if (pHData == NULL || pHData->hook->com_hook == NULL)
        return TrueGetCommTimeouts(hFile, lpCommTimeouts);

    com_hook_t* com_hook = pHData->hook->com_hook;
    if (com_hook->GetCommTimeouts == NULL) {
        log_error(COMM_LOGGER, "GetCommTimeouts(%ls) unimplemented", com_hook->wName);
        return FALSE;
    }
    return com_hook->GetCommTimeouts(hFile, lpCommTimeouts);
}

BOOL WINAPI FakeSetCommTimeouts(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts) {
    log_misc(COMM_LOGGER, "SetCommTimeouts(0x%p, 0x%p)", hFile, lpCommTimeouts);

    open_hook_t* pHData = GetDataForHandle(hFile, HDATA_FILE);
    if (pHData == NULL || pHData->hook->com_hook == NULL)
        return TrueSetCommTimeouts(hFile, lpCommTimeouts);

    com_hook_t* com_hook = pHData->hook->com_hook;
    if (com_hook->SetCommTimeouts == NULL) {
        log_error(COMM_LOGGER, "SetCommTimeouts(%ls) unimplemented", com_hook->wName);
        return FALSE;
    }
    return com_hook->SetCommTimeouts(hFile, lpCommTimeouts);
}

BOOL WINAPI FakeSetupComm(HANDLE hFile, DWORD dwInQueue, DWORD dwOutQueue) {
    log_misc(COMM_LOGGER, "SetupCom(0x%p, 0x%08x, 0x%08x)", hFile, dwInQueue, dwOutQueue);

    open_hook_t* pHData = GetDataForHandle(hFile, HDATA_FILE);
    if (pHData == NULL || pHData->hook->com_hook == NULL)
        return TrueSetupComm(hFile, dwInQueue, dwOutQueue);

    com_hook_t* com_hook = pHData->hook->com_hook;
    if (com_hook->SetupComm == NULL) {
        log_error(COMM_LOGGER, "SetupComm(%ls) unimplemented", com_hook->wName);
        return FALSE;
    }
    return com_hook->SetupComm(hFile, dwInQueue, dwOutQueue);
}

BOOL WINAPI FakePurgeComm(HANDLE hFile, DWORD dwFlags) {
    log_misc(COMM_LOGGER, "PurgeComm(0x%p, 0x%08x)", hFile, dwFlags);

    open_hook_t* pHData = GetDataForHandle(hFile, HDATA_FILE);
    if (pHData == NULL || pHData->hook->com_hook == NULL) return TruePurgeComm(hFile, dwFlags);

    com_hook_t* com_hook = pHData->hook->com_hook;
    if (com_hook->PurgeComm == NULL) {
        log_error(COMM_LOGGER, "PurgeComm(%ls) unimplemented", com_hook->wName);
        return FALSE;
    }
    return com_hook->PurgeComm(hFile, dwFlags);
}

BOOL WINAPI FakeGetCommModemStatus(HANDLE hFile, LPDWORD lpModelStat) {
    log_misc(COMM_LOGGER, "GetCommModemStatus(0x%p, 0x%p)", hFile, lpModelStat);

    open_hook_t* pHData = GetDataForHandle(hFile, HDATA_FILE);
    if (pHData == NULL || pHData->hook->com_hook == NULL)
        return TrueGetCommModemStatus(hFile, lpModelStat);

    com_hook_t* com_hook = pHData->hook->com_hook;
    if (com_hook->GetCommModemStatus == NULL) {
        log_error(COMM_LOGGER, "GetCommModemStatus(%ls) unimplemented", com_hook->wName);
        return FALSE;
    }
    return com_hook->GetCommModemStatus(hFile, lpModelStat);
}

BOOL WINAPI FakeWaitCommEvent(HANDLE hFile, LPDWORD lpEvtMask, LPOVERLAPPED lpOverlapped) {
    log_misc(COMM_LOGGER, "WaitCommEvent(0x%p)", hFile);

    open_hook_t* pHData = GetDataForHandle(hFile, HDATA_FILE);
    if (pHData == NULL || pHData->hook->com_hook == NULL)
        return TrueWaitCommEvent(hFile, lpEvtMask, lpOverlapped);

    com_hook_t* com_hook = pHData->hook->com_hook;
    if (com_hook->WaitCommEvent == NULL) {
        log_error(COMM_LOGGER, "WaitCommEvent(%ls) unimplemented", com_hook->wName);
        return FALSE;
    }
    return com_hook->WaitCommEvent(hFile, lpEvtMask, lpOverlapped);
}

BOOL WINAPI FakeClearCommError(HANDLE hFile, LPDWORD lpErrors, LPCOMSTAT lpStat) {
    log_trace(COMM_LOGGER, "ClearCommError(0x%p)", hFile);

    open_hook_t* pHData = GetDataForHandle(hFile, HDATA_FILE);
    if (pHData == NULL || pHData->hook->com_hook == NULL)
        return TrueClearCommError(hFile, lpErrors, lpStat);

    com_hook_t* com_hook = pHData->hook->com_hook;
    if (com_hook->ClearCommError == NULL) {
        log_error(COMM_LOGGER, "ClearCommError(%ls) unimplemented", com_hook->wName);
        return FALSE;
    }
    return com_hook->ClearCommError(hFile, lpErrors, lpStat);
}

void hook_commio() {
    hook("Kernel32.dll", "GetCommState", FakeGetCommState, (void**)&TrueGetCommState, 6);
    hook("Kernel32.dll", "SetCommState", FakeSetCommState, (void**)&TrueSetCommState, 6);
    hook("Kernel32.dll", "GetCommTimeouts", FakeGetCommTimeouts, (void**)&TrueGetCommTimeouts, 6);
    hook("Kernel32.dll", "SetCommTimeouts", FakeSetCommTimeouts, (void**)&TrueSetCommTimeouts, 6);
    hook("Kernel32.dll", "SetupComm", FakeSetupComm, (void**)&TrueSetupComm, 6);
    hook("Kernel32.dll", "PurgeComm", FakePurgeComm, (void**)&TruePurgeComm, 6);
    hook("Kernel32.dll", "GetCommModemStatus", FakeGetCommModemStatus,
         (void**)&TrueGetCommModemStatus, 6);
    hook("Kernel32.dll", "WaitCommEvent", FakeWaitCommEvent, (void**)&TrueWaitCommEvent, 6);
    hook("Kernel32.dll", "ClearCommError", FakeClearCommError, (void**)&TrueClearCommError, 6);
}