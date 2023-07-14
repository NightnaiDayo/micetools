#include "comdevice.h"

#define RESERVED_JVS_COM_PORT 4

com_device_t* GetComDevice(HANDLE hFile) {
    open_hook_t* pHData = GetDataForHandle(hFile);
    if (pHData == NULL || pHData->hook == NULL || pHData->hook->com_hook == NULL) return NULL;
    return pHData->hook->com_hook->com_device;
}

BOOL DevGetCommState(HANDLE hFile, LPDCB lpDCB) { return TRUE; }
BOOL DevSetCommState(HANDLE hFile, LPDCB lpDCB) { return TRUE; }
BOOL DevGetCommTimeouts(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts) { return TRUE; }
BOOL DevSetCommTimeouts(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts) { return TRUE; }
BOOL DevSetupComm(HANDLE hFile, DWORD dwInQueue, DWORD dwOutQueue) { return TRUE; }
BOOL DevPurgeComm(HANDLE hFile, DWORD dwFlags) {
    com_device_t* dev = GetComDevice(hFile);
    if (dev == NULL) return FALSE;
    if (dwFlags & PURGE_RXCLEAR) ringbuf_purge(&(dev->out));
    return TRUE;
}
BOOL DevGetCommModemStatus(HANDLE hFile, LPDWORD lpModemStatus) {
    if (!lpModemStatus) return FALSE;
    com_device_t* dev = GetComDevice(hFile);
    if (dev == NULL) return FALSE;
    *lpModemStatus = dev->modemStatus;
    return TRUE;
}
BOOL DevWaitCommEvent(HANDLE hFile, LPDWORD lpEvtMask, LPOVERLAPPED lpOverlapped) {
    com_device_t* dev = GetComDevice(hFile);
    if (dev == NULL) return FALSE;

    WaitForSingleObject(dev->dataOutReady, INFINITE);
    if (lpOverlapped != NULL) SetEvent(lpOverlapped->hEvent);
    return TRUE;
}
BOOL DevClearCommError(HANDLE hFile, LPDWORD lpErrors, LPCOMSTAT lpStat) {
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

        com_device_t* dev = GetComDevice(hFile);
        if (dev == NULL) return FALSE;

        lpStat->cbInQue = ringbuf_available(&dev->out);
        lpStat->cbOutQue = ringbuf_available(&dev->in);
    }
    return TRUE;
}

BOOL DevWriteFile(file_context_t* ctx, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                  LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    if (nNumberOfBytesToWrite > 0xffff) return FALSE;
    com_device_t* dev = GetComDevice(ctx->m_Handle);
    if (dev == NULL) return FALSE;

    // Ignore overflow
    ringbuf_write(&dev->in, lpBuffer, nNumberOfBytesToWrite & 0xffff);
    if (!SetEvent(dev->dataInReady))
        log_error(plfComm, "Failed to signal data in: %d", GetLastError());

    if (lpNumberOfBytesWritten) *lpNumberOfBytesWritten = nNumberOfBytesToWrite;
    return TRUE;
}
BOOL DevReadFile(file_context_t* ctx, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                 LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    if (nNumberOfBytesToRead > 0xffff) return FALSE;
    com_device_t* dev = GetComDevice(ctx->m_Handle);
    if (dev == NULL) return FALSE;

    // Make sure we have at least one byte to return
    // while (!ringbuf_available(&(dev->out))) {
    //     WaitForSingleObject(dev->event, INFINITE);
    // }

    short read = ringbuf_read(&dev->out, lpBuffer, nNumberOfBytesToRead & 0xffff);
    if (lpNumberOfBytesRead) *lpNumberOfBytesRead = read;

    if (lpOverlapped) {
        lpOverlapped->InternalHigh = read;
        SetEvent(lpOverlapped->hEvent);
    }
    return TRUE;
}

short comdev_read_blocking(com_device_t* com, unsigned char* buffer, short bytes) {
    while (comdev_available(com) < bytes) WaitForSingleObject(com->dataInReady, INFINITE);
    return ringbuf_read(&com->in, buffer, bytes);
}
short comdev_read(com_device_t* com, LPVOID buffer, short bytes) {
    return ringbuf_read(&com->in, buffer, bytes);
}
bool comdev_write(com_device_t* com, LPCVOID buffer, short bytes) {
    bool ret = ringbuf_write(&com->out, buffer, bytes);
    SetEvent(com->dataOutReady);
    return ret;
}
short comdev_available(com_device_t* com) { return ringbuf_available(&com->in); }
BYTE comdev_peek(com_device_t* com) { return com->in.buffer[com->in.read]; }

