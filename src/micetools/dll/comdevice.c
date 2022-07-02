#include "comdevice.h"

BOOL DevGetCommState(void* data, LPDCB lpDCB) { return TRUE; }
BOOL DevSetCommState(void* data, LPDCB lpDCB) { return TRUE; }
BOOL DevGetCommTimeouts(void* data, LPCOMMTIMEOUTS lpCommTimeouts) { return TRUE; }
BOOL DevSetCommTimeouts(void* data, LPCOMMTIMEOUTS lpCommTimeouts) { return TRUE; }
BOOL DevSetupComm(void* data, DWORD dwInQueue, DWORD dwOutQueue) { return TRUE; }
BOOL DevPurgeComm(void* data, DWORD dwFlags) {
    if (dwFlags & PURGE_RXCLEAR) ringbuf_purge(&((com_device_t*)data)->out);

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
BOOL DevClearCommError(void* data, LPDWORD lpErrors, LPCOMSTAT lpStat) {
    if (lpErrors != NULL) *lpErrors = 0;
    if (lpStat != NULL) {
        lpStat->fCtsHold = FALSE;
        lpStat->fDsrHold = FALSE;
        lpStat->fRlsdHold = FALSE;
        lpStat->fXoffHold = FALSE;
        lpStat->fXoffSent = FALSE;
        lpStat->fEof = FALSE;
        lpStat->fTxim = FALSE;
        lpStat->fReserved = 0;
        lpStat->cbInQue = ringbuf_available(&((com_device_t*)data)->out);
        lpStat->cbOutQue = ringbuf_available(&((com_device_t*)data)->in);
    }
    return TRUE;
}

BOOL DevWriteFile(void* file, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten,
                  LPOVERLAPPED lpOverlapped) {
    if (nNumberOfBytesToWrite > 0xffff) return FALSE;
    // Ignore overflow
    ringbuf_write(&((com_device_t*)file)->in, lpBuffer, nNumberOfBytesToWrite & 0xffff);
    if (lpNumberOfBytesWritten) *lpNumberOfBytesWritten = nNumberOfBytesToWrite;
    return TRUE;
}
BOOL DevReadFile(void* file, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead,
                 LPOVERLAPPED lpOverlapped) {
    if (nNumberOfBytesToRead > 0xffff) return FALSE;

    // Make sure we have at least one byte to return
    // while (!ringbuf_available(&((com_device_t*)file)->out)) {
    //     WaitForSingleObject(((com_device_t*)file)->event, INFINITE);
    // }

    short read = ringbuf_read(&((com_device_t*)file)->out, lpBuffer, nNumberOfBytesToRead & 0xffff);
    if (lpNumberOfBytesRead) *lpNumberOfBytesRead = read;

    if (read != 0) {
        // log_info("drf", "%d", read);
        // for (int i = 0; i < read; i++) {
        //     printf("%02x ", ((unsigned char*)lpBuffer)[i]);
        // }
        // puts("");
    }
    return TRUE;
}

short comdev_read(com_device_t* com, unsigned char* buffer, short bytes) {
    return ringbuf_read(&com->in, buffer, bytes);
}
bool comdev_write(com_device_t* com, unsigned char* buffer, short bytes) {
    bool ret = ringbuf_write(&com->out, buffer, bytes);
    SetEvent(com->event);
    return ret;
}
short comdev_available(com_device_t* com) { return ringbuf_available(&com->in); }

void com_device_thread(com_device_t* com, FnComDeviceThread* thread) {
    com->thread = CreateThread(NULL, 0, thread, com, 0, NULL);
}

com_device_t* new_com_device(BYTE port) {
    com_device_t* hook = (com_device_t*)malloc(sizeof *hook);
    com_hook_t* com = new_com_hook(port);
    file_hook_t* file = new_file_hook(com->wName);
    file->altFilename = com->wDosName;
    com->data = hook;
    file->data = hook;
    hook->com = com;
    hook->file = file;

    com->GetCommState = DevGetCommState;
    com->SetCommState = DevSetCommState;
    com->GetCommTimeouts = DevGetCommTimeouts;
    com->SetCommTimeouts = DevSetCommTimeouts;
    com->SetupComm = DevSetupComm;
    com->PurgeComm = DevPurgeComm;
    com->GetCommModemStatus = DevGetCommModemStatus;
    com->WaitCommEvent = DevWaitCommEvent;
    com->ClearCommError = DevClearCommError;

    file->ReadFile = DevReadFile;
    file->WriteFile = DevWriteFile;

    ringbuf_purge(&hook->in);
    ringbuf_purge(&hook->out);
    hook->event = CreateEventW(NULL, TRUE, FALSE, hook->com->wName);

    hook_file(file);
    hook_com(com);
    free(com->virtual_handle);
    com->virtual_handle = file->virtual_handle;

    return hook;
}
