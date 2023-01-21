#include "comdevice.h"

com_device_t* GetComDevice(HANDLE hFile) {
    open_hook_t* pHData = GetDataForHandle(hFile, HDATA_FILE);
    return pHData->hook->com_hook->com_device;
}

BOOL DevGetCommState(HANDLE hFile, LPDCB lpDCB) { return TRUE; }
BOOL DevSetCommState(HANDLE hFile, LPDCB lpDCB) { return TRUE; }
BOOL DevGetCommTimeouts(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts) { return TRUE; }
BOOL DevSetCommTimeouts(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts) { return TRUE; }
BOOL DevSetupComm(HANDLE hFile, DWORD dwInQueue, DWORD dwOutQueue) { return TRUE; }
BOOL DevPurgeComm(HANDLE hFile, DWORD dwFlags) {
    if (dwFlags & PURGE_RXCLEAR) ringbuf_purge(&(GetComDevice(hFile)->out));
    return TRUE;
}
BOOL DevGetCommModemStatus(HANDLE hFile, LPDWORD lpModelStat) {
    // TODO: JVS SENSE
    return TRUE;
}
BOOL DevWaitCommEvent(HANDLE hFile, LPDWORD lpEvtMask, LPOVERLAPPED lpOverlapped) {
    WaitForSingleObject(GetComDevice(hFile)->event, INFINITE);
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
        lpStat->cbInQue = ringbuf_available(&(GetComDevice(hFile)->out));
        lpStat->cbOutQue = ringbuf_available(&(GetComDevice(hFile)->in));
    }
    return TRUE;
}

BOOL DevWriteFile(file_context_t* ctx, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                  LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    if (nNumberOfBytesToWrite > 0xffff) return FALSE;
    // Ignore overflow
    ringbuf_write(&(GetComDevice(ctx->m_Handle)->in), lpBuffer, nNumberOfBytesToWrite & 0xffff);
    if (lpNumberOfBytesWritten) *lpNumberOfBytesWritten = nNumberOfBytesToWrite;
    return TRUE;
}
BOOL DevReadFile(file_context_t* ctx, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                 LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    if (nNumberOfBytesToRead > 0xffff) return FALSE;
    com_device_t* comdev = GetComDevice(ctx->m_Handle);
    // Make sure we have at least one byte to return
    // while (!ringbuf_available(&(comdev->out))) {
    //     WaitForSingleObject(comdev->event, INFINITE);
    // }

    short read = ringbuf_read(&(comdev->out), lpBuffer, nNumberOfBytesToRead & 0xffff);
    if (lpNumberOfBytesRead) *lpNumberOfBytesRead = read;

    if (read != 0) {
        // log_info("drf", "%d", read);
        // for (int i = 0; i < read; i++) {
        //     printf("%02x ", ((LPBYTE)lpBuffer)[i]);
        // }
        // puts("");
    }
    return TRUE;
}

short comdev_read_blocking(com_device_t* com, unsigned char* buffer, short bytes) {
    while (comdev_available(com) < bytes) SwitchToThread();
    return ringbuf_read(&com->in, buffer, bytes);
}
short comdev_read(com_device_t* com, unsigned char* buffer, short bytes) {
    return ringbuf_read(&com->in, buffer, bytes);
}
bool comdev_write(com_device_t* com, const unsigned char* buffer, short bytes) {
    bool ret = ringbuf_write(&com->out, buffer, bytes);
    SetEvent(com->event);
    return ret;
}
short comdev_available(com_device_t* com) { return ringbuf_available(&com->in); }
BYTE comdev_peek(com_device_t* com) { return com->in.buffer[com->in.read]; }

// Read data from a com device, unescaping as we go
void comio_read(com_device_t* com, BYTE* data, BYTE len) {
    BYTE one_byte;
    for (; len; len--) {
        comdev_read_blocking(com, &one_byte, 1);
        if (one_byte == COMIO_MARK) {
            comdev_read_blocking(com, &one_byte, 1);
            one_byte++;
        }
        *(data++) = one_byte;
    }
}
// Write data to a com device, escaping as we go
void comio_write(com_device_t* com, BYTE* data, BYTE len) {
    BYTE one_byte;
    for (; len; len--) {
        one_byte = *(data++);
        if (one_byte == COMIO_MARK || one_byte == COMIO_SYNC) {
            BYTE mark = COMIO_MARK;
            comdev_write(com, &mark, 1);
            one_byte--;
        }
        comdev_write(com, &one_byte, 1);
    }
}
void comio_next_req(com_device_t* com, comio_recv_head_t* head, BYTE* data) {
    BYTE one_byte;
    do {
        if (comdev_available(com) < (sizeof *head + 1)) {
            SwitchToThread();
            continue;
        }
        comdev_read(com, &one_byte, 1);
        if (one_byte != COMIO_SYNC) {
            log_error("com", "Garbage on JVS: %02x", one_byte);
            continue;
        }
        break;
    } while (1);

    comio_read(com, (LPBYTE)head, sizeof *head);

    // TODO: Validate the sum? Do we care really?
    comio_read(com, data, head->length);
    unsigned char sum;
    comio_read(com, &sum, 1);
}
void comio_reply(com_device_t* com, comio_recv_head_t* req, BYTE status, BYTE len, BYTE* data) {
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
    for (BYTE i = 0; i < len; i++) one_byte += data[i];
    comio_write(com, &one_byte, 1);
}

void com_device_thread(com_device_t* com, FnComDeviceThread* thread) {
    com->thread = CreateThread(NULL, 0, thread, com, 0, NULL);
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
    com_device->event = CreateEventW(NULL, TRUE, FALSE, com_device->com->wName);

    hook_file(file);

    return com_device;
}
