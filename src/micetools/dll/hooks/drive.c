#include "drive.h"

#include "./files.h"

BOOL c_drive_DeviceIoControl(void* file, DWORD dwIoControlCode, LPVOID lpInBuffer,
                             DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                             LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    log_trace("C", "DeviceIOControl %08x", dwIoControlCode);
    memset(lpOutBuffer, 0, nInBufferSize);
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

BOOL x_drive_DeviceIoControl(void* file, DWORD dwIoControlCode, LPVOID lpInBuffer,
                             DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                             LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    log_trace("X", "DeviceIOControl %08x", dwIoControlCode);
    memset(lpOutBuffer, 0, nInBufferSize);
    switch (dwIoControlCode) {
        case IOCTL_STORAGE_GET_DEVICE_NUMBER:
            ((STORAGE_DEVICE_NUMBER*)lpOutBuffer)->DeviceType = FILE_DEVICE_DISK;
            ((STORAGE_DEVICE_NUMBER*)lpOutBuffer)->DeviceNumber = 0;
            return TRUE;
        default:
            log_warning("X", "Unhandled DeviceIOControl %08x", dwIoControlCode);
            return FALSE;
    }
}

LARGE_INTEGER pd0_file_pointer;
BOOL pd0_SetFilePointerEx(void* file, LARGE_INTEGER liDistanceToMove,
                          PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod) {
    log_trace("pd0", "Seek 0x%08x", liDistanceToMove.QuadPart);
    if (dwMoveMethod == FILE_BEGIN)
        pd0_file_pointer = liDistanceToMove;
    else
        pd0_file_pointer.QuadPart += liDistanceToMove.QuadPart;
    if (lpNewFilePointer) lpNewFilePointer->QuadPart = pd0_file_pointer.QuadPart;
    return TRUE;
}
DWORD pd0_SetFilePointer(void* file, LONG liDistanceToMove, PLONG lpDistanceToMoveHigh,
                         DWORD dwMoveMethod) {
    if (lpDistanceToMoveHigh && *lpDistanceToMoveHigh)
        log_trace("pd0", "Seek 0x%x%08x", *lpDistanceToMoveHigh, liDistanceToMove);
    else
        log_trace("pd0", "Seek 0x%08x", liDistanceToMove);

    if (dwMoveMethod == FILE_BEGIN) {
        if (lpDistanceToMoveHigh)
            pd0_file_pointer.HighPart = *lpDistanceToMoveHigh;
        else
            pd0_file_pointer.HighPart = 0;
        pd0_file_pointer.LowPart = liDistanceToMove;
    } else {
        pd0_file_pointer.QuadPart += liDistanceToMove;
        if (lpDistanceToMoveHigh) pd0_file_pointer.HighPart += *lpDistanceToMoveHigh;
    }
    return pd0_file_pointer.LowPart;
}

void write_chs(BYTE* chs, WORD cylinder, BYTE head, BYTE sector) {
    chs[0] = head;
    chs[1] = sector | ((cylinder >> 8) << 6);
    chs[2] = cylinder & 0xff;
}

#pragma pack(1)
typedef struct spd {
    uint32_t crc;
    uint8_t version;
    uint8_t _[11];
#pragma pack(1)
    struct {
        /*
        (BCD)
        1.0 = original0
        1.1 = original1
        2.0 = patch0
        2.1 = patch1
        3.0 = os
        4.0 = app_data
        */
        spd_slot_t slot_content;
        /*
        Guess: Filesystem type
        0: Encrypted FAT16
        1: Decrypted NTFS
        */
        uint8_t uk1;
        uint16_t block_size;
        uint32_t block_count;
        uint8_t __[8];
    } slots[31];
} spd_t;

#pragma pack(1)
typedef struct {
    uint16_t year;
    uint8_t mon;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t _;
} slot_time_t;
#pragma pack(1)
typedef struct {
    char id[4];
    slot_time_t time;
    uint32_t version;
    uint32_t _[2];
    uint32_t segcount;
    uint32_t segsize;

    char hw[3];
    uint8_t instant;
    slot_time_t orgtime;
    uint32_t orgversion;
    uint32_t osver;
    uint32_t ossegcount;

    uint8_t __[8];
} sbr_slot_t;
#pragma pack(1)
enum {
    Slot_Check = 0x00,     // status=error
    Slot_Install = 0x01,   // status=install -> FAILED TO READ PREMADE BLOCK!!
    Slot_Complete = 0x02,  // status=complete
    Slot_Empty = 0x03,     // status=error
    Slot_Error = 0x04,     // status=error
    Slot_Invalid = 0xff,
};
typedef uint8_t slot_status_t;
#pragma pack(1)
typedef struct {
    uint32_t crc;
    uint8_t version;
    uint8_t _[11 + 16 + 32];
    uint8_t bootslot;
    uint8_t __[2];
    slot_status_t slot_status[5];
    uint8_t ___[8 + 16 + 32 + 64];
    sbr_slot_t slot_os;
    sbr_slot_t slot_original0;
    sbr_slot_t slot_appdata;
    sbr_slot_t slot_patch0;
    sbr_slot_t slot_patch1;
} sbr_t;

sbr_t SegaBootRecord = {
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
        .hw = { 'A', 'A', 'S' },
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

        .id = {'S', 'D', 'E', 'Y'},
        .time = { 2018, 10, 29, 15, 7, 36 },
        .version = 0x10061,
        .segcount = 65082,
        .segsize = 0x40000,
        .hw = { 'A', 'A', 'S' },
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

        .id = { 'S', 'D', 'D', 'Z' },
        .time = { 2018, 6, 21, 13, 46, 24 },
        .version = 0x10060,
        .segcount = 173,
        .segsize = 0x40000,
        .hw = { 'A', 'A', 'S' },
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

        .id = { 'S', 'D', 'E', 'Y' },
        .time = { 2018, 11, 16, 19, 4, 48},
        .version = 0x10062,
        .segcount = 173,
        .segsize = 0x40000,
        .hw = { 'A', 'A', 'S' },
        .instant = 0,
        .osver = 0,
        .ossegcount = 0,
        .orgtime = { 2018, 10, 29, 15, 7, 36 },
        .orgversion = 0x10061,
     },
};

