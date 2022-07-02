#include "../lib/dmi/dmi.h"
#include "mx.h"

// Much easier than pulling in winddk.h
typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;
typedef struct {
    PHYSICAL_ADDRESS addr;
    DWORD data_type;
    DWORD bytes;
} columba_request;

#define DMI_HEADER_START 0x000f0000
#define DMI_TABLES_START 0x000f1000

BOOL columba_DeviceIoControl(void* file, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize,
                             LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned,
                             LPOVERLAPPED lpOverlapped) {
    switch (dwIoControlCode) {
        case IOCTL_COLUMBA_READ_DMI:
            log_misc("columba",
                     "DeviceIoControl(<columba>, <read dmi>, 0x%p, 0x%x, -, "
                     "0x%x, -, -)",
                     lpInBuffer, nInBufferSize, nOutBufferSize);
            columba_request* request = (columba_request*)lpInBuffer;
            log_info("columba", "Physical read: 0x%04x %ss at %08X", request->bytes,
                     request->data_type == 1   ? "byte"
                     : request->data_type == 2 ? "short"
                     : request->data_type == 4 ? "long"
                                               : "void",
                     request->addr);
            DWORD requested_size = request->data_type * request->bytes;

            memset(lpOutBuffer, 0, nOutBufferSize);

            if (request->addr.QuadPart == DMI_HEADER_START) {
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
            } else if (request->addr.QuadPart == DMI_TABLES_START) {
                memcpy(lpOutBuffer, dmi_table, dmi_size);
                if (lpBytesReturned) *lpBytesReturned = 0x10000;
            } else {
                log_error("columna", "Request to unmapped memory location: %08x", request->addr);
                return FALSE;
            }

            break;
        default:
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
