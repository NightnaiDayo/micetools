#include "../../amBackupStructs.h"
#include "../../maiBackupStructs.h"
#include "mx.h"

#define SRAM_DUMP L"dev/sram.bin"
#define SRAM_SIZE 1024 * 2084
LPBYTE SRAM;

void sram_dump() {
    HANDLE dump =
        _CreateFileW(SRAM_DUMP, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
    if (dump == INVALID_HANDLE_VALUE) {
        log_error("sram", "CreateFileA(SRAM_DUMP) failed");
        return;
    }
    _WriteFile(dump, SRAM, SRAM_SIZE, NULL, NULL);
    _CloseHandle(dump);
}

void sram_restore() {
    HANDLE dump = _CreateFileW(SRAM_DUMP, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, NULL);
    if (dump == INVALID_HANDLE_VALUE) return;
    DWORD read;
    if (!_ReadFile(dump, SRAM, SRAM_SIZE, &read, NULL))
        log_error("sram", "failed to restore (%d)", GetLastError());
    _CloseHandle(dump);
}

#define ADDR_BACKUP 0x0000
#define ADDR_HM_PEAK 0x0200
#define ADDR_TIMEZONE 0x0400
#define ADDR_ERROR_LOG 0x0600
#define ADDR_DUP 0x1000

#define fix_crc(block)                                                            \
    do {                                                                          \
        (block).m_Crc = amiCrc32RCalc(sizeof(block) - 4, (BYTE*)(&(block)) + 4, 0); \
    } while (0)

int build_sram() {
    static BOOL built = false;
    if (built) return 0;
    built = true;

    log_info("mxsram", "Building default SRAM file");

    AM_SYSDATAwH_BACKUP Backup = { 0 };
    fix_crc(Backup);
    memcpy(SRAM + ADDR_BACKUP, (unsigned char*)&Backup, sizeof Backup);
    memcpy(SRAM + ADDR_BACKUP + ADDR_DUP, (unsigned char*)&Backup, sizeof Backup);

    AM_SYSDATAwH_HM_PEAK HmPeak = { 0 };
    fix_crc(HmPeak);
    memcpy(SRAM + ADDR_HM_PEAK, (unsigned char*)&HmPeak, sizeof HmPeak);
    memcpy(SRAM + ADDR_HM_PEAK + ADDR_DUP, (unsigned char*)&HmPeak, sizeof HmPeak);

    AM_SYSDATAwH_TIMEZONE Timezone = { 0 };
    fix_crc(Timezone);
    memcpy(SRAM + ADDR_TIMEZONE, (unsigned char*)&Timezone, sizeof Timezone);
    memcpy(SRAM + ADDR_TIMEZONE + ADDR_DUP, (unsigned char*)&Timezone, sizeof Timezone);

    AM_SYSDATAwH_ERROR_LOG ErrorLog = { 0 };
    fix_crc(ErrorLog);
    memcpy(SRAM + ADDR_ERROR_LOG, (unsigned char*)&ErrorLog, sizeof ErrorLog);
    memcpy(SRAM + ADDR_ERROR_LOG + ADDR_DUP, (unsigned char*)&ErrorLog, sizeof ErrorLog);

    sram_dump();

    return 1;
}

BOOL WINAPI mxsram_DeviceIoControl(file_context_t* ctx, DWORD dwIoControlCode, LPVOID lpInBuffer,
                                   DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                                   LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    DWORD SRAM_VERSION = 0x01000001;
    DWORD SRAM_SECTOR_SIZE = 4;  // Max is 0x800

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

            PDISK_GEOMETRY out = (PDISK_GEOMETRY)lpOutBuffer;
            memset(out, 0, sizeof *out);
            out->Cylinders.QuadPart = 256;
            out->MediaType = FixedMedia;
            out->TracksPerCylinder = 2;
            out->SectorsPerTrack = 8;
            out->BytesPerSector = 512;
            if (lpBytesReturned) *lpBytesReturned = sizeof(DISK_GEOMETRY);
            break;
        case IOCTL_DISK_GET_LENGTH_INFO:
            log_error("mxsram", "Unhandled IOCTL_DISK_GET_LENGTH_INFO");
            return FALSE;
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

BOOL mxsram_WriteFile(file_context_t* ctx, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                      LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    log_misc("mxsram", "sram write 0x%04x bytes at 0x%04x", nNumberOfBytesToWrite,
             ctx->m_Pointer.LowPart);
    if (ctx->m_Pointer.LowPart + nNumberOfBytesToWrite >= SRAM_SIZE) {
        nNumberOfBytesToWrite = SRAM_SIZE - ctx->m_Pointer.LowPart;
    }
    memcpy(SRAM + ctx->m_Pointer.LowPart, lpBuffer, nNumberOfBytesToWrite);
    sram_dump();
    *lpNumberOfBytesWritten = nNumberOfBytesToWrite;
    return TRUE;
}

BOOL mxsram_ReadFile(file_context_t* ctx, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                     LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    log_misc("mxsram", "sram read 0x%04x bytes at 0x%04x", nNumberOfBytesToRead,
             ctx->m_Pointer.LowPart);
    if (ctx->m_Pointer.LowPart + nNumberOfBytesToRead >= SRAM_SIZE) {
        nNumberOfBytesToRead = SRAM_SIZE - ctx->m_Pointer.LowPart;
    }
    sram_restore();
    memcpy((LPVOID)lpBuffer, SRAM + ctx->m_Pointer.LowPart, nNumberOfBytesToRead);
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
    memset(SRAM, 0xff, SRAM_SIZE);
    if (FileExists(SRAM_DUMP))
        sram_restore();
    else
        build_sram();

    file_hook_t* mxsram = new_file_hook(L"\\\\.\\mxsram");
    mxsram->DeviceIoControl = &mxsram_DeviceIoControl;
    mxsram->ReadFile = &mxsram_ReadFile;
    mxsram->WriteFile = &mxsram_WriteFile;

    hook_file(mxsram);
}