#define BOOT_PARITION_SIZE 0x300B85      // 1.5GB
#define RECOVER_PARTITION_SIZE 0x300BC4  // 1.5GB
typedef struct {
    uint32_t size;
    uint8_t type;
    spd_slot_t content;
} partition_t;
#define PARITION_OS 0
#define PARITION_PATCH0 1
#define PARITION_PATCH1 2
#define PARITION_APP_DATA 3
#define PARITION_ORIGINAL0 4
partition_t partitions[] = {
    { 0x102d83, MBR_FS_FAT16, SPD_OS },          // 512MB OS update
    { 0x403947, MBR_FS_FAT16, SPD_Patch0 },      // 2GB patch0
    { 0x403947, MBR_FS_FAT16, SPD_Patch1 },      // 2GB patch1
    { 0x48ed459, MBR_FS_NTFS, SPD_AppData },     // 40GB something
    { 0x20014aa, MBR_FS_FAT16, SPD_Original0 },  // 16GB game
};
#define NUM_PARITIONS (sizeof partitions / sizeof partitions[0])

#define MBR_LBA_GAP 0x3f  // 1 track worth of sectors is claimed for the MBR

#define BLOCKSIZE 512ll

#define SPD_OFFSET ((extended_base * BLOCKSIZE) + sizeof(mbr_t))
#define SBR0_OFFSET (SPD_OFFSET + sizeof(spd_t))
#define SBR1_OFFSET (SBR0_OFFSET + sizeof SegaBootRecord)

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

sbr_slot_t* get_sbr_slot(spd_slot_t slot) {
    switch (slot) {
        case SPD_Original0:
            return &SegaBootRecord.slot_original0;
        case SPD_Original1:
            return NULL;
        case SPD_Patch0:
            return &SegaBootRecord.slot_patch0;
        case SPD_Patch1:
            return &SegaBootRecord.slot_patch1;
        case SPD_OS:
            return &SegaBootRecord.slot_os;
        case SPD_AppData:
            return &SegaBootRecord.slot_appdata;
        default:
            return NULL;
    }
}

