#include "comdevice.h"

BOOL DevGetCommState(void* data, LPDCB lpDCB) { return TRUE; }
BOOL DevSetCommState(void* data, LPDCB lpDCB) { return TRUE; }
BOOL DevGetCommTimeouts(void* data, LPCOMMTIMEOUTS lpCommTimeouts) { return TRUE; }
BOOL DevSetCommTimeouts(void* data, LPCOMMTIMEOUTS lpCommTimeouts) { return TRUE; }
BOOL DevSetupComm(void* data, DWORD dwInQueue, DWORD dwOutQueue) { return TRUE; }
BOOL DevPurgeComm(void* data, DWORD dwFlags) {
    if (dwFlags & PURGE_RXCLEAR) ((com_device_t*)data)->filled = 0;
    return TRUE;
}
BOOL DevGetCommModemStatus(void* data, LPDWORD lpModelStat) {
    // TODO: JVS SENSE
    return TRUE;
}
BOOL DevWaitCommEvent(void* data, LPDWORD lpEvtMask, LPOVERLAPPED lpOverlapped) {
    WaitForSingleObject(((com_device_t*)data)->event, INFINITE);
    if (lpOverlapped != NULL) SetEvent(lpOverlapped->hEvent);
    return TRUE;
}
BOOL DevClearCommError(void* data, LPDWORD lpErrors, LPCOMSTAT lpStat) { return TRUE; }

com_device_t* new_com_device(BYTE port) {
    com_device_t* hook = (com_device_t*)malloc(sizeof *hook);
    com_hook_t* com = new_com_hook(port);
    file_hook_t* file = new_file_hook(com->wName);
    com->data = hook;

    com->GetCommState = DevGetCommState;
    com->SetCommState = DevSetCommState;
    com->GetCommTimeouts = DevGetCommTimeouts;
    com->SetCommTimeouts = DevSetCommTimeouts;
    com->SetupComm = DevSetupComm;
    com->PurgeComm = DevPurgeComm;
    com->GetCommModemStatus = DevGetCommModemStatus;
    com->WaitCommEvent = DevWaitCommEvent;
    com->ClearCommError = DevClearCommError;

    hook->filled = 0;
    hook->event = CreateEventW(NULL, TRUE, FALSE, hook->com->wName);
    hook->com = com;

    return hook;
}
