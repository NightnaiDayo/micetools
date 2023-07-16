#define _MICE_DRIVE
#include "drive.h"

#include "../../../lib/ami/amiMd5.h"
#include "../../../micemaster/mxmEventLog.h"
#include "../../../sysconf.h"
#include "../../util/_util.h"
#include "../files.h"

static DWORD _AttachedPhysicalDisks = 0;

/*
    First 512 bytes of SPD_Original0:
        LOADER::LoadBootIDHeader
*/
BYTE Original0BootIDHeader[512] = {
    0x4B, 0xDF, 0xB4, 0x43, 0x27, 0x3C, 0xFE, 0xD6, 0xA2, 0xA8, 0xF8, 0xFD, 0xB4, 0x55, 0xE6, 0xBA,
    0x5A, 0x0F, 0xBF, 0x14, 0xA4, 0x37, 0xB6, 0x42, 0xB3, 0xAD, 0x11, 0x4A, 0xCA, 0x1A, 0x4C, 0xD9,
    0x11, 0xF1, 0x60, 0x4B, 0x37, 0x58, 0x9C, 0x9A, 0x89, 0x8F, 0x07, 0x9C, 0xE7, 0xF2, 0x64, 0xCC,
    0x6E, 0x84, 0x87, 0xAA, 0x20, 0xA0, 0xB8, 0x55, 0x9B, 0xE7, 0x79, 0x4F, 0x51, 0x25, 0xC3, 0x7E,
    0xEF, 0xD9, 0x25, 0xA9, 0x94, 0xF1, 0x7F, 0xEF, 0xE1, 0xD9, 0xAE, 0x4F, 0xC2, 0xEE, 0xB6, 0xA8,
    0xD9, 0x54, 0x0F, 0x33, 0xA8, 0xA9, 0x22, 0x72, 0x81, 0xD0, 0xA3, 0x04, 0x5C, 0x45, 0x3E, 0xBE,
    0xF7, 0x2A, 0xED, 0x55, 0xAB, 0x16, 0xC1, 0xA8, 0x61, 0x70, 0xEE, 0x55, 0xCB, 0xE6, 0x68, 0xA5,
    0xB4, 0xDC, 0x30, 0x6D, 0x32, 0xD6, 0x69, 0x8D, 0xFC, 0x90, 0x71, 0x7E, 0xDB, 0x6B, 0x17, 0xFA,
    0xAB, 0xE0, 0x11, 0x14, 0xBA, 0xD9, 0x33, 0xB7, 0x7C, 0x54, 0x9E, 0x21, 0xA1, 0x43, 0xFD, 0x8F,
    0x14, 0xF4, 0xBE, 0x5F, 0x0B, 0x02, 0x8E, 0x87, 0xCA, 0x85, 0xE9, 0xC1, 0x60, 0xF7, 0x7E, 0x44,
    0x55, 0x3A, 0x5E, 0x94, 0x65, 0x95, 0xFD, 0x02, 0xF9, 0xFF, 0xBF, 0x07, 0x80, 0xC5, 0x26, 0x58,
    0x6F, 0x37, 0xFA, 0x59, 0x2F, 0x02, 0xF3, 0x9D, 0x24, 0x7D, 0x42, 0x6B, 0xF3, 0x49, 0x63, 0xC9,
    0x2A, 0xCB, 0xFC, 0x71, 0x69, 0x44, 0xB5, 0xAC, 0xD3, 0x37, 0xA0, 0x01, 0x65, 0x3D, 0x49, 0xC4,
    0x7D, 0xE5, 0xF8, 0x6E, 0x09, 0xC7, 0x3E, 0xD1, 0x96, 0x09, 0x23, 0xA4, 0xE8, 0xA5, 0x6A, 0xA2,
    0x5B, 0x5B, 0xA5, 0x9C, 0xF8, 0x8D, 0x84, 0x55, 0x3F, 0x19, 0x8F, 0xDC, 0xFA, 0x7B, 0xF1, 0xC9,
    0xB6, 0xBF, 0xE8, 0x73, 0xB9, 0xC9, 0xC3, 0x17, 0x14, 0xAB, 0xA3, 0x60, 0x13, 0xED, 0x6D, 0xCC,
    0x10, 0x7B, 0x1D, 0xC6, 0xBC, 0xEC, 0x56, 0xFA, 0x52, 0xC5, 0x4E, 0xAC, 0x8F, 0x36, 0x8B, 0x92,
    0x6C, 0xB5, 0x9A, 0x57, 0x7D, 0xFA, 0x97, 0x72, 0xFC, 0xFA, 0xB8, 0xFE, 0x20, 0x71, 0xFB, 0x63,
    0x00, 0x96, 0x29, 0xCE, 0xE2, 0x06, 0xFF, 0x64, 0x48, 0xB5, 0x1F, 0xD6, 0x88, 0x48, 0x7A, 0x62,
    0x2B, 0xBE, 0xE6, 0xC4, 0xFD, 0xF6, 0x85, 0x45, 0x0A, 0x8C, 0x6C, 0x20, 0x64, 0x05, 0x81, 0x13,
    0xB5, 0x59, 0xAE, 0x34, 0x41, 0x0B, 0xB5, 0x65, 0x57, 0x59, 0x9C, 0xE8, 0xD0, 0xAE, 0x81, 0xD8,
    0x6D, 0xC9, 0xFD, 0xF8, 0xC9, 0x15, 0xB6, 0xDC, 0xC9, 0x13, 0xF2, 0x6E, 0xD9, 0xA5, 0x77, 0x62,
    0xB7, 0x15, 0x61, 0x21, 0x73, 0xFE, 0x0A, 0x57, 0x3B, 0x2C, 0x2F, 0x23, 0xC3, 0x33, 0xB8, 0x77,
    0xCE, 0xCE, 0x76, 0x98, 0xDB, 0xE5, 0x9A, 0x00, 0xE1, 0xC3, 0x6F, 0x7D, 0x42, 0xC4, 0xDE, 0xB7,
    0x1D, 0xA0, 0xC1, 0x1C, 0xB9, 0x09, 0x28, 0xD9, 0x59, 0x9D, 0x3F, 0xEA, 0xF1, 0xB6, 0xA0, 0x1C,
    0x5E, 0x4A, 0xE4, 0x1A, 0xE7, 0xA7, 0x1C, 0xAD, 0xF6, 0xF1, 0xCB, 0x9C, 0x06, 0xE6, 0x4C, 0xF4,
    0xD6, 0xEE, 0x5F, 0x18, 0xF5, 0x00, 0x4A, 0x76, 0x5E, 0x7D, 0x96, 0x20, 0x57, 0x68, 0x23, 0x69,
    0x8F, 0x60, 0x91, 0xBF, 0x00, 0x08, 0xFE, 0x4F, 0x36, 0x45, 0x86, 0x14, 0x48, 0xC5, 0x8B, 0xEA,
    0xE3, 0x64, 0x27, 0x1E, 0x49, 0xDF, 0x98, 0xAD, 0xE2, 0x66, 0x09, 0x07, 0xDD, 0x24, 0xB0, 0x4D,
    0x52, 0xA6, 0xD1, 0x3D, 0xB9, 0x52, 0x0B, 0x88, 0x97, 0x97, 0x0F, 0x83, 0x85, 0xD5, 0x3F, 0x0E,
    0x1A, 0xF2, 0x26, 0xBA, 0x14, 0x53, 0xDD, 0xF4, 0x7D, 0xAF, 0xB6, 0xEE, 0x36, 0x3A, 0xB5, 0xDA,
    0x2F, 0x99, 0xC8, 0x54, 0xD2, 0xDB, 0x52, 0x49, 0xD6, 0xB6, 0x07, 0x1A, 0xBA, 0x9A, 0x85, 0xBB,
};