BOOL pd0_ReadFile(void* file, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                  LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    log_info("pd0", "Read %d @ %llx", nNumberOfBytesToRead, pd0_file_pointer.QuadPart);

    uint32_t ext_offset = 0;
    uint32_t offsets[NUM_PARITIONS];
    LONGLONG slot_offsets[NUM_PARITIONS];
    uint32_t extended_base = MBR_LBA_GAP + BOOT_PARITION_SIZE + RECOVER_PARTITION_SIZE;
    for (int i = 0; i < NUM_PARITIONS; i++) {
        offsets[i] = ext_offset;
        ext_offset += partitions[i].size + MBR_LBA_GAP;

        sbr_slot_t* pslot = get_sbr_slot(partitions[i].content);
        if (pslot == NULL)
            slot_offsets[i] = offsets[i];
        else {
            LONGLONG partition_start = (offsets[i] + extended_base + MBR_LBA_GAP) * BLOCKSIZE;
            LONGLONG slot_size = (LONGLONG)pslot->segcount * (LONGLONG)pslot->segsize;
            LONGLONG slot_start = partitions[i].size * BLOCKSIZE - slot_size;
            slot_offsets[i] = partition_start + slot_start;
        }
    }

    printf("Offsets0: %08x\n", offsets[0]);
    printf("ExtendBase: %08x\n", extended_base);

    // MBR (C+Recover+Extend)
    if (pd0_file_pointer.QuadPart == 0) {
        mbr_t* mbr = (mbr_t*)lpBuffer;
        memset(mbr, 0, sizeof *mbr);
        mbr->sig[0] = 0x55;
        mbr->sig[1] = 0xAA;

        // 1.5GB C parition
        mbr->partitions[0].status = MBR_FLAG_BOOTABLE;
        mbr->partitions[0].type = MBR_FS_NTFS;
        mbr->partitions[0].lba = MBR_LBA_GAP;
        mbr->partitions[0].sectors = BOOT_PARITION_SIZE;

        // 1.5GB Recovery
        mbr->partitions[1].status = MBR_FLAG_NONE;
        mbr->partitions[1].type = MBR_FS_NTFS;
        mbr->partitions[1].lba = MBR_LBA_GAP + BOOT_PARITION_SIZE;
        mbr->partitions[1].sectors = RECOVER_PARTITION_SIZE;

        // Everything else is in an extended
        mbr->partitions[2].status = MBR_FLAG_NONE;
        mbr->partitions[2].type = MBR_FS_EXT_LBA;
        mbr->partitions[2].lba = MBR_LBA_GAP + BOOT_PARITION_SIZE + RECOVER_PARTITION_SIZE;
        mbr->partitions[2].sectors = ext_offset;

        *lpNumberOfBytesRead = sizeof *mbr;
        return TRUE;
    }

    // Extended partitions
    for (int i = 0; i < NUM_PARITIONS; i++) {
        if (pd0_file_pointer.QuadPart != ((extended_base + offsets[i]) * BLOCKSIZE)) continue;

        mbr_t* mbr = (mbr_t*)lpBuffer;
        memset(mbr, 0, sizeof *mbr);
        mbr->sig[0] = 0x55;
        mbr->sig[1] = 0xAA;

        mbr->partitions[0].status = MBR_FLAG_NONE;
        mbr->partitions[0].type = partitions[i].type;
        mbr->partitions[0].lba = MBR_LBA_GAP;
        mbr->partitions[0].sectors = partitions[i].size;

        if (i != NUM_PARITIONS - 1) {
            mbr->partitions[1].status = MBR_FLAG_NONE;
            mbr->partitions[1].type = MBR_FS_EXT_CHS;
            mbr->partitions[1].lba = offsets[i + 1];
            mbr->partitions[1].sectors = partitions[i + 1].size + MBR_LBA_GAP;
        }

        *lpNumberOfBytesRead = sizeof *mbr;
        return TRUE;
    }

    SegaBootRecord.crc = crc32(sizeof SegaBootRecord - 4, &SegaBootRecord.version, 0);

    // SEGA Partition Description
    if (pd0_file_pointer.QuadPart == SPD_OFFSET) {
        spd_t* spd = (spd_t*)lpBuffer;
        spd->version = SPD_VERSION;
        for (uint8_t i = 0; i < NUM_PARITIONS; i++) {
            spd->slots[i].block_size = BLOCKSIZE;
            spd->slots[i].block_count = partitions[i].size;
            spd->slots[i].slot_content = partitions[i].content;
            spd->slots[i].uk1 = partitions[i].type == MBR_FS_FAT16 ? 0 : 1;
        }
        spd->crc = crc32(sizeof *spd - 4, &(spd->version), 0);

        *lpNumberOfBytesRead = sizeof *spd;
        return TRUE;
    }

    // SEGA Boot Record 0 and 1. The two are a redundant copy of each other
    if (pd0_file_pointer.QuadPart == SBR0_OFFSET || pd0_file_pointer.QuadPart == SBR1_OFFSET) {
        memcpy(lpBuffer, &SegaBootRecord, sizeof SegaBootRecord);
        *lpNumberOfBytesRead = sizeof SegaBootRecord;
        return TRUE;
    }

    // Read within a partition
    if (pd0_file_pointer.QuadPart < MBR_LBA_GAP * BLOCKSIZE) {
        log_error("pd0", "Read performed within the first track of disk!");
    } else if (pd0_file_pointer.QuadPart < (MBR_LBA_GAP + BOOT_PARITION_SIZE) * BLOCKSIZE) {
        log_warning("pd0", "Game attempting to read windows partition!");
    } else if (pd0_file_pointer.QuadPart <
               (MBR_LBA_GAP + BOOT_PARITION_SIZE + RECOVER_PARTITION_SIZE) * BLOCKSIZE) {
        log_warning("pd0", "Game attempting to read recovery partition!");
    } else if (pd0_file_pointer.QuadPart == slot_offsets[PARITION_ORIGINAL0]) {
        log_info("pd0", "Original0 BootID Header read");
        memcpy(lpBuffer, Original0BootIDHeader, sizeof Original0BootIDHeader);
        *lpNumberOfBytesRead = sizeof Original0BootIDHeader;
        return TRUE;
    } else {
        for (int i = 0; i < NUM_PARITIONS; i++) {
            if (pd0_file_pointer.QuadPart <
                (offsets[i] + MBR_LBA_GAP + extended_base) * BLOCKSIZE) {
                log_error("pd0", "Read performed within the first track of partition %d!", i + 5);
                goto warned;
            } else if (pd0_file_pointer.QuadPart <
                       (offsets[i] + MBR_LBA_GAP + partitions[i].size + extended_base) *
                           BLOCKSIZE) {
                LONGLONG partition_start = (offsets[i] + extended_base + MBR_LBA_GAP) * BLOCKSIZE;
                LONGLONG delta = pd0_file_pointer.QuadPart - partition_start;

                sbr_slot_t* slot = get_sbr_slot(partitions[i].content);
                if (slot == NULL) {
                    log_warning("pd0",
                                "Read performed within empty partition %d (partition +%09llx)",
                                i + 5, delta);
                } else {
                    LONGLONG slot_size = slot->segcount * slot->segsize;
                    LONGLONG slot_start = partitions[i].size * BLOCKSIZE - slot_size;
                    log_warning("pd0",
                                "Read performed within slot %d (partition +%09llx/slot +%09llx)", i,
                                delta, delta - slot_start);
                }

                goto warned;
            }
        }
        log_error("pd0", "Read to unmapped address: %llx (%d bytes)", pd0_file_pointer.QuadPart,
                  nNumberOfBytesToRead);
    warned:;
    }

    return FALSE;
}

