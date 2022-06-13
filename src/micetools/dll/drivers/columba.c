#include <Windows.h>

#include "../lib/dmi/dmi.h"
#include "../lib/mice/mice.h"
#include "files.h"

BOOL columba_DeviceIoControl(DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer,
                             DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    switch (dwIoControlCode) {
        case IOCTL_COLUMBA_READ_DMI:
            log_misc("columba",
                     "DeviceIoControl(<columba>, <read dmi>, 0x%p, 0x%x, -, "
                     "0x%x, -, -)",
                     lpInBuffer, nInBufferSize, nOutBufferSize);

            memset(lpOutBuffer, 0, nOutBufferSize);

            if (((LPWORD)lpInBuffer)[0] == 0x0000) {
                DMI_HEADER dmi = {
                    .Signature = {'_', 'D', 'M', 'I', '_'},
                    .Checksum = 0,
                    .StructLength = dmi_size,
                    .StructAddr = 0xdeeabeef,
                    .NumberOfStructs = 0x20,
                    .BCDRevision = 0,
                    .Reserved = 0,
                };
                dmi.Checksum = dmi_calc_checksum((char*)&dmi, 15);

                memcpy(lpOutBuffer, &dmi, sizeof(DMI_HEADER));
                if (lpBytesReturned) *lpBytesReturned = sizeof(DMI_HEADER);
            } else {
                memcpy(lpOutBuffer, dmi_table, dmi_size);
                if (lpBytesReturned) *lpBytesReturned = dmi_size;
            }

            break;
        default:
            log_warning("columba", "unhandled 0x%08x", dwIoControlCode);
            return FALSE;
    }

    return FALSE;
}

void setup_columba() {
    file_hook_t* columba = new_file_hook(L"\\\\.\\columba");
    columba->DeviceIoControl = &columba_DeviceIoControl;

    hook_file(columba);
}