physical_disk_t* _GetDisk0() {
    for (int i = 0; i < MAX_DISKS; i++) {
        if (AVAILABLE_DISKS[i].m_Installed) return AVAILABLE_DISKS[i].m_Disk;
    }
    return NULL;
}

BOOL ReadFunc_Original0(DWORD nOffset, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                        LPDWORD lpNumberOfBytesRead) {
    *lpNumberOfBytesRead = 0;
    if (!FileExistsA(ORIGINAL0_PATH)) {
        log_error(plfDrive, "Failed to open %s (does not exist)", ORIGINAL0_PATH);
        return FALSE;
    }

    HANDLE hFile =
        _CreateFileA(ORIGINAL0_PATH, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        log_error(plfDrive, "Failed to open %s", ORIGINAL0_PATH);
        return FALSE;
    }
    physical_disk_t* lpSsd = _GetDisk0();
    if (lpSsd == NULL) return FALSE;

    LARGE_INTEGER seekTo;
    seekTo.QuadPart = (LONGLONG)nOffset * (LONGLONG)lpSsd->m_BlockSize;
    _SetFilePointerEx(hFile, seekTo, NULL, FILE_BEGIN);
    BOOL ret = _ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, NULL);
    if (!ret) log_error(plfDrive, "Failed to read to %s: %03x", ORIGINAL0_PATH, GetLastError());
    _CloseHandle(hFile);
    return ret;
}
BOOL WriteFunc_Original0(DWORD nOffset, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                         LPDWORD lpNumberOfBytesWritten) {
    *lpNumberOfBytesWritten = 0;

    HANDLE hFile =
        _CreateFileA(ORIGINAL0_PATH, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        log_error(plfDrive, "Failed to open %s", ORIGINAL0_PATH);
        return FALSE;
    }
    physical_disk_t* lpSsd = _GetDisk0();
    if (lpSsd == NULL) return FALSE;

    LARGE_INTEGER seekTo;
    seekTo.QuadPart = (LONGLONG)nOffset * (LONGLONG)lpSsd->m_BlockSize;
    _SetFilePointerEx(hFile, seekTo, NULL, FILE_BEGIN);
    BOOL ret = _WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, NULL);
    if (!ret) log_error(plfDrive, "Failed to write to %s: %03x", ORIGINAL0_PATH, GetLastError());
    _CloseHandle(hFile);
    return ret;
}

BOOL ReadFunc_Patch0(DWORD nOffset, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                     LPDWORD lpNumberOfBytesRead) {
    *lpNumberOfBytesRead = 0;
    if (!FileExistsA(PATCH0_PATH)) {
        log_error(plfDrive, "Failed to open %s (does not exist)", PATCH0_PATH);
        return FALSE;
    }

    HANDLE hFile =
        _CreateFileA(PATCH0_PATH, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        log_error(plfDrive, "Failed to open %s", PATCH0_PATH);
        return FALSE;
    }
    physical_disk_t* lpSsd = _GetDisk0();
    if (lpSsd == NULL) return FALSE;

    LARGE_INTEGER seekTo;
    seekTo.QuadPart = (LONGLONG)nOffset * (LONGLONG)lpSsd->m_BlockSize;
    _SetFilePointerEx(hFile, seekTo, NULL, FILE_BEGIN);
    BOOL ret = _ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, NULL);
    if (!ret) log_error(plfDrive, "Failed to read to %s: %03x", PATCH0_PATH, GetLastError());
    _CloseHandle(hFile);
    return ret;
}
BOOL WriteFunc_Patch0(DWORD nOffset, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                      LPDWORD lpNumberOfBytesWritten) {
    *lpNumberOfBytesWritten = 0;

    HANDLE hFile =
        _CreateFileA(PATCH0_PATH, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        log_error(plfDrive, "Failed to open %s", PATCH0_PATH);
        return FALSE;
    }
    physical_disk_t* lpSsd = _GetDisk0();
    if (lpSsd == NULL) return FALSE;

    LARGE_INTEGER seekTo;
    seekTo.QuadPart = (LONGLONG)nOffset * (LONGLONG)lpSsd->m_BlockSize;
    _SetFilePointerEx(hFile, seekTo, NULL, FILE_BEGIN);
    BOOL ret = _WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, NULL);
    if (!ret) log_error(plfDrive, "Failed to write to %s: %03x", PATCH0_PATH, GetLastError());
    _CloseHandle(hFile);
    return ret;
}

