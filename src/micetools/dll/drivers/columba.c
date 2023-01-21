#include "../lib/am/amOemstring.h"
#include "../lib/dmi/dmi.h"
#include "mx.h"

#define DMI_HEADER_START 0x000f0000
#define DMI_TABLES_START 0x000f1000

BOOL WINAPI columba_DeviceIoControl(file_context_t* ctx, DWORD dwIoControlCode, LPVOID lpInBuffer,
                                    DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                                    LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    switch (dwIoControlCode) {
        case IOCTL_COLUMBA_READ:
            log_misc("columba", "DeviceIoControl(<columba>, <read>, 0x%p, 0x%x, -, 0x%x, -, -)",
                     lpInBuffer, nInBufferSize, nOutBufferSize);
            AM_COLUMBA_REQUEST* request = (AM_COLUMBA_REQUEST*)lpInBuffer;
            log_info("columba", "Physical read: 0x%04x %ss at %08X", request->m_elementCount,
                     request->m_elementSize == 1   ? "byte"
                     : request->m_elementSize == 2 ? "short"
                     : request->m_elementSize == 4 ? "long"
                                                   : "void",
                     request->m_physAddr.QuadPart);
            DWORD requested_size = request->m_elementSize * request->m_elementCount;

            memset(lpOutBuffer, 0, nOutBufferSize);

            if (request->m_physAddr.QuadPart == DMI_HEADER_START) {
                DMI_HEADER dmi = {
                    .Signature = { '_', 'D', 'M', 'I', '_' },
                    .Checksum = 0,
                    .StructLength = dmi_size,
                    .StructAddr = DMI_TABLES_START,
                    .NumberOfStructs = 0x20,
                    .BCDRevision = 0,
                    .Reserved = 0,
                };
                dmi.Checksum = dmi_calc_checksum((char*)&dmi, 15);

                memcpy(lpOutBuffer, &dmi, sizeof(DMI_HEADER));
                if (lpBytesReturned) *lpBytesReturned = requested_size;
            } else if (request->m_physAddr.QuadPart == DMI_TABLES_START) {
                memcpy(lpOutBuffer, dmi_table, dmi_size);
                if (lpBytesReturned) *lpBytesReturned = 0x10000;
            } else {
                log_error("columba", "Request to unmapped memory location: %08x",
                          request->m_physAddr);
                return FALSE;
            }

            break;
        default:
            // Observed: IOCTL_KSEC_RNG_REKEY
            log_warning("columba", "unhandled 0x%08x", dwIoControlCode);
            return FALSE;
    }

    return TRUE;
}

void setup_columba() {
    dmi_build_default();

    file_hook_t* columba = new_file_hook(L"\\\\.\\columba");
    columba->DeviceIoControl = &columba_DeviceIoControl;

    hook_file(columba);
}
