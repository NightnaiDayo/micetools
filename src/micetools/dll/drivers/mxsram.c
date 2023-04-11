#include "../../amBackupStructs.h"
#include "../../maiBackupStructs.h"
#include "mx.h"

#define SRAM_PATH L"dev/sram.bin"
#define SRAM_SIZE 1024 * 2084
LPBYTE SRAM_DATA = NULL;
HANDLE SRAM_FILE = INVALID_HANDLE_VALUE;
HANDLE SRAM_FILE_MAPPING = INVALID_HANDLE_VALUE;

#define ADDR_BACKUP 0x0000
#define ADDR_HM_PEAK 0x0200
#define ADDR_TIMEZONE 0x0400
#define ADDR_ERROR_LOG 0x0600
#define ADDR_DUP 0x1000

#define fix_crc(block)                                                              \
    do {                                                                            \
        (block).m_Crc = amiCrc32RCalc(sizeof(block) - 4, (BYTE*)(&(block)) + 4, 0); \
    } while (0)

void build_sram() {
    log_info(plfMxSram, "Building default SRAM file");

    memset(SRAM_DATA, 0xff, SRAM_SIZE);

    // TODO: Do we want to re-enable this with actual values
#if false
    AM_SYSDATAwH_BACKUP Backup = { 0 };
    fix_crc(Backup);
    memcpy(SRAM_DATA + ADDR_BACKUP, (unsigned char*)&Backup, sizeof Backup);
    memcpy(SRAM_DATA + ADDR_BACKUP + ADDR_DUP, (unsigned char*)&Backup, sizeof Backup);

    AM_SYSDATAwH_HM_PEAK HmPeak = { 0 };
    fix_crc(HmPeak);
    memcpy(SRAM_DATA + ADDR_HM_PEAK, (unsigned char*)&HmPeak, sizeof HmPeak);
    memcpy(SRAM_DATA + ADDR_HM_PEAK + ADDR_DUP, (unsigned char*)&HmPeak, sizeof HmPeak);

    AM_SYSDATAwH_TIMEZONE Timezone = { 0 };
    fix_crc(Timezone);
    memcpy(SRAM_DATA + ADDR_TIMEZONE, (unsigned char*)&Timezone, sizeof Timezone);
    memcpy(SRAM_DATA + ADDR_TIMEZONE + ADDR_DUP, (unsigned char*)&Timezone, sizeof Timezone);

    AM_SYSDATAwH_ERROR_LOG ErrorLog = { 0 };
    fix_crc(ErrorLog);
    memcpy(SRAM_DATA + ADDR_ERROR_LOG, (unsigned char*)&ErrorLog, sizeof ErrorLog);
    memcpy(SRAM_DATA + ADDR_ERROR_LOG + ADDR_DUP, (unsigned char*)&ErrorLog, sizeof ErrorLog);
#endif

    return;
}

void ensure_valid_sram() {
    if (!SRAM_DATA) {
        BOOL isNew = !FileExists(SRAM_PATH);

        SRAM_DATA = open_mapped_file(SRAM_PATH, SRAM_SIZE, &SRAM_FILE, &SRAM_FILE_MAPPING);
        if (SRAM_DATA == NULL) {
            log_error(plfMxSram, "SRAM will be memory-backed and not syncronised!");
            SRAM_DATA = malloc(SRAM_SIZE);
        }

        if (isNew) build_sram();
    }
}

BOOL WINAPI mxsram_DeviceIoControl(file_context_t* ctx, DWORD dwIoControlCode, LPVOID lpInBuffer,
                                   DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                                   LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    DWORD SRAM_VERSION = 0x01000001;
    DWORD SRAM_SECTOR_SIZE = 4;  // Max is 0x800

    switch (dwIoControlCode) {
        case IOCTL_MXSRAM_PING:  // Get version
            log_misc(plfMxSram,
                     "DeviceIoControl(<mxsram>, <ping>, 0x%p, 0x%x, -, 0x%x, "
                     "-, -)",
                     lpInBuffer, nInBufferSize, nOutBufferSize);

            ((LPDWORD)lpOutBuffer)[0] = SRAM_VERSION;
            if (lpBytesReturned) *lpBytesReturned = 4;
            break;
        case IOCTL_DISK_GET_DRIVE_GEOMETRY:
            log_misc(plfMxSram,
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
            log_error(plfMxSram, "Unhandled IOCTL_DISK_GET_LENGTH_INFO");
            return FALSE;
        case IOCTL_MXSRAM_GET_SECTOR_SIZE:
            log_misc(plfMxSram,
                     "DeviceIoControl(<mxsram>, <get sector size>, 0x%p, "
                     "0x%x, -, 0x%x, -, -)",
                     lpInBuffer, nInBufferSize, nOutBufferSize);

            ((LPDWORD)lpOutBuffer)[0] = SRAM_SECTOR_SIZE;
            if (lpBytesReturned) *lpBytesReturned = 4;
            break;
        default:
            log_warning(plfMxSram, "unhandled 0x%08x", dwIoControlCode);
            return FALSE;
    }

    return TRUE;
}

BOOL mxsram_WriteFile(file_context_t* ctx, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                      LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    log_misc(plfMxSram, "sram write 0x%04x bytes at 0x%04x", nNumberOfBytesToWrite,
             ctx->m_Pointer.LowPart);
    if (ctx->m_Pointer.LowPart + nNumberOfBytesToWrite >= SRAM_SIZE) {
        nNumberOfBytesToWrite = SRAM_SIZE - ctx->m_Pointer.LowPart;
    }
    ensure_valid_sram();
    memcpy(SRAM_DATA + ctx->m_Pointer.LowPart, lpBuffer, nNumberOfBytesToWrite);
    *lpNumberOfBytesWritten = nNumberOfBytesToWrite;
    return TRUE;
}

BOOL mxsram_ReadFile(file_context_t* ctx, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                     LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    log_misc(plfMxSram, "sram read 0x%04x bytes at 0x%04x", nNumberOfBytesToRead,
             ctx->m_Pointer.LowPart);
    if (ctx->m_Pointer.LowPart + nNumberOfBytesToRead >= SRAM_SIZE) {
        nNumberOfBytesToRead = SRAM_SIZE - ctx->m_Pointer.LowPart;
    }
    ensure_valid_sram();
    memcpy((LPVOID)lpBuffer, SRAM_DATA + ctx->m_Pointer.LowPart, nNumberOfBytesToRead);
    *lpNumberOfBytesRead = nNumberOfBytesToRead;
    return TRUE;
}

void setup_mxsram() {
    file_hook_t* mxsram = new_file_hook(L"\\\\.\\mxsram");
    mxsram->DeviceIoControl = &mxsram_DeviceIoControl;
    mxsram->ReadFile = &mxsram_ReadFile;
    mxsram->WriteFile = &mxsram_WriteFile;

    hook_file(mxsram);
}