BOOL WriteFunc_OS(DWORD nOffset, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                  LPDWORD lpNumberOfBytesWritten) {
    *lpNumberOfBytesWritten = nNumberOfBytesToWrite;
    log_warning(plfDrive, "%d bytes write in OS at %08x", nNumberOfBytesToWrite, nOffset);
    return TRUE;
}
BOOL ReadFunc_OSLogFiles(DWORD nOffset, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                         LPDWORD lpNumberOfBytesRead) {
    if (nOffset == 2047 && nNumberOfBytesToRead == sizeof(AM_EVENT_LOG_HEADER)) {
        AM_EVENT_LOG_HEADER header;
        ZeroMemory(&header, sizeof header);

        // BYTE nullBuffer[512];
        // ZeroMemory(nullBuffer, sizeof nullBuffer);

        // AmiMd5 md5;
        // amiMd5Init(&md5);
        // amiMd5Update(&md5, sizeof nullBuffer, nullBuffer);
        // amiMd5Finalise(&md5);

        AmiMd5 md5;
        amiMd5Init(&md5);
        amiMd5Update(&md5, 0, NULL);
        amiMd5Finalise(&md5);

        header.m_nBytes[0] = 0;
        header.m_nBytes[1] = 0;

        memcpy(header.m_md5[0], md5.m_digest, sizeof header.m_md5[0]);
        memcpy(header.m_md5[1], md5.m_digest, sizeof header.m_md5[1]);

        header.m_crc32 = amiCrc32RCalc(sizeof header - 4, ((unsigned char*)&header) + 4, 0);

        memcpy(lpBuffer, &header, sizeof header);
        *lpNumberOfBytesRead = sizeof header;
        return TRUE;
    }
    if (nOffset == 2048) {
        log_warning(plfDrive, "Requsted %d bytes of application log file", nNumberOfBytesToRead);
        ZeroMemory(lpBuffer, nNumberOfBytesToRead);
        *lpNumberOfBytesRead = nNumberOfBytesToRead;
        return TRUE;
    }
    if (nOffset == 2048 + 1024) {
        log_warning(plfDrive, "Requsted %d bytes of system log file", nNumberOfBytesToRead);
        ZeroMemory(lpBuffer, nNumberOfBytesToRead);
        *lpNumberOfBytesRead = nNumberOfBytesToRead;
        return TRUE;
    }

    *lpNumberOfBytesRead = 0;
    return FALSE;
}

BOOL WINAPI c_drive_DeviceIoControl(file_context_t* ctx, DWORD dwIoControlCode, LPVOID lpInBuffer,
                                    DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                                    LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    log_trace(plfDrive, "C:DeviceIOControl %08x", dwIoControlCode);
    ZeroMemory(lpOutBuffer, nOutBufferSize);
    switch (dwIoControlCode) {
        case IOCTL_STORAGE_GET_DEVICE_NUMBER:
            ((STORAGE_DEVICE_NUMBER*)lpOutBuffer)->DeviceType = FILE_DEVICE_DISK;
            ((STORAGE_DEVICE_NUMBER*)lpOutBuffer)->DeviceNumber = 0;
            return TRUE;
        case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS:
            ((VOLUME_DISK_EXTENTS*)lpOutBuffer)->NumberOfDiskExtents = 1;
            ((VOLUME_DISK_EXTENTS*)lpOutBuffer)->Extents[0].DiskNumber = 0;
            return TRUE;
        default:
            return FALSE;
    }
}

BOOL WINAPI x_drive_DeviceIoControl(file_context_t* ctx, DWORD dwIoControlCode, LPVOID lpInBuffer,
                                    DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                                    LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    log_trace(plfDrive, "X:DeviceIOControl %08x", dwIoControlCode);
    ZeroMemory(lpOutBuffer, nOutBufferSize);
    switch (dwIoControlCode) {
        case IOCTL_STORAGE_GET_DEVICE_NUMBER:
            ((STORAGE_DEVICE_NUMBER*)lpOutBuffer)->DeviceType = FILE_DEVICE_DISK;
            ((STORAGE_DEVICE_NUMBER*)lpOutBuffer)->DeviceNumber = 0;
            return TRUE;
        default:
            log_warning(plfDrive, "X:Unhandled DeviceIOControl %08x", dwIoControlCode);
            return FALSE;
    }
}

extern physical_disk_t ALPHA_DVD;  // Bit hacky, but good enough for now
BOOL WINAPI q_drive_DeviceIoControl(file_context_t* ctx, DWORD dwIoControlCode, LPVOID lpInBuffer,
                                    DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                                    LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    log_trace(plfDrive, "Q:DeviceIOControl %08x", dwIoControlCode);
    ZeroMemory(lpOutBuffer, nOutBufferSize);
    switch (dwIoControlCode) {
        case IOCTL_STORAGE_MEDIA_REMOVAL:
            // The game wants to disable ejecting the disk during installing
            return TRUE;
        case IOCTL_CDROM_GET_DRIVE_GEOMETRY:
            ((PDISK_GEOMETRY)lpOutBuffer)->BytesPerSector = ALPHA_DVD.m_BlockSize;
            return TRUE;
        default:
            log_warning(plfDrive, "Q:Unhandled DeviceIOControl %08x", dwIoControlCode);
            return FALSE;
    }
}

