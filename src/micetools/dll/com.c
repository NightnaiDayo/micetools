#include "com.h"

com_hook_t* com_hook_list = NULL;
com_hook_t* new_com_hook(BYTE port) {
    com_hook_t* hook = (com_hook_t*)malloc(sizeof *hook);

    swprintf(hook->wName, (sizeof hook->wName) / (sizeof hook->wName[0]), L"COM%d", port);
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
};
void hook_com(com_hook_t* hook) {
    hook->next = NULL;
    hook->virtual_handle = NULL;
    if (com_hook_list == NULL) {
        com_hook_list = hook;
        return;
    }

    com_hook_t* hl = com_hook_list;
    while (hl->next != NULL) hl = hl->next;
    hl->next = hook;
};

BOOL WINAPI FakeGetCommState(HANDLE hFile, LPDCB lpDCB) {
    log_misc("comm", "GetCommState(0x%p, 0x%p)", hFile, lpDCB);

    com_hook_t* hook = com_hook_list;
    while (hook != NULL) {
        if (hook->virtual_handle == hFile && hook->GetCommState != NULL) {
            return hook->GetCommState(hook->data, lpDCB);
        }
    }
    return TrueGetCommState(hFile, lpDCB);
}
BOOL WINAPI FakeSetCommState(HANDLE hFile, LPDCB lpDCB) {
    log_misc("comm", "SetCommState(0x%p, 0x%p)", hFile, lpDCB);

    com_hook_t* hook = com_hook_list;
    while (hook != NULL) {
        if (hook->virtual_handle == hFile && hook->SetCommState != NULL) {
            return hook->SetCommState(hook->data, lpDCB);
        }
    }
    return TrueSetCommState(hFile, lpDCB);
}
BOOL WINAPI FakeGetCommTimeouts(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts) {
    log_misc("comm", "GetCommTimeouts(0x%p, 0x%p)", hFile, lpCommTimeouts);
    com_hook_t* hook = com_hook_list;
    while (hook != NULL) {
        if (hook->virtual_handle == hFile && hook->GetCommTimeouts != NULL) {
            return hook->GetCommTimeouts(hook->data, lpCommTimeouts);
        }
    }
    return TrueGetCommTimeouts(hFile, lpCommTimeouts);
}
BOOL WINAPI FakeSetCommTimeouts(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts) {
    log_misc("comm", "SetCommTimeouts(0x%p, 0x%p)", hFile, lpCommTimeouts);
    com_hook_t* hook = com_hook_list;
    while (hook != NULL) {
        if (hook->virtual_handle == hFile && hook->SetCommTimeouts != NULL) {
            return hook->SetCommTimeouts(hook->data, lpCommTimeouts);
        }
    }
    return TrueSetCommTimeouts(hFile, lpCommTimeouts);
}
BOOL WINAPI FakeSetupComm(HANDLE hFile, DWORD dwInQueue, DWORD dwOutQueue) {
    log_misc("comm", "SetupCom(0x%p, 0x%08x, 0x%08x)", hFile, dwInQueue, dwOutQueue);
    com_hook_t* hook = com_hook_list;
    while (hook != NULL) {
        if (hook->virtual_handle == hFile && hook->SetupComm != NULL) {
            return hook->SetupComm(hook->data, dwInQueue, dwOutQueue);
        }
    }
    return TrueSetupComm(hFile, dwInQueue, dwOutQueue);
}
BOOL WINAPI FakePurgeComm(HANDLE hFile, DWORD dwFlags) {
    log_misc("comm", "PurgeComm(0x%p, 0x%08x)", hFile, dwFlags);
    com_hook_t* hook = com_hook_list;
    while (hook != NULL) {
        if (hook->virtual_handle == hFile && hook->PurgeComm != NULL) {
            return hook->PurgeComm(hook->data, dwFlags);
        }
    }
    return TruePurgeComm(hFile, dwFlags);
}
BOOL WINAPI FakeGetCommModemStatus(HANDLE hFile, LPDWORD lpModelStat) {
    log_misc("comm", "GetCommModemStatus(0x%p, 0x%p)", hFile, lpModelStat);

    com_hook_t* hook = com_hook_list;
    while (hook != NULL) {
        if (hook->virtual_handle == hFile && hook->GetCommModemStatus != NULL) {
            return hook->GetCommModemStatus(hook->data, lpModelStat);
        }
    }
    return TrueGetCommModemStatus(hFile, lpModelStat);
}
BOOL WINAPI FakeWaitCommEvent(HANDLE hFile, LPDWORD lpEvtMask, LPOVERLAPPED lpOverlapped) {
    log_misc("comm", "WaitCommEvent");

    com_hook_t* hook = com_hook_list;
    while (hook != NULL) {
        if (hook->virtual_handle == hFile && hook->WaitCommEvent != NULL) {
            return hook->WaitCommEvent(hook->data, lpEvtMask, lpOverlapped);
        }
    }
    return TrueWaitCommEvent(hFile, lpEvtMask, lpOverlapped);
}
BOOL WINAPI FakeClearCommError(HANDLE hFile, LPDWORD lpErrors, LPCOMSTAT lpStat) {
    log_misc("comm", "ClearCommError");

    com_hook_t* hook = com_hook_list;
    while (hook != NULL) {
        if (hook->virtual_handle == hFile && hook->ClearCommError != NULL) {
            return hook->ClearCommError(hook->data, lpErrors, lpStat);
        }
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
    hook("Kernel32.dll", "GetCommModemStatus", FakeGetCommModemStatus, (void**)&TrueGetCommModemStatus, 6);
    hook("Kernel32.dll", "WaitCommEvent", FakeWaitCommEvent, (void**)&TrueWaitCommEvent, 6);
    hook("Kernel32.dll", "ClearCommError", FakeClearCommError, (void**)&TrueClearCommError, 6);
}