BOOL pd0_WriteFile(void* file, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                   LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    log_warning("pd0", "Write %d @ %llx", nNumberOfBytesToWrite, pd0_file_pointer.QuadPart);

    for (DWORD i = 0; i < nNumberOfBytesToWrite; i += 32) {
        for (int j = 0; j < 32; j++) {
            printf("%02x ", ((BYTE*)lpBuffer)[i + j]);
        }
        puts("");
    }

    *lpNumberOfBytesWritten = nNumberOfBytesToWrite;
    return TRUE;
}

void hook_drives() {
    file_hook_t* c_drive = new_file_hook(L"\\\\.\\C:");
    c_drive->DeviceIoControl = &c_drive_DeviceIoControl;
    hook_file(c_drive);

    file_hook_t* x_drive = new_file_hook(L"X:");
    x_drive->DeviceIoControl = &x_drive_DeviceIoControl;
    hook_file(x_drive);

    file_hook_t* physical_drive_0 = new_file_hook(L"\\\\.\\PhysicalDrive0");
    // file_hook_t* physical_drive_0 = new_file_hook(L"\\\\.\\PhysicalDrive2063598201");
    // file_hook_t* physical_drive_0 = new_file_hook(L"\\\\.\\PhysicalDrive1811939950");
    physical_drive_0->SetFilePointerEx = pd0_SetFilePointerEx;
    physical_drive_0->SetFilePointer = pd0_SetFilePointer;
    physical_drive_0->ReadFile = pd0_ReadFile;
    physical_drive_0->WriteFile = pd0_WriteFile;
    hook_file(physical_drive_0);
}