sbr_t SegaBootRecordDefault = {
    .version = SBR_VERSION,
    .bootslot = 0x02,

    .slot_status = {  Slot_Complete, Slot_Complete, Slot_Invalid, Slot_Check, Slot_Complete },

    .slot_os = {
        // Starts at 0xc5469400
        // i.e. [partition 6] - (segcount * segsize)
        // i.e. partition 5 = [dead space] [slot_os]
        .id = {'A', 'A', 'S', '0'},
        .time = { 0 },
        .version = 0x450a01,
        .segcount = 1745,
        .segsize = 0x40000,
        .hw = HW_ID,
        .instant = 0,
        .osver = 0x450a01,
        .ossegcount = 0,
        .orgtime = { 0 },
        .orgversion = 0x450a01,
    },
    .slot_original0 = {
        // Starts at 0xb065bae00
        // i.e. [end of disk] - (segcount * segsize)
        // i.e. partition 9 = [dead space] [slot_original0]

        .id = GAME_ID,
        .time = { 2018, 10, 29, 15, 7, 36 },
        .version = 0x10061,
        .segcount = 65082,
        .segsize = 0x40000,
        .hw = HW_ID,
        .instant = 0,
        .osver = 0x450a01,
        .ossegcount = 1745,
        .orgtime = { 2018, 10, 29, 15, 7, 36 },
        .orgversion = 0x10061,
     },
    .slot_appdata = { 0 },
    .slot_patch0 = {
        // Starts at 0x15e49a000
        // i.e. [partition 7] - (segcount * segsize)
        // i.e. partition 6 = [dead space] [something] [patch0]

        .id = GAME_ID,
        .time = { 2018, 6, 21, 13, 46, 24 },
        .version = 0x10060,
        .segcount = 173,
        .segsize = 0x40000,
        .hw = HW_ID,
        .instant = 0,
        .osver = 0,
        .ossegcount = 0,
        .orgtime = { 2018, 5, 16, 20, 7, 12 },
        .orgversion = 0x01005f,
    },
    .slot_patch1 = {
        // Starts at 0x1debcac00
        // i.e. [partition 8] - (segcount * segsize)
        // i.e. partition 7 = [dead space] [something] [patch0]

        .id = GAME_ID,
        .time = { 2018, 11, 16, 19, 4, 48},
        .version = 0x10062,
        .segcount = 173,
        .segsize = 0x40000,
        .hw = HW_ID,
        .instant = 0,
        .osver = 0,
        .ossegcount = 0,
        .orgtime = { 2018, 10, 29, 15, 7, 36 },
        .orgversion = 0x10061,
     },
};
sbr_t SegaBootRecord0;
sbr_t SegaBootRecord1;

// Drives
/**
 * VID_0CA3 = SEGA CORPROATION
 * VID_0928 = PLX / Oxford Semiconductor
 * VID/PID pairs for B:
 *   0CA3/000E
 *   0928/0007
 *   0CA3/000C
 *
 * Mountpoints based on drive name:
 *      Z:      DEV
 *      L:      SEGA_AM_LOG
 */

// Volumes
sbr_slot_t* get_sbr_slot(spd_slot_t slot) {
    switch (slot) {
        case SPD_Original0:
            return &SegaBootRecord0.slot_original0;
        case SPD_Original1:
            return NULL;
        case SPD_Patch0:
            return &SegaBootRecord0.slot_patch0;
        case SPD_Patch1:
            return &SegaBootRecord0.slot_patch1;
        case SPD_OS:
            return &SegaBootRecord0.slot_os;
        case SPD_AppData:
            return &SegaBootRecord0.slot_appdata;
        default:
            return NULL;
    }
}

char matchWorkFilename[MAX_PATH + 1];
inline static BOOL matchVolume(disk_volume_t* volume, LPCSTR lpRootPathName, DWORD match) {
    if (match & VOL_MATCH_GUID) {
        strcpy_s(matchWorkFilename, sizeof matchWorkFilename, volume->m_FilenameA);
        if (strcmp(matchWorkFilename, lpRootPathName) == 0) return TRUE;
        strcat_s(matchWorkFilename, sizeof matchWorkFilename, "\\");
        if (strcmp(matchWorkFilename, lpRootPathName) == 0) return TRUE;
    }
    if (match & VOL_MATCH_PATH && volume->m_MountPoint) {
        sprintf_s(matchWorkFilename, sizeof matchWorkFilename,
                  "\\\\.\\%c:", char_lower(volume->m_MountPoint));
        if (strstr(lpRootPathName, matchWorkFilename) == lpRootPathName) return TRUE;

        sprintf_s(matchWorkFilename, sizeof matchWorkFilename,
                  "\\\\.\\%c:", char_upper(volume->m_MountPoint));
        if (strstr(lpRootPathName, matchWorkFilename) == lpRootPathName) return TRUE;
    }
    if (match & VOL_MATCH_PATH && volume->m_pDrive && volume->m_pPartition) {
        sprintf_s(matchWorkFilename, sizeof matchWorkFilename, "\\device\\harddisk%d\\partition%d",
                  volume->m_pDrive->m_DriveNumber, volume->m_pPartition->m_PartitionNumber);

        if (_stricmp(lpRootPathName, matchWorkFilename) == 0) return TRUE;
    }
    if (match & VOL_MATCH_DOS_DEVICE && volume->m_MountPoint) {
        if (char_lower(volume->m_MountPoint) == char_lower(lpRootPathName[0])) {
            if (lpRootPathName[1] == ':' && lpRootPathName[2] == '\0') return TRUE;
            if (lpRootPathName[1] == ':' && lpRootPathName[2] == '\\' && lpRootPathName[3] == '\0')
                return TRUE;
        }
    }
    return FALSE;
}

