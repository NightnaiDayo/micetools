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
            ((VOLUME_DISK_EXTENTS*)lpOutBuffer)->Extents[0].StartingOffset.QuadPart = 0;
            DWORD a = (sizeof(*((VOLUME_DISK_EXTENTS*)lpOutBuffer)));
            return TRUE;
        default:
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
#define SBR0_OFFSET (SPD_OFFSET + sizeof (spd_t))
#define SBR1_OFFSET (SBR0_OFFSET + sizeof SegaBootRecord)

BOOL pd0_ReadFile(void* file, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                  LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    log_info("pd0", "Read %d @ %llx", nNumberOfBytesToRead, pd0_file_pointer.QuadPart);

    uint32_t ext_offset = 0;
    uint32_t offsets[NUM_PARITIONS];
    uint32_t extended_base = MBR_LBA_GAP + BOOT_PARITION_SIZE + RECOVER_PARTITION_SIZE;
    for (int i = 0; i < NUM_PARITIONS; i++) {
        offsets[i] = ext_offset;
        ext_offset += partitions[i].size + MBR_LBA_GAP;
    }

    printf("%08x\n", offsets[0]);
    printf("%08x\n", extended_base);

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

    crc32_build_table();
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
    } else {
        for (int i = 0; i < NUM_PARITIONS; i++) {
            if (pd0_file_pointer.QuadPart <
                (offsets[i] + MBR_LBA_GAP + extended_base) * BLOCKSIZE) {
                log_error("pd0", "Read performed within the first track of partition %d!", i + 5);
                goto warned;
            } else if (pd0_file_pointer.QuadPart <
                       (offsets[i] + MBR_LBA_GAP + partitions[i].size + extended_base) *
                           BLOCKSIZE) {
                log_warning("pd0", "Read performed within partition %d", i + 5);
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

    file_hook_t* physical_drive_0 = new_file_hook(L"\\\\.\\PhysicalDrive0");
    // file_hook_t* physical_drive_0 = new_file_hook(L"\\\\.\\PhysicalDrive2063598201");
    // file_hook_t* physical_drive_0 = new_file_hook(L"\\\\.\\PhysicalDrive1811939950");
    physical_drive_0->SetFilePointerEx = pd0_SetFilePointerEx;
    physical_drive_0->SetFilePointer = pd0_SetFilePointer;
    physical_drive_0->ReadFile = pd0_ReadFile;
    physical_drive_0->WriteFile = pd0_WriteFile;
    hook_file(physical_drive_0);
}
