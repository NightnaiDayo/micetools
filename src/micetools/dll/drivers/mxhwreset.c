#include <Windows.h>

#include "../lib/mice/mice.h"
#include "mx.h"

BOOL mxhwreset_DeviceIoControl(DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer,
                               DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    switch (dwIoControlCode) {
        case IOCTL_MXHWRESET_RESET:
            if (lpBytesReturned) *lpBytesReturned = 0;
            break;
        default:
            log_warning("mxhwreset", "unhandled 0x%08x", dwIoControlCode);
            return FALSE;
    }
    return TRUE;
}

void setup_mxhwreset() {
    file_hook_t* mxhwreset = new_file_hook(L"\\\\.\\mxhwreset");
    mxhwreset->DeviceIoControl = &mxhwreset_DeviceIoControl;

    hook_file(mxhwreset);
}