disk_volume_t* getVolumeByPath(LPCSTR lpRootPathName, DWORD match) {
    if (lpRootPathName == NULL) {
        log_error(plfDrive, "getVolumeByPath(NULL) unimplemented!");
        return FALSE;
    }

    for (size_t disk = 0; disk < MAX_DISKS && AVAILABLE_DISKS[disk].m_Disk != NULL; disk++) {
        if (!AVAILABLE_DISKS[disk].m_Installed) continue;

        for (size_t part = 0; part < 4; part++) {
            disk_volume_t* volume = &(AVAILABLE_DISKS[disk].m_Disk->m_Partitions[part].m_Volume);
            if (matchVolume(volume, lpRootPathName, match)) return volume;
        }

        for (size_t part = 0; AVAILABLE_DISKS[disk].m_Disk->m_Extended[part].m_PartitionNumber;
             part++) {
            disk_volume_t* volume = &(AVAILABLE_DISKS[disk].m_Disk->m_Extended[part].m_Volume);
            if (matchVolume(volume, lpRootPathName, match)) return volume;
        }
    }
    return NULL;
}

BOOL WINAPI volume_DeviceIoControl(file_context_t* ctx, DWORD dwIoControlCode, LPVOID lpInBuffer,
                                   DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                                   LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    disk_volume_t* volume = (disk_volume_t*)ctx->m_HookData;
    if (volume == NULL) {
        log_error(plfDrive, "ctx->m_HookData NULL; expected a volume!");
        return FALSE;
    }

    memset(lpOutBuffer, 0, nInBufferSize);
    switch (dwIoControlCode) {
        case IOCTL_STORAGE_GET_DEVICE_NUMBER:
            if (volume->m_pDrive->m_DiskType == DiskType_CdRom)
                ((PSTORAGE_DEVICE_NUMBER)lpOutBuffer)->DeviceType = FILE_DEVICE_CD_ROM;
            else
                ((PSTORAGE_DEVICE_NUMBER)lpOutBuffer)->DeviceType = FILE_DEVICE_DISK;
            ((PSTORAGE_DEVICE_NUMBER)lpOutBuffer)->DeviceNumber = volume->m_pDrive->m_DriveNumber;
            ((PSTORAGE_DEVICE_NUMBER)lpOutBuffer)->PartitionNumber =
                volume->m_pPartition->m_PartitionNumber;
            return TRUE;
        case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS:
            PVOLUME_DISK_EXTENTS vde = (PVOLUME_DISK_EXTENTS)lpOutBuffer;

            vde->NumberOfDiskExtents = 1;
            vde->Extents[0].DiskNumber = volume->m_pDrive->m_DriveNumber;

            vde->Extents[0].StartingOffset.QuadPart =
                volume->m_pPartition->m_PhysicalLBA * (long long)volume->m_pDrive->m_BlockSize;
            vde->Extents[0].ExtentLength.QuadPart =
                volume->m_pPartition->m_Size * (long long)volume->m_pDrive->m_BlockSize;

            return TRUE;
        case IOCTL_STORAGE_QUERY_PROPERTY:
            PSTORAGE_PROPERTY_QUERY queryIn = (PSTORAGE_PROPERTY_QUERY)lpInBuffer;

            if (queryIn->PropertyId != StorageDeviceProperty ||
                queryIn->QueryType != PropertyStandardQuery) {
                log_error(plfDrive, "Unimplemented storage query: %d/%d", queryIn->PropertyId,
                          queryIn->QueryType);
                return FALSE;
            }
            PSTORAGE_DEVICE_DESCRIPTOR descriptor = (PSTORAGE_DEVICE_DESCRIPTOR)(lpOutBuffer);

            descriptor->DeviceType = volume->m_pDrive->m_DeviceType & 0xff;
            descriptor->BusType = volume->m_pDrive->m_BusType;

            size_t rdpOffset = offsetof(STORAGE_DEVICE_DESCRIPTOR, RawDeviceProperties);
            if (volume->m_pDrive->m_VID) {
                descriptor->VendorIdOffset = rdpOffset;
                rdpOffset += sprintf_s((char*)descriptor + rdpOffset, nOutBufferSize - rdpOffset,
                                       "%s", volume->m_pDrive->m_VID) +
                             1;
            }
            if (volume->m_pDrive->m_PID) {
                descriptor->ProductIdOffset = rdpOffset;
                rdpOffset += sprintf_s((char*)descriptor + rdpOffset, nOutBufferSize - rdpOffset,
                                       "%s", volume->m_pDrive->m_PID) +
                             1;
            }
            descriptor->RawPropertiesLength =
                rdpOffset - offsetof(STORAGE_DEVICE_DESCRIPTOR, RawDeviceProperties);
            return TRUE;
        case IOCTL_DISK_GET_LENGTH_INFO:
            PGET_LENGTH_INFORMATION length = (PGET_LENGTH_INFORMATION)lpOutBuffer;
            if (volume->m_pPartition) {
                length->Length.QuadPart = volume->m_pPartition->m_Size;
            }
            return TRUE;
        default:
            log_error(plfDrive, "Unimplemented IOCTL: %08x", dwIoControlCode);
            return FALSE;
    }
}

