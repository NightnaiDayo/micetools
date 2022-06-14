#include <Windows.h>

#include "../lib/mice/mice.h"
#include "mx.h"

BOOL JVS_SENSE = false;

BOOL mxjvs_DeviceIoControl(DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer,
                           DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    switch (dwIoControlCode) {
        case IOCTL_MXJVS_EXCHANGE:
            log_error("mxjvs",
                      "DeviceIoControl(<mxjvs>, <exchange>, 0x%p, 0x%x, -, "
                      "0x%x, -, -)",
                      lpInBuffer, nInBufferSize, nOutBufferSize);

            // mxjvsDevice->exchange(
            // 	lpbyte(lpInBuffer), nInBufferSize & 0xFFFF,
            // 	lpbyte(lpOutBuffer), lpBytesReturned
            // );

            if (lpBytesReturned) *lpBytesReturned = 0;
            break;
        default:
            log_warning("mxjvs", "unhandled 0x%08x", dwIoControlCode);
            return FALSE;
    }

    return TRUE;
}

BOOL mxjvs_SetupComm(void* com, DWORD dwInQueue, DWORD dwOutQueue) { return TRUE; }
BOOL mxjvs_PurgeComm(void* com, DWORD dwFlags) { return TRUE; }
BOOL mxjvs_GetCommState(void* com, LPDCB lpDCB) { return TRUE; }
BOOL mxjvs_SetCommTimeouts(void* com, LPCOMMTIMEOUTS lpDCB) { return TRUE; }

BOOL mxjvs_GetCommModemStatus(void* com, LPDWORD lpModelStat) {
    *lpModelStat = JVS_SENSE ? MS_DSR_ON : 0;
    return TRUE;
}

BOOL mxjvs_SetCommState(void* com, LPDCB lpDCB) {
    char PARITY[] = {'N', 'O', 'E', 'M', 'S'};
    char* STOP[] = {"1", "1.5", "2"};
    log_info("mxjvs", "Switching to %d baud (%d%c%s)", lpDCB->BaudRate, lpDCB->ByteSize, PARITY[lpDCB->Parity],
             STOP[lpDCB->StopBits]);
    return TRUE;
}

void setup_mxjvs() {
    file_hook_t* mxjvs = new_file_hook(L"\\\\.\\mxjvs");
    mxjvs->DeviceIoControl = &mxjvs_DeviceIoControl;

    com_hook_t* jvscom = new_com_hook(-1);
    jvscom->GetCommState = mxjvs_GetCommState;
    jvscom->SetCommState = mxjvs_SetCommState;
    jvscom->SetCommTimeouts = mxjvs_SetCommTimeouts;
    jvscom->SetupComm = mxjvs_SetupComm;
    jvscom->PurgeComm = mxjvs_PurgeComm;
    jvscom->GetCommModemStatus = mxjvs_GetCommModemStatus;

    hook_file(mxjvs);
    hook_com(jvscom);
    free(jvscom->virtual_handle);
    jvscom->virtual_handle = mxjvs->virtual_handle;
}
