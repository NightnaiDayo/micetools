#include <Windows.h>

#include "../lib/mice/mice.h"
#include "mx.h"

BOOL mxjvs_DeviceIoControl(DWORD dwIoControlCode, LPVOID lpInBuffer,
                           DWORD nInBufferSize, LPVOID lpOutBuffer,
                           DWORD nOutBufferSize, LPDWORD lpBytesReturned,
                           LPOVERLAPPED lpOverlapped) {
    switch (dwIoControlCode) {
        case IOCTL_MXJVS_EXCHANGE:
            log_misc("mxjvs",
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

void setup_mxjvs() {
    file_hook_t* mxjvs = new_file_hook(L"\\\\.\\mxjvs");
    mxjvs->DeviceIoControl = &mxjvs_DeviceIoControl;

    hook_file(mxjvs);
}