void init_volume(disk_volume_t* vol) {
    wchar_t* volume_name = vol->m_FilenameW;
    wcscpy_s(volume_name, MAX_PATH + 1, L"\\\\?\\Volume");

    GUID guid;
    CoCreateGuid(&guid);
    StringFromGUID2(&guid, volume_name + 10, MAX_PATH + 1 - 10);

    file_hook_t* volume_hook = new_file_hook(volume_name);
    log_misc(plfDrive, "Creating fake volume: %ls (for %s)", volume_name, vol->m_Name);

    volume_hook->DeviceIoControl = &volume_DeviceIoControl;
    volume_hook->hook_data = (void*)vol;

    hook_file(volume_hook);

    // This is a pretty nasty way to convert from W to A, but all values
    // are always going to be within ASCII range so it's.. okay.
    for (size_t i = 0; i < sizeof vol->m_FilenameA; i++) {
        vol->m_FilenameA[i] = vol->m_FilenameW[i] & 0xff;
    }

    // Construct default device names
    if (vol->m_DeviceName[0] == '\0') {
        swprintf_s(vol->m_DeviceName, (sizeof vol->m_DeviceName) / (sizeof vol->m_DeviceName[0]),
                   L"Volume%s", volume_name + 10);
    }
    if (vol->m_DosDeviceName[0] == '\0') {
        swprintf_s(
            vol->m_DosDeviceName, (sizeof vol->m_DosDeviceName) / (sizeof vol->m_DosDeviceName[0]),
            L"%ls\\Volume%d", vol->m_pDrive->m_DosDeviceName, vol->m_pPartition->m_PartitionNumber);
    }
}
void init_pd(physical_disk_t* pd) {
    // Apply default block sizes
    if (!pd->m_BlockSize) {
        switch (pd->m_DiskType) {
            case DiskType_HardDisk:
                pd->m_BlockSize = BLOCK_SIZE_HDD;
                break;
            case DiskType_Flash:
                pd->m_BlockSize = BLOCK_SIZE_FLASH;
                break;
            case DiskType_CdRom:
                pd->m_BlockSize = BLOCK_SIZE_CDROM;
                break;
            default:
                log_error(plfDrive, "Unable to guess block size for drive %d", pd->m_DriveNumber);
                break;
        }
    }

    // Construct default device paths
    if (pd->m_DeviceName[0] == '\0') {
        switch (pd->m_DiskType) {
            case DiskType_HardDisk:
            case DiskType_Flash:  // TODO: Figure out what flash is meant to be called
                swprintf_s(pd->m_DeviceName,
                           (sizeof pd->m_DeviceName) / (sizeof pd->m_DeviceName[0]), L"Harddisk%d",
                           pd->m_DriveNumber);
                break;
            case DiskType_CdRom:
                swprintf_s(pd->m_DeviceName,
                           (sizeof pd->m_DeviceName) / (sizeof pd->m_DeviceName[0]), L"CdRom%d",
                           pd->m_DriveNumber);
                break;
            case DiskType_Floppy:
                swprintf_s(pd->m_DeviceName,
                           (sizeof pd->m_DeviceName) / (sizeof pd->m_DosDeviceName[0]), L"Floppy%d",
                           pd->m_DriveNumber);
                break;
            default:
                log_error(plfDrive, "Unable to set DeviceName for drive %d (type: %d)",
                          pd->m_DriveNumber, pd->m_DiskType);
                break;
        }
    }
    if (pd->m_DosDeviceName[0] == '\0')
        swprintf_s(pd->m_DosDeviceName,
                   (sizeof pd->m_DosDeviceName) / (sizeof pd->m_DosDeviceName[0]),
                   L"PhysicalDrive%d", pd->m_DriveNumber);

    // Non-SCSI devices don't have a SCSI type to report
    if (pd->m_BusType != BusTypeScsi) pd->m_DeviceType = DeviceTypeUnknown;

    // If we need to initialise the partition tables, do so
    if (pd->m_IsFormatted) {
        DWORD partitionNumber = 1;
        DWORD currentLBA = MBR_LBA_GAP;

        // Init MBR
        for (int i = 0; i < 4; i++) {
            if (pd->m_Partitions[i].m_Size == 0) break;

            pd->m_Partitions[i].m_PartitionNumber = partitionNumber++;
            pd->m_Partitions[i].m_PhysicalLBA = currentLBA;

            pd->m_Partitions[i].m_Volume.m_pDrive = pd;
            pd->m_Partitions[i].m_Volume.m_pPartition = &(pd->m_Partitions[i]);
            init_volume(&(pd->m_Partitions[i].m_Volume));

            currentLBA += pd->m_Partitions[i].m_Size;
        }

        // If we have any extended partitions
        DWORD extendedPartNo = 0;
        if (pd->m_Extended[0].m_Size) {
            if (partitionNumber == 5) {
                log_error(plfDrive, "Fatal: Too many paritions in drive %d!", pd->m_DriveNumber);
                exit(1);
            }
            pd->m_Partitions[partitionNumber - 1].m_Filesystem = MBR_FS_EXT_LBA;
            pd->m_Partitions[partitionNumber - 1].m_PhysicalLBA = currentLBA;
            extendedPartNo = partitionNumber;

            // Note: We don't increment partitionNumber, to keep the presence of this partition
            // transparent elsewhere.
        }
        DWORD extendedStart = currentLBA;

        // Init extended partitions
        for (int i = 0;; i++) {
            if (!pd->m_Extended[i].m_Size) break;

            pd->m_Extended[i].m_PartitionNumber = partitionNumber++;

            currentLBA += EXT_HEADER_GAP;
            pd->m_Extended[i].m_PhysicalLBA = currentLBA;
            pd->m_Extended[i].m_SlotLBA = currentLBA;
            currentLBA += pd->m_Extended[i].m_Size;

            pd->m_Extended[i].m_Volume.m_pDrive = pd;
            pd->m_Extended[i].m_Volume.m_pPartition = &(pd->m_Extended[i]);
            init_volume(&(pd->m_Extended[i].m_Volume));

            sbr_slot_t* pslot = get_sbr_slot(pd->m_Extended[i].m_SPDContent);
            if (pslot != NULL) {
                DWORD slot_size = ((LONGLONG)pslot->segcount * (LONGLONG)pslot->segsize) /
                                  (long long)pd->m_BlockSize;
                DWORD slot_offset = pd->m_Extended[i].m_Size - slot_size;
                pd->m_Extended[i].m_SlotLBA += slot_offset;
            }
        }

        // Back-fill, if needed
        if (extendedPartNo) {
            pd->m_Partitions[extendedPartNo - 1].m_Size = currentLBA - extendedStart;
        }
    } else {
        // Raw disks have just a single spanning volume
        pd->m_Partitions[0].m_PartitionNumber = 1;
        pd->m_Partitions[0].m_PhysicalLBA = 0;
        pd->m_Partitions[0].m_Size = pd->m_TotalSize;
        pd->m_Partitions[0].m_Volume.m_pDrive = pd;
        pd->m_Partitions[0].m_Volume.m_pPartition = &(pd->m_Partitions[0]);
        init_volume(&(pd->m_Partitions[0].m_Volume));

        wcscpy_s(pd->m_Partitions[0].m_Volume.m_DeviceName,
                 sizeof pd->m_Partitions[0].m_Volume.m_DeviceName /
                     sizeof pd->m_Partitions[0].m_Volume.m_DeviceName[0],
                 pd->m_DeviceName);
        wcscpy_s(pd->m_Partitions[0].m_Volume.m_DosDeviceName,
                 sizeof pd->m_Partitions[0].m_Volume.m_DosDeviceName /
                     sizeof pd->m_Partitions[0].m_Volume.m_DosDeviceName[0],
                 pd->m_DosDeviceName);
    }

    // Install hooks
    // strlen(\\.\PhysicalDrive255) = 20, then +1 for null
    wchar_t* hookPath = malloc(21 * sizeof(wchar_t));
    swprintf_s(hookPath, 21, L"\\\\.\\%ls", pd->m_DosDeviceName);
    file_hook_t* hook = new_file_hook(hookPath);
    hook->DeviceIoControl = pd_DeviceIoControl;
    hook->ReadFile = pd_ReadFile;
    hook->WriteFile = pd_WriteFile;
    hook->hook_data = (void*)pd;
    hook_file(hook);
}

