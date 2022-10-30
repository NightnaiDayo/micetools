#include "com.h"

#include "files.h"

com_hook_t* com_hook_list = NULL;
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

void hook_com(com_hook_t* hook) {
    hook->next = NULL;
    hook->virtual_handle = (LPHANDLE)malloc(sizeof(HANDLE));
    *hook->virtual_handle = NULL;
    if (com_hook_list == NULL) {
        com_hook_list = hook;
        return;
    }

    com_hook_t* hl = com_hook_list;
    while (hl->next != NULL) hl = hl->next;
    hl->next = hook;
}

BOOL WINAPI FakeGetCommState(HANDLE hFile, LPDCB lpDCB) {
    com_hook_t* hook = get_handle_com_hook(hFile);
    log_misc(COMM_LOGGER, "GetCommState(0x%p, 0x%p) (%08x)", hFile, lpDCB, hook);

    if (hook != NULL) {
        if (hook->GetCommState == NULL) {
            log_error(COMM_LOGGER, "GetCommState(%ls) unimplemented", hook->wName);
            return FALSE;
        }
        return hook->GetCommState(hook->data, lpDCB);
    }
    return TrueGetCommState(hFile, lpDCB);
}

BOOL WINAPI FakeSetCommState(HANDLE hFile, LPDCB lpDCB) {
    log_misc(COMM_LOGGER, "SetCommState(0x%p, 0x%p)", hFile, lpDCB);

    com_hook_t* hook = get_handle_com_hook(hFile);
    if (hook != NULL) {
        if (hook->SetCommState == NULL) {
            log_error(COMM_LOGGER, "SetCommState(%ls) unimplemented", hook->wName);
            return FALSE;
        }
        return hook->SetCommState(hook->data, lpDCB);
    }

    return TrueSetCommState(hFile, lpDCB);
}

BOOL WINAPI FakeGetCommTimeouts(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts) {
    log_misc(COMM_LOGGER, "GetCommTimeouts(0x%p, 0x%p)", hFile, lpCommTimeouts);

    com_hook_t* hook = get_handle_com_hook(hFile);
    if (hook != NULL) {
        if (hook->GetCommTimeouts == NULL) {
            log_error(COMM_LOGGER, "GetCommTimeouts(%ls) unimplemented", hook->wName);
            return FALSE;
        }
        return hook->GetCommTimeouts(hook->data, lpCommTimeouts);
    }
    return TrueGetCommTimeouts(hFile, lpCommTimeouts);
}

BOOL WINAPI FakeSetCommTimeouts(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts) {
    log_misc(COMM_LOGGER, "SetCommTimeouts(0x%p, 0x%p)", hFile, lpCommTimeouts);

    com_hook_t* hook = get_handle_com_hook(hFile);
    if (hook != NULL) {
        if (hook->SetCommTimeouts == NULL) {
            log_error(COMM_LOGGER, "SetCommTimeouts(%ls) unimplemented", hook->wName);
            return FALSE;
        }
        return hook->SetCommTimeouts(hook->data, lpCommTimeouts);
    }
    return TrueSetCommTimeouts(hFile, lpCommTimeouts);
}

BOOL WINAPI FakeSetupComm(HANDLE hFile, DWORD dwInQueue, DWORD dwOutQueue) {
    log_misc(COMM_LOGGER, "SetupCom(0x%p, 0x%08x, 0x%08x)", hFile, dwInQueue, dwOutQueue);

    com_hook_t* hook = get_handle_com_hook(hFile);
    if (hook != NULL) {
        if (hook->SetupComm == NULL) {
            log_error(COMM_LOGGER, "SetupComm(%ls) unimplemented", hook->wName);
            return FALSE;
        }
        return hook->SetupComm(hook->data, dwInQueue, dwOutQueue);
    }
    return TrueSetupComm(hFile, dwInQueue, dwOutQueue);
}

BOOL WINAPI FakePurgeComm(HANDLE hFile, DWORD dwFlags) {
    log_misc(COMM_LOGGER, "PurgeComm(0x%p, 0x%08x)", hFile, dwFlags);

    com_hook_t* hook = get_handle_com_hook(hFile);
    if (hook != NULL) {
        if (hook->PurgeComm == NULL) {
            log_error(COMM_LOGGER, "PurgeComm(%ls) unimplemented", hook->wName);
            return FALSE;
        }
        return hook->PurgeComm(hook->data, dwFlags);
    }
    return TruePurgeComm(hFile, dwFlags);
}

BOOL WINAPI FakeGetCommModemStatus(HANDLE hFile, LPDWORD lpModelStat) {
    log_misc(COMM_LOGGER, "GetCommModemStatus(0x%p, 0x%p)", hFile, lpModelStat);

    com_hook_t* hook = get_handle_com_hook(hFile);
    if (hook != NULL) {
        if (hook->GetCommModemStatus == NULL) {
            log_error(COMM_LOGGER, "GetCommModemStatus(%ls) unimplemented", hook->wName);
            return FALSE;
        }
        return hook->GetCommModemStatus(hook->data, lpModelStat);
    }
    return TrueGetCommModemStatus(hFile, lpModelStat);
}

BOOL WINAPI FakeWaitCommEvent(HANDLE hFile, LPDWORD lpEvtMask, LPOVERLAPPED lpOverlapped) {
    log_misc(COMM_LOGGER, "WaitCommEvent(0x%p)", hFile);

    com_hook_t* hook = get_handle_com_hook(hFile);
    if (hook != NULL) {
        if (hook->WaitCommEvent == NULL) {
            log_error(COMM_LOGGER, "WaitCommEvent(%ls) unimplemented", hook->wName);
            return FALSE;
        }
        return hook->WaitCommEvent(hook->data, lpEvtMask, lpOverlapped);
    }
    return TrueWaitCommEvent(hFile, lpEvtMask, lpOverlapped);
}

BOOL WINAPI FakeClearCommError(HANDLE hFile, LPDWORD lpErrors, LPCOMSTAT lpStat) {
    log_trace(COMM_LOGGER, "ClearCommError(0x%p)", hFile);

    com_hook_t* hook = get_handle_com_hook(hFile);
    if (hook != NULL) {
        if (hook->ClearCommError == NULL) {
            log_error(COMM_LOGGER, "ClearCommError(%ls) unimplemented", hook->wName);
            return FALSE;
        }
        return hook->ClearCommError(hook->data, lpErrors, lpStat);
    }
    return TrueClearCommError(hFile, lpErrors, lpStat);
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