#include "mx.h"

#define SRAM_DUMP L"dev/sram.bin"
#define SRAM_SIZE 1024 * 2084
LPBYTE SRAM;
DWORD SRAM_POINTER = 0;

void sram_dump() {
    HANDLE dump = _CreateFileW(SRAM_DUMP, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
    if (dump == INVALID_HANDLE_VALUE) {
        log_error("sram", "CreateFileA(SRAM_DUMP) failed");
        return;
    }
    _WriteFile(dump, SRAM, SRAM_SIZE, NULL, NULL);
    _CloseHandle(dump);
}

void sram_restore() {
    HANDLE dump =
        _CreateFileW(SRAM_DUMP, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (dump == INVALID_HANDLE_VALUE) return;
    DWORD read;
    if (!_ReadFile(dump, SRAM, SRAM_SIZE, &read, NULL)) log_error("sram", "failed to restore (%d)", GetLastError());
    _CloseHandle(dump);
}

BOOL mxsram_DeviceIoControl(void* file, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize,
                            LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned,
                            LPOVERLAPPED lpOverlapped) {
    DWORD SRAM_VERSION = 0x0001;
    DWORD SRAM_SECTOR_SIZE = 0x100;  // Max is 0x800

    switch (dwIoControlCode) {
        case IOCTL_MXSRAM_PING:  // Get version
            log_info("mxsram",
                     "DeviceIoControl(<mxsram>, <ping>, 0x%p, 0x%x, -, 0x%x, "
                     "-, -)",
                     lpInBuffer, nInBufferSize, nOutBufferSize);

            ((LPDWORD)lpOutBuffer)[0] = SRAM_VERSION;
            if (lpBytesReturned) *lpBytesReturned = 4;
            break;
        case IOCTL_DISK_GET_DRIVE_GEOMETRY:
            log_info("mxsram",
                     "DeviceIoControl(<mxsram>, <get drive geom>, 0x%p, "
                     "0x%x, -, 0x%x, -, -)",
                     lpInBuffer, nInBufferSize, nOutBufferSize);

            DISK_GEOMETRY out = *(PDISK_GEOMETRY)lpOutBuffer;
            memset(&out, 0, sizeof(out));
            out.Cylinders.QuadPart = 1;
            out.MediaType = FixedMedia;
            out.TracksPerCylinder = 224;
            out.SectorsPerTrack = 32;
            out.BytesPerSector = 1;
            if (lpBytesReturned) *lpBytesReturned = sizeof(DISK_GEOMETRY);
            break;
        case IOCTL_MXSRAM_GET_SECTOR_SIZE:
            log_info("mxsram",
                     "DeviceIoControl(<mxsram>, <get sector size>, 0x%p, "
                     "0x%x, -, 0x%x, -, -)",
                     lpInBuffer, nInBufferSize, nOutBufferSize);

            ((LPDWORD)lpOutBuffer)[0] = SRAM_SECTOR_SIZE;
            if (lpBytesReturned) *lpBytesReturned = 4;
            break;
        default:
            log_warning("mxsram", "unhandled 0x%08x", dwIoControlCode);
            return FALSE;
    }

    return TRUE;
}

DWORD mxsram_SetFilePointer(void* file, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod) {
    if (dwMoveMethod == FILE_BEGIN) {
        SRAM_POINTER = lDistanceToMove;
    } else if (dwMoveMethod == FILE_CURRENT) {
        SRAM_POINTER += lDistanceToMove;
    }
    return SRAM_POINTER;
}

BOOL mxsram_WriteFile(void* file, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten,
                      LPOVERLAPPED lpOverlapped) {
    log_misc("mxsram", "sram write 0x%08x (0x%04x bytes)", SRAM_POINTER, nNumberOfBytesToWrite);
    if (SRAM_POINTER + nNumberOfBytesToWrite >= SRAM_SIZE) {
        nNumberOfBytesToWrite = SRAM_SIZE - SRAM_POINTER;
    }
    memcpy(SRAM + SRAM_POINTER, lpBuffer, nNumberOfBytesToWrite);
    sram_dump();
    *lpNumberOfBytesWritten = nNumberOfBytesToWrite;
    return TRUE;
}

BOOL mxsram_ReadFile(void* file, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead,
                     LPOVERLAPPED lpOverlapped) {
    log_misc("mxsram", "sram read 0x%08x (0x%04x bytes)", SRAM_POINTER, nNumberOfBytesToRead);
    if (SRAM_POINTER + nNumberOfBytesToRead >= SRAM_SIZE) {
        nNumberOfBytesToRead = SRAM_SIZE - SRAM_POINTER;
    }
    sram_restore();
    memcpy((LPVOID)lpBuffer, SRAM + SRAM_POINTER, nNumberOfBytesToRead);
    *lpNumberOfBytesRead = nNumberOfBytesToRead;
    return TRUE;
}

void setup_mxsram() {
    // Allocate 2MB of SRAM
    SRAM = (LPBYTE)malloc(SRAM_SIZE);
    if (!SRAM) {
        log_error(BOOT_LOGGER, "unable to allocate 2MiB for SRAM");
        exit(1);
    }
    memset(SRAM, 0, SRAM_SIZE);
    sram_restore();

    file_hook_t* mxsram = new_file_hook(L"\\\\.\\mxsram");
    mxsram->DeviceIoControl = &mxsram_DeviceIoControl;
    mxsram->SetFilePointer = &mxsram_SetFilePointer;
    mxsram->ReadFile = &mxsram_ReadFile;
    mxsram->WriteFile = &mxsram_WriteFile;

    hook_file(mxsram);
}