BOOL attach_disk(virtual_disk_t* lpDisk) {
    if (lpDisk->m_Disk->m_DriveNumber != (DWORD)-1) return FALSE;
    if (lpDisk->m_Attached) return TRUE;
    lpDisk->m_Attached = TRUE;

    lpDisk->m_Disk->m_DriveNumber = _AttachedPhysicalDisks++;
    lpDisk->m_Disk->m_DeviceName[0] = '\0';
    lpDisk->m_Disk->m_DosDeviceName[0] = '\0';
    init_pd(lpDisk->m_Disk);
    return TRUE;
}
BOOL detach_disk(virtual_disk_t* lpDisk) {
    if (lpDisk->m_Disk->m_DriveNumber == (DWORD)-1) return FALSE;
    if (!lpDisk->m_Attached) return TRUE;

    wchar_t hookPath[21];
    swprintf_s(hookPath, _countof(hookPath), L"\\\\.\\%ls", lpDisk->m_Disk->m_DosDeviceName);

    file_hook_t* hook = unhook_file(hookPath);
    if (hook == NULL) return FALSE;

    lpDisk->m_Disk->m_DriveNumber = (DWORD)-1;
    lpDisk->m_Attached = FALSE;
    free((void*)hook->filename);
    free(hook);
    return TRUE;
}

void init_all_pd() {
    for (int i = 0; i < MAX_DISKS && AVAILABLE_DISKS[i].m_Disk != NULL; i++) {
        AVAILABLE_DISKS[i].m_Disk->m_DriveNumber = (DWORD)-1;
        AVAILABLE_DISKS[i].m_Attached = FALSE;
    }
    for (int i = 0; i < MAX_DISKS && AVAILABLE_DISKS[i].m_Disk != NULL; i++) {
        if (AVAILABLE_DISKS[i].m_Installed) attach_disk(&AVAILABLE_DISKS[i]);
    }
}

BOOL q_drive_ReadFile(file_context_t* ctx, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                      LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    log_game(plfDrive, "Q:CDROM read 0x%04x bytes at 0x%08x", nNumberOfBytesToRead,
             ctx->m_Pointer.QuadPart);

    /**
     * [ Drive format ]
     *
     * - AlphaDVD authentication is prefixed onto the drive.
     *  - The real start can be extracted from the header
     *  - offset = 1264 (for my SDCQ)
     *
     * +511: Disk header (1 sector)
     *      000800004574A1180002000053444351
     * +512: Boot ID (128 sectors) [keychip.decrypt]
     */

    // HANDLE hFile = _CreateFileA("\\\\.\\E:", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
    // NULL,
    //                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE hFile = _CreateFileA("H:\\Arcades\\Images\\ALLNet_Pras_Multi_Ver2\\DVR-0069A.iso",
                                GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        log_error(plfDrive, "Q:READ FAILED. FILE FALLBACK FAILED. %d", GetLastError());
        return FALSE;
    }

    _SetFilePointerEx(hFile, ctx->m_Pointer, NULL, FILE_BEGIN);
    _ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
    _CloseHandle(hFile);

    log_warning(plfDrive, "Q:READ FAILED. FILE FALLBACK (Read %d bytes).", *lpNumberOfBytesRead);
    return TRUE;
}