// Read data from a com device, unescaping as we go
void comio_read(com_device_t* com, LPVOID data, BYTE len) {
    LPBYTE lpbData = (LPBYTE)data;
    BYTE one_byte;
    for (; len; len--) {
        comdev_read_blocking(com, &one_byte, 1);
        if (one_byte == COMIO_MARK) {
            comdev_read_blocking(com, &one_byte, 1);
            one_byte++;
        }
        *(lpbData++) = one_byte;
    }
}
// Write data to a com device, escaping as we go
void comio_write(com_device_t* com, LPCVOID data, BYTE len) {
    LPBYTE lpbData = (LPBYTE)data;
    BYTE one_byte;
    for (; len; len--) {
        one_byte = *(lpbData++);
        if (one_byte == COMIO_MARK || one_byte == COMIO_SYNC) {
            BYTE mark = COMIO_MARK;
            comdev_write(com, &mark, 1);
            one_byte--;
        }
        comdev_write(com, &one_byte, 1);
    }
}
unsigned char comio_next_req(com_device_t* com, comio_recv_head_t* head, LPVOID data) {
    BYTE one_byte;
    do {
        comdev_read_blocking(com, &one_byte, 1);
        if (one_byte != COMIO_SYNC) {
            log_error(plfComm, "Garbage on JVS: %02x", one_byte);
            continue;
        }
        break;
    } while (1);

    comio_read(com, (LPBYTE)head, sizeof *head);

    // TODO: Validate the sum? Do we care really?
    comio_read(com, data, head->length);
    unsigned char sum;
    comio_read(com, &sum, 1);
    return sum;
}
void comio_reply(com_device_t* com, comio_recv_head_t* req, BYTE status, BYTE len, LPCVOID data) {
    BYTE one_byte;
    one_byte = COMIO_SYNC;
    comdev_write(com, &one_byte, 1);

    comio_resp_head_t resp = {
        .frame_length = len + sizeof resp,
        .src = req->dst,
        .seq = req->seq,
        .status = status,
        .op = req->op,
        .length = len,
    };
    // Header
    comio_write(com, (LPBYTE)&resp, sizeof resp);
    // Payload
    if (len)  // If len == 0, we allow data to be null
        comio_write(com, data, len);
    // Checksum
    one_byte = 0;
    for (BYTE i = 0; i < sizeof resp; i++) one_byte += ((LPBYTE)&resp)[i];
    for (BYTE i = 0; i < len; i++) one_byte += ((LPBYTE)data)[i];
    comio_write(com, &one_byte, 1);
}

BOOL attach_com_device(BYTE port, PVIRTUAL_SERIAL_DEVICE lpDevice) {
    if (port < 1 || port > NUM_COM_PORTS) {
        log_error(plfComm, "Requested COM%hhu but that is out of range!", port);
        return FALSE;
    }
    com_device_t* com = com_devices[port - 1];

    if (com->thread != NULL) {
        if (port == RESERVED_JVS_COM_PORT) {
            log_warning(plfComm, "Refusing to attach to reserved port COM%hhu", port);
            return FALSE;
        }

        // No need to change what's assigned!
        if (com->lpDevice == lpDevice) return TRUE;
        log_warning(plfComm, "COM%hhu is already attached!", port);
        TerminateThread(com->thread, (DWORD)-1);
    }

    com->thread = CreateThread(NULL, 0, lpDevice->m_Entrypoint, com, 0, NULL);
    if (com->thread == NULL) {
        log_error(plfComm, "Attach to COM%hhu failed: %d", port, GetLastError());
        return FALSE;
    }
    com->lpDevice = lpDevice;

    return TRUE;
}
void detach_com_device(BYTE port) {
    // If the port is out of range, there's guaranteeably nothing attached
    if (port < 1 || port > NUM_COM_PORTS) return;
    if (port == RESERVED_JVS_COM_PORT) return;

    com_device_t* com = com_devices[port - 1];
    if (!com->thread) return;

    TerminateThread(com->thread, (DWORD)-1);
    com->thread = NULL;
}
void detach_all_com_devices(void) {
    for (int i = 0; i < NUM_COM_PORTS; i++) {
        if (i == RESERVED_JVS_COM_PORT - 1) continue;

        if (com_devices[i]->thread != NULL) {
            TerminateThread(com_devices[i]->thread, (DWORD)-1);
            com_devices[i]->thread = NULL;
        }
    }
}

com_device_t* new_com_device(BYTE port) {
    com_device_t* com_device = (com_device_t*)malloc(sizeof *com_device);
    com_hook_t* com = new_com_hook(port);
    file_hook_t* file = new_file_hook(com->wName);
    file->com_hook = com;
    com->com_device = com_device;

    file->altFilename = com->wDosName;
    com_device->com = com;
    com_device->file = file;
    com_device->dataInReady = CreateEventA(NULL, FALSE, FALSE, NULL);
    com_device->dataOutReady = CreateEventW(NULL, TRUE, FALSE, NULL);

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

    ringbuf_purge(&com_device->in);
    ringbuf_purge(&com_device->out);
    com_device->thread = NULL;

    hook_file(file);

    return com_device;
}

void init_com_devices(void) {
    for (BYTE i = 1; i < NUM_COM_PORTS; i++) {
        com_devices[i] = new_com_device(i + 1);
    }
}