void hook_drives() {
    init_all_pd();

    file_hook_t* c_drive = new_file_hook(L"\\\\.\\C:");
    c_drive->DeviceIoControl = &c_drive_DeviceIoControl;
    hook_file(c_drive);

    file_hook_t* x_drive = new_file_hook(L"X:");
    x_drive->DeviceIoControl = &x_drive_DeviceIoControl;
    hook_file(x_drive);

    file_hook_t* q_drive = new_file_hook(L"\\\\.\\Q:");
    q_drive->DeviceIoControl = &q_drive_DeviceIoControl;
    q_drive->ReadFile = &q_drive_ReadFile;
    hook_file(q_drive);

    hook("Kernel32.dll", "FindFirstVolumeW", &FakeFindFirstVolumeW, NULL);
    hook("Kernel32.dll", "FindNextVolumeW", &FakeFindNextVolumeW, NULL);
    hook("Kernel32.dll", "FindFirstVolumeA", &FakeFindFirstVolumeA, NULL);
    hook("Kernel32.dll", "FindNextVolumeA", &FakeFindNextVolumeA, NULL);
    hook("Kernel32.dll", "FindVolumeClose", &FakeFindVolumeClose, NULL);
    hook("Kernel32.dll", "SetVolumeLabelA", &FakeSetVolumeLabelA, NULL);
    hook("Kernel32.dll", "GetVolumeInformationA", &FakeGetVolumeInformationA, NULL);
    hook("Kernel32.dll", "QueryDosDeviceA", &FakeQueryDosDeviceA, NULL);
    hook("Kernel32.dll", "QueryDosDeviceW", &FakeQueryDosDeviceW, NULL);
    hook("Kernel32.dll", "DefineDosDeviceW", &FakeDefineDosDeviceW, NULL);
    hook("Kernel32.dll", "SetVolumeMountPointW", &FakeSetVolumeMountPointW, NULL);

    hook("Kernel32.dll", "GetVolumeNameForVolumeMountPointA",
         &FakeGetVolumeNameForVolumeMountPointA, NULL);
    hook("Kernel32.dll", "GetVolumePathNamesForVolumeNameA", &FakeGetVolumePathNamesForVolumeNameA,
         NULL);
    hook("Kernel32.dll", "DeleteVolumeMountPointA", &FakeDeleteVolumeMountPointA, NULL);
    hook("Kernel32.dll", "SetVolumeMountPointA", &FakeSetVolumeMountPointA, NULL);
    hook("Kernel32.dll", "DefineDosDeviceA", &FakeDefineDosDeviceA, NULL);
    hook("Kernel32.dll", "GetDriveTypeA", &FakeGetDriveTypeA, (void**)&TrueGetDriveTypeA);
    hook("Kernel32.dll", "GetDiskFreeSpaceExA", &FakeGetDiskFreeSpaceExA,
         (void**)&TrueGetDiskFreeSpaceExA);

    hook("Winmm.dll", "mciSendStringA", &Fake_mciSendStringA, NULL);

    make_dirs(DISK_PATH));

    SegaBootRecordDefault.crc =
        amiCrc32RCalc(sizeof SegaBootRecordDefault - 4, &SegaBootRecordDefault.version, 0);

    HANDLE hFile;
    DWORD numberOfBytesRead;

    if (FileExistsA(SBR0_PATH)) {
        hFile =
            _CreateFileA(SBR0_PATH, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            log_error(plfDrive, "Failed to open %s", SBR0_PATH);
        } else {
            _ReadFile(hFile, &SegaBootRecord0, sizeof SegaBootRecord0, &numberOfBytesRead, NULL);
            _CloseHandle(hFile);
            log_info(plfDrive, "Restored SBR0 from %s", SBR0_PATH);
        }
    } else {
        // memcpy(&SegaBootRecord0, &SegaBootRecordDefault, sizeof SegaBootRecord0);
    }

    if (FileExistsA(SBR1_PATH)) {
        hFile =
            _CreateFileA(SBR1_PATH, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            log_error(plfDrive, "Failed to open %s", SBR1_PATH);
        } else {
            _ReadFile(hFile, &SegaBootRecord1, sizeof SegaBootRecord1, &numberOfBytesRead, NULL);
            _CloseHandle(hFile);
            log_info(plfDrive, "Restored SBR1 from %s", SBR0_PATH);
        }
    } else {
        memcpy(&SegaBootRecord1, &SegaBootRecord0, sizeof SegaBootRecord0);
    }

    /**
     * mxinstaller iterates all volumes to find those that match the slot metadata.
     * It uses:
     *  - FindFirstVolumeW
     *  - FindNextVolumeW
     *  - FindVolumeClose
     * to iterate volumes.
     * For each volume, it:
     *  - opens it with CreateFileW
     *  - calls IOCTL_STORAGE_GET_DEVICE_NUMBER
     *  - asserts that DeviceType == FILE_DEVICE_DISK
     *  - calls IOCTL_VOLUME_GET_VOLUME_DISK_EXTENT
     *  - checks that NumberOfDiskExtents < 128
     *    - this is not fatal, however an error will be shown and only the first 128 will be checked
     *  - for each extent (!! sizeof DISK_EXTENT == 24; it's 8-byte aligned):
     *    - asserts that [].DiskNumber == state_->DiskNumber
     *    - finds a slot that matches:
     *      - slot->offset == [].StartingOffset
     *      - slot->segments == ([].ExtentLength.LowPart >> 9) | ([].ExtentLength.HighPart << 23)
     *                       == [].ExtentLength / 512
     */
}
