#include "drive.h"

#include <Ntddscsi.h>
#include <Objbase.h>

#include "./files.h"

typedef struct _physical_disk physical_disk_t;
typedef struct _disk_partition disk_partition_t;
typedef struct _disk_volume disk_volume_t;

struct _disk_volume {
    wchar_t m_FilenameW[MAX_PATH + 1];
    char m_FilenameA[MAX_PATH + 1];
    LPSTR m_Name;
    char m_MountPoint;

    // Back-references
    physical_disk_t* m_pDrive;
    disk_partition_t* m_pPartition;
    BOOL m_NameIsOnHeap;
};
struct _disk_partition {
    DWORD m_Size;
    UCHAR m_Filesystem;
    spd_slot_t m_SPDContent;
    UCHAR* m_Content;
    LONGLONG m_ContentSize;

    disk_volume_t m_Volume;

    DWORD m_PhysicalLBA;
    DWORD m_SlotLBA;
    DWORD m_PartitionNumber;
};
struct _physical_disk {
    DWORD m_DriveNumber;
    STORAGE_BUS_TYPE m_BusType;
    LPSTR m_VID;
    LPSTR m_PID;
    DWORD m_BootPartition;
    BOOL m_HasSegaboot;

    disk_partition_t m_Partitions[4];
    disk_partition_t m_Extended[];
};

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

physical_disk_t SSD = {
    .m_DriveNumber = 0,
    .m_BusType = BusTypeAta,
    .m_BootPartition = 1,
    .m_HasSegaboot = TRUE,
    .m_Partitions = {
        // 1.5GB boot partitions
        {
            .m_Size = 0x300B85,
            .m_Filesystem = MBR_FS_NTFS,
        },
        // 1.5GB recovery partitions
        {
            .m_Size = 0x300BC4,
            .m_Filesystem = MBR_FS_NTFS,
        },
    },
    .m_Extended = {
        { 0x102d83, MBR_FS_FAT16, SPD_OS },          // 512MB OS update
        { 0x403947, MBR_FS_FAT16, SPD_Patch0 },      // 2GB patch0
        { 0x403947, MBR_FS_FAT16, SPD_Patch1 },      // 2GB patch1
        { 0x48ed459, MBR_FS_NTFS, SPD_AppData },     // 40GB something
        {
            0x20014aa,
            MBR_FS_FAT16,
            SPD_Original0,
            .m_Content = Original0BootIDHeader,
            .m_ContentSize = sizeof Original0BootIDHeader,
        },  // 16GB game
        { 0 },
    },
};
physical_disk_t UPDATE_USB = {
    .m_DriveNumber = 1,
    .m_BusType = BusTypeUsb,
    .m_VID = "13FE",
    .m_PID = "4200",
    .m_Partitions = {
        // 59050 MB update partitions
        {
            .m_Size = 0x735a080,
            .m_Filesystem = MBR_FS_NTFS,
            .m_Volume = {
                .m_Name = "SEGA_INS",
            },
        },
    },
    .m_Extended = {{0}},
};
physical_disk_t* PHYSICAL_DISKS[] = { &SSD, &UPDATE_USB };

BOOL WINAPI c_drive_DeviceIoControl(file_context_t* ctx, DWORD dwIoControlCode, LPVOID lpInBuffer,
                                    DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                                    LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    log_trace("C", "DeviceIOControl %08x", dwIoControlCode);
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
    log_trace("X", "DeviceIOControl %08x", dwIoControlCode);
    ZeroMemory(lpOutBuffer, nOutBufferSize);
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

BOOL WINAPI pd_DeviceIoControl(file_context_t* ctx, DWORD dwIoControlCode, LPVOID lpInBuffer,
                               DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                               LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    physical_disk_t* pd = (physical_disk_t*)ctx->m_HookData;
    if (pd == NULL) {
        log_error("drive", "ioctl:ctx->m_HookData NULL; expected a physical drive!");
        return FALSE;
    }

    log_trace("drive", "DeviceIOControl %08x", dwIoControlCode);
    ZeroMemory(lpOutBuffer, nOutBufferSize);
    switch (dwIoControlCode) {
        case IOCTL_ATA_PASS_THROUGH:
            if (pd->m_BusType != BusTypeAta) return FALSE;

            PATA_PASS_THROUGH_EX ata = (PATA_PASS_THROUGH_EX)lpInBuffer;

            BYTE command = ata->CurrentTaskFile[6];
            if (command != 0xC1) {
                log_error("pd0", "Unimplemented ATA command: %02x", command);
                return FALSE;
            }

            BYTE data = ata->CurrentTaskFile[0];
            switch (data) {
                    // These are used un UnlockSection1, 2, and 3 in mxstorage!

                case 0x11:  // mxkSsdHostProofSet (step == 0)
                            // Game -> ATA | 128 bytes: random from N2
                case 0x12:  // mxkSsdHostProofGet (step == 0)
                            // ATA -> GAME | 128 bytes: (guess) readback of 0x11
                case 0x21:  // mxkSsdHostProofSet (step == 1)
                            // Game -> ATA | 128 bytes: random from N2
                case 0x22:  // mxkSsdHostProofGet (step == 1)
                            // ATA -> GAME | 128 bytes: (guess) readback of 0x21
                case 0x31:  // mxkSsdHostProofSeed
                            // ATA -> Game | 80 bytes: seed
                case 0x32:  // mxkSsdHostProofChallenge
                    // Game -> ATA | 80 bytes: the seed, encrypted by N2 with S-Key

                    // It looks like mxkeychip never actually checks the response buffer,
                    // as long as the ioctl succeeds! Saves us a lot of work here!
                    return TRUE;
            }
            log_error("drive", "Unimeplemented ATA C1 command: %02x", data);
            return FALSE;
    }
    log_error("drive", "Unimplemented ioctl: %08x", dwIoControlCode);
    return FALSE;
}

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

// Partitions
#define BOOT_PARITION_SIZE 0x300B85      // 1.5GB
#define RECOVER_PARTITION_SIZE 0x300BC4  // 1.5GB
typedef struct {
    uint32_t size;
    uint8_t type;
    spd_slot_t content;
    uint32_t offset;
    LONGLONG slot_offset;
} partition_t;
#define PARITION_OS 0
#define PARITION_PATCH0 1
#define PARITION_PATCH1 2
#define PARITION_APP_DATA 3
#define PARITION_ORIGINAL0 4

#define MBR_LBA_GAP 0x3f  // 1 track worth of sectors is claimed for the MBR
#define EXT_HEADER_GAP 0x3f

#define BLOCKSIZE 512ll

#define SPD_OFFSET 1
#define SBR0_OFFSET 2
#define SBR1_OFFSET 3

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
typedef struct {
    size_t disk;
    size_t partition;
} find_index_t;

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

BOOL pd_ReadFile(file_context_t* ctx, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                 LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    physical_disk_t* pd = (physical_disk_t*)ctx->m_HookData;
    if (pd == NULL) {
        log_error("drive", "read:ctx->m_HookData NULL; expected a physical drive!");
        return FALSE;
    }

    log_info("drive", "PhysicalDrive%d: Read %d @ %llx", pd->m_DriveNumber, nNumberOfBytesToRead,
             ctx->m_Pointer.QuadPart);

    // ! WARNING: This ReadFile implementation is currently limited to aligned
    // ! reads. Given the main purpose of this function is to allow reading
    // ! the partition information, this doesn't currently pose an issue.

    DWORD ptrLBA = (ctx->m_Pointer.QuadPart / BLOCKSIZE) & 0xffffffff;

    // MBR header
    if (ptrLBA == 0) {
        mbr_t* mbr = (mbr_t*)lpBuffer;
        if (nNumberOfBytesToRead < sizeof *mbr) {
            log_error("drive", "Buffer too small for master boot record!");
            return FALSE;
        }

        memset(mbr, 0, sizeof *mbr);
        mbr->sig[0] = 0x55;
        mbr->sig[1] = 0xAA;

        for (size_t i = 0; i < 4; i++) {
            if (pd->m_Partitions[i].m_Size == 0) break;

            mbr->partitions[i].status =
                (pd->m_BootPartition == i + 1) ? MBR_FLAG_BOOTABLE : MBR_FLAG_NONE;
            mbr->partitions[i].type = pd->m_Partitions[i].m_Filesystem;
            mbr->partitions[i].lba = pd->m_Partitions[i].m_PhysicalLBA;
            mbr->partitions[i].sectors = pd->m_Partitions[i].m_Size;
        }

        *lpNumberOfBytesRead = sizeof *mbr;

        return TRUE;
    }
    if (ptrLBA <= MBR_LBA_GAP) {
        // Read within the 63 extra tracks
        log_error("drive", "Read failed");
        return FALSE;
    }

    // MBR partitions
    for (size_t i = 0; i < 4; i++) {
        if (pd->m_Partitions[i].m_PartitionNumber == 0) break;

        if (ptrLBA >= pd->m_Partitions[i].m_PhysicalLBA &&
            ptrLBA < pd->m_Partitions[i].m_PhysicalLBA + pd->m_Partitions[i].m_Size) {
            DWORD readOffset = ptrLBA - pd->m_Partitions[i].m_PhysicalLBA;
            int willRead = pd->m_Partitions[i].m_ContentSize - (readOffset * BLOCKSIZE);
            if (willRead > nNumberOfBytesToRead) willRead = nNumberOfBytesToRead;

            if (willRead < 0) {
                log_error("disk", "Attempted read in %d/%d at block offset %08x; No data to read!",
                          pd->m_DriveNumber, pd->m_Partitions[i].m_PartitionNumber, readOffset);
                return FALSE;
            }

            memcpy(lpBuffer, pd->m_Partitions[i].m_Content, willRead);
            *lpNumberOfBytesRead = willRead;

            log_info("drive", "Read at %d/%d+%d", pd->m_DriveNumber,
                     pd->m_Partitions[i].m_PartitionNumber, readOffset);
            return TRUE;
        }
    }

    // Extended partitions
    for (size_t i = 0; pd->m_Extended[i].m_Size; i++) {
        // Extended header
        DWORD headerLBA = pd->m_Extended[i].m_PhysicalLBA - EXT_HEADER_GAP;
        if (ptrLBA == headerLBA) {
            mbr_t* mbr = (mbr_t*)lpBuffer;
            if (nNumberOfBytesToRead < sizeof *mbr) {
                log_error("drive", "Buffer too small for an extended boot record!");
                return FALSE;
            }

            memset(mbr, 0, sizeof *mbr);
            mbr->sig[0] = 0x55;
            mbr->sig[1] = 0xAA;

            mbr->partitions[0].status = MBR_FLAG_NONE;
            mbr->partitions[0].type = pd->m_Extended[i].m_Filesystem;
            mbr->partitions[0].lba = EXT_HEADER_GAP;
            mbr->partitions[0].sectors = pd->m_Extended[i].m_Size;

            if (pd->m_Extended[i + 1].m_Size) {
                mbr->partitions[1].status = MBR_FLAG_NONE;
                // ! mxinstaller expects to see CHS here, then uses the LBA values
                mbr->partitions[1].type = MBR_FS_EXT_CHS;
                mbr->partitions[1].lba =
                    pd->m_Extended[i + 1].m_PhysicalLBA - pd->m_Extended[0].m_PhysicalLBA;
                mbr->partitions[1].sectors = pd->m_Extended[i + 1].m_Size + EXT_HEADER_GAP;
            }

            *lpNumberOfBytesRead = sizeof *mbr;
            return TRUE;
        }

        if (i == 0 && pd->m_HasSegaboot) {
            // SEGA Partition Description
            if (ptrLBA == headerLBA + SPD_OFFSET) {
                spd_t* spd = (spd_t*)lpBuffer;
                if (nNumberOfBytesToRead < sizeof *spd) {
                    log_error("drive", "Buffer too small for SPD!");
                    return FALSE;
                }

                spd->version = SPD_VERSION;
                for (size_t j = 0; pd->m_Extended[j].m_Size; j++) {
                    spd->slots[j].block_size = BLOCKSIZE;
                    spd->slots[j].block_count = pd->m_Extended[j].m_Size;
                    spd->slots[j].slot_content = pd->m_Extended[j].m_SPDContent;
                    spd->slots[j].uk1 = pd->m_Extended[j].m_Filesystem == MBR_FS_FAT16 ? 0 : 1;
                }

                spd->crc = amCrc32RGet(sizeof *spd - 4, &(spd->version), 0);
                *lpNumberOfBytesRead = sizeof *spd;
                return TRUE;
            }
            // SEGA Boot Record 0 and 1. The two are a redundant copy of each other
            if (ptrLBA == headerLBA + SBR0_OFFSET || ptrLBA == headerLBA + SBR1_OFFSET) {
                SegaBootRecord.crc = amCrc32RGet(sizeof SegaBootRecord - 4, &SegaBootRecord.version, 0);

                memcpy(lpBuffer, &SegaBootRecord, sizeof SegaBootRecord);
                *lpNumberOfBytesRead = sizeof SegaBootRecord;
                return TRUE;
            }
        }

        if (ptrLBA >= pd->m_Extended[i].m_PhysicalLBA - EXT_HEADER_GAP &&
            ptrLBA < pd->m_Extended[i].m_PhysicalLBA) {
            // Read within the 63 extra tracks
            log_error("drive", "Read failed");
            return FALSE;
        }

        if (ptrLBA >= pd->m_Extended[i].m_PhysicalLBA &&
            ptrLBA < pd->m_Extended[i].m_PhysicalLBA + pd->m_Extended[i].m_Size) {
            DWORD readOffset = ptrLBA - pd->m_Extended[i].m_PhysicalLBA;
            int willRead = pd->m_Extended[i].m_ContentSize - (readOffset * BLOCKSIZE);
            if (willRead > nNumberOfBytesToRead) willRead = nNumberOfBytesToRead;

            if (willRead < 0) {
                log_error("disk", "Attempted read in %d/%d at block offset %08x; No data to read!",
                          pd->m_DriveNumber, pd->m_Extended[i].m_PartitionNumber, readOffset);
                return FALSE;
            }

            memcpy(lpBuffer, pd->m_Extended[i].m_Content, willRead);
            *lpNumberOfBytesRead = willRead;

            log_info("drive", "Read at %d/%d+%d", pd->m_DriveNumber,
                     pd->m_Partitions[i].m_PartitionNumber, readOffset);
            return TRUE;
        }
    }

    log_error("drive", "Read failed");
    return FALSE;
}

BOOL pd_WriteFile(file_context_t* ctx, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                  LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    physical_disk_t* pd = (physical_disk_t*)ctx->m_HookData;
    if (pd == NULL) {
        log_error("drive", "write:ctx->m_HookData NULL; expected a physical drive!");
        return FALSE;
    }

    log_warning("drive", "PhysicalDrive%d: Write %d @ %llx", pd->m_DriveNumber,
                nNumberOfBytesToWrite, ctx->m_Pointer.QuadPart);

    for (DWORD i = 0; i < nNumberOfBytesToWrite; i += 32) {
        for (int j = 0; j < 32; j++) {
            printf("%02x ", ((BYTE*)lpBuffer)[i + j]);
        }
        puts("");
    }

    *lpNumberOfBytesWritten = nNumberOfBytesToWrite;
    return TRUE;
}

disk_volume_t* incrementFindIndex(HANDLE hFindVolume) {
    find_index_t* find_index = GetDataForHandle(hFindVolume, HDATA_FIND_VOLUME);
    if (find_index == NULL) return NULL;

    while (1) {
        if (find_index->disk >= sizeof PHYSICAL_DISKS / sizeof PHYSICAL_DISKS[0]) {
            return NULL;
        }

        if (find_index->partition > 3) {
            if (PHYSICAL_DISKS[find_index->disk]
                    ->m_Extended[find_index->partition - 4]
                    .m_PartitionNumber == 0) {
                find_index->disk++;
                find_index->partition = 0;
                continue;
            }

            return &(PHYSICAL_DISKS[find_index->disk]
                         ->m_Extended[(find_index->partition++) - 4]
                         .m_Volume);
        }
        if (PHYSICAL_DISKS[find_index->disk]
                ->m_Partitions[find_index->partition]
                .m_PartitionNumber == 0) {
            find_index->partition = 4;
            continue;
        }

        return &(PHYSICAL_DISKS[find_index->disk]->m_Partitions[find_index->partition++].m_Volume);
    }
}
BOOL WINAPI FakeFindNextVolumeW(HANDLE hFindVolume, LPWSTR lpszVolumeName, DWORD cchBufferLength) {
    disk_volume_t* volume = incrementFindIndex(hFindVolume);
    if (volume == NULL) {
        SetLastError(ERROR_NO_MORE_FILES);
        return FALSE;
    }

    wcscpy_s(lpszVolumeName, cchBufferLength, volume->m_FilenameW);
    wcscat_s(lpszVolumeName, cchBufferLength, L"\\");
    return TRUE;
}
BOOL WINAPI FakeFindNextVolumeA(HANDLE hFindVolume, LPSTR lpszVolumeName, DWORD cchBufferLength) {
    disk_volume_t* volume = incrementFindIndex(hFindVolume);
    if (volume == NULL) {
        SetLastError(ERROR_NO_MORE_FILES);
        return FALSE;
    }

    strcpy_s(lpszVolumeName, cchBufferLength, volume->m_FilenameA);
    strcat_s(lpszVolumeName, cchBufferLength, "\\");
    return TRUE;
}
HANDLE WINAPI FakeFindFirstVolumeW(LPWSTR lpszVolumeName, DWORD cchBufferLength) {
    find_index_t* find_index = malloc(sizeof(find_index_t));
    find_index->disk = 0;
    find_index->partition = 0;
    HANDLE handle = GetDummyHandle();
    SetDataForHandle(handle, HDATA_FIND_VOLUME, find_index, TRUE);

    FakeFindNextVolumeW(handle, lpszVolumeName, cchBufferLength);
    return handle;
}
HANDLE WINAPI FakeFindFirstVolumeA(LPSTR lpszVolumeName, DWORD cchBufferLength) {
    find_index_t* find_index = malloc(sizeof(find_index_t));
    find_index->disk = 0;
    find_index->partition = 0;
    HANDLE handle = GetDummyHandle();
    SetDataForHandle(handle, HDATA_FIND_VOLUME, find_index, TRUE);

    FakeFindNextVolumeA(handle, lpszVolumeName, cchBufferLength);
    return handle;
}
BOOL WINAPI FakeFindVolumeClose(HANDLE hFindVolume) {
    if (RemoveDataForHandle(hFindVolume, HDATA_FIND_VOLUME)) return _CloseHandle(hFindVolume);
    return FALSE;
}

disk_volume_t* getVolumeByPath(LPCSTR lpRootPathName) {
    // Unimplemented
    if (lpRootPathName == NULL) return FALSE;

    char filename[MAX_PATH + 1];

    for (size_t disk = 0; disk < sizeof PHYSICAL_DISKS / sizeof PHYSICAL_DISKS[0]; disk++) {
        for (size_t part = 0; part < 4; part++) {
            strcpy_s(filename, sizeof filename,
                     PHYSICAL_DISKS[disk]->m_Partitions[part].m_Volume.m_FilenameA);
            strcat_s(filename, sizeof filename, "\\");
            if (strcmp(filename, lpRootPathName)) continue;

            return &(PHYSICAL_DISKS[disk]->m_Partitions[part].m_Volume);
        }

        for (size_t part = 0; PHYSICAL_DISKS[disk]->m_Extended[part].m_PartitionNumber; part++) {
            strcpy_s(filename, sizeof filename,
                     PHYSICAL_DISKS[disk]->m_Extended[part].m_Volume.m_FilenameA);
            strcat_s(filename, sizeof filename, "\\");
            if (strcmp(filename, lpRootPathName)) continue;

            return &(PHYSICAL_DISKS[disk]->m_Extended[part].m_Volume);
        }
    }
    return NULL;
}

BOOL WINAPI FakeGetVolumeInformationA(LPCSTR lpRootPathName, LPSTR lpVolumeNameBuffer,
                                      DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber,
                                      LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags,
                                      LPSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize) {
    if (nVolumeNameSize) lpVolumeNameBuffer[0] = '\0';
    if (lpVolumeSerialNumber) lpVolumeSerialNumber[0] = 0;
    if (lpMaximumComponentLength) *lpMaximumComponentLength = MAX_PATH;
    if (lpFileSystemFlags) *lpFileSystemFlags = 0;
    if (nFileSystemNameSize) lpFileSystemNameBuffer[0] = '\0';

    disk_volume_t* volume = getVolumeByPath(lpRootPathName);
    if (volume == NULL) return FALSE;

    if (volume->m_Name && lpVolumeNameBuffer)
        strcpy_s(lpVolumeNameBuffer, nVolumeNameSize, volume->m_Name);
    return TRUE;
}
BOOL WINAPI FakeSetVolumeLabelA(LPCSTR lpRootPathName, LPCSTR lpVolumeName) {
    disk_volume_t* volume = getVolumeByPath(lpRootPathName);
    if (volume == NULL) return FALSE;

    if (lpVolumeName) {
        size_t len = strlen(lpVolumeName);
        // By default volume names are pointers to constants!
        if (volume->m_NameIsOnHeap)
            realloc(volume->m_Name, len + 1);
        else {
            volume->m_Name = malloc(len + 1);
            volume->m_NameIsOnHeap = TRUE;
        }
        memcpy(volume->m_Name, lpVolumeName, len);
    } else {
        volume->m_Name = NULL;
    }
    return TRUE;
};

BOOL WINAPI volume_DeviceIoControl(file_context_t* ctx, DWORD dwIoControlCode, LPVOID lpInBuffer,
                                   DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                                   LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    disk_volume_t* volume = (disk_volume_t*)ctx->m_HookData;
    if (volume == NULL) {
        log_error("drive", "ctx->m_HookData NULL; expected a volume!");
        return FALSE;
    }

    memset(lpOutBuffer, 0, nInBufferSize);
    switch (dwIoControlCode) {
        case IOCTL_STORAGE_GET_DEVICE_NUMBER:
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
                volume->m_pPartition->m_PhysicalLBA * BLOCKSIZE;
            vde->Extents[0].ExtentLength.QuadPart = volume->m_pPartition->m_Size * BLOCKSIZE;

            return TRUE;
        case IOCTL_STORAGE_QUERY_PROPERTY:
            PSTORAGE_PROPERTY_QUERY queryIn = (PSTORAGE_PROPERTY_QUERY)lpInBuffer;

            if (queryIn->PropertyId != StorageDeviceProperty ||
                queryIn->QueryType != PropertyStandardQuery) {
                log_error("drive", "Unimplemented storage query: %d/%d", queryIn->PropertyId,
                          queryIn->QueryType);
                return FALSE;
            }
            PSTORAGE_DEVICE_DESCRIPTOR descriptor = (PSTORAGE_DEVICE_DESCRIPTOR)(lpOutBuffer);

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
            log_error("drive", "Unimplemented IOCTL: %08x", dwIoControlCode);
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
    log_misc("disk", "Creating fake volume: %ls", volume_name);

    volume_hook->DeviceIoControl = &volume_DeviceIoControl;
    volume_hook->hook_data = (void*)vol;

    hook_file(volume_hook);

    // This is a pretty nasty way to convert from W to A, but all values
    // are always going to be within ASCII range so it's.. okay.
    for (size_t i = 0; i < sizeof vol->m_FilenameA; i++) {
        vol->m_FilenameA[i] = vol->m_FilenameW[i] & 0xff;
    }
}
void init_pd(physical_disk_t* pd) {
    DWORD partitionNumber = 1;
    DWORD currentLBA = MBR_LBA_GAP;

    // Init MBR
    for (int i = 0; i < 4; i++) {
        if (pd->m_Partitions[i].m_Size == 0) break;

        pd->m_Partitions[i].m_PartitionNumber = partitionNumber++;
        pd->m_Partitions[i].m_PhysicalLBA = currentLBA;

        init_volume(&(pd->m_Partitions[i].m_Volume));
        pd->m_Partitions[i].m_Volume.m_pDrive = pd;
        pd->m_Partitions[i].m_Volume.m_pPartition = &(pd->m_Partitions[i]);

        currentLBA += pd->m_Partitions[i].m_Size;
    }

    // If we have any extended partitions
    DWORD extendedPartNo = 0;
    if (pd->m_Extended[0].m_Size) {
        if (partitionNumber == 5) {
            log_error("drive", "Fatal: Too many paritions in drive %d!", pd->m_DriveNumber);
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
    for (int i = 0; pd->m_Extended[i].m_Size; i++) {
        pd->m_Extended[i].m_PartitionNumber = partitionNumber++;

        currentLBA += EXT_HEADER_GAP;
        pd->m_Extended[i].m_PhysicalLBA = currentLBA;
        pd->m_Extended[i].m_SlotLBA = currentLBA;
        currentLBA += pd->m_Extended[i].m_Size;

        init_volume(&(pd->m_Extended[i].m_Volume));
        pd->m_Extended[i].m_Volume.m_pDrive = pd;
        pd->m_Extended[i].m_Volume.m_pPartition = &(pd->m_Extended[i]);

        sbr_slot_t* pslot = get_sbr_slot(pd->m_Extended[i].m_SPDContent);
        if (pslot != NULL) {
            DWORD slot_size = ((LONGLONG)pslot->segcount * (LONGLONG)pslot->segsize) / BLOCKSIZE;
            DWORD slot_offset = pd->m_Extended[i].m_Size - slot_size;
            pd->m_Extended[i].m_SlotLBA += slot_offset;
        }
    }

    // Back-fill, if needed
    if (extendedPartNo) {
        pd->m_Partitions[extendedPartNo - 1].m_Size = currentLBA - extendedStart;
    }

    // Install hooks
    // strlen(\\.\PhysicalDrive255) = 20, then +1 for null
    wchar_t* hookPath = malloc(21 * sizeof(wchar_t));
    swprintf_s(hookPath, 21, L"\\\\.\\PhysicalDrive%d", pd->m_DriveNumber);
    file_hook_t* hook = new_file_hook(hookPath);
    hook->DeviceIoControl = pd_DeviceIoControl;
    hook->ReadFile = pd_ReadFile;
    hook->WriteFile = pd_WriteFile;
    hook->hook_data = (void*)pd;
    hook_file(hook);
}

void hook_drives() {
    init_pd(&SSD);
    init_pd(&UPDATE_USB);

    file_hook_t* c_drive = new_file_hook(L"\\\\.\\C:");
    c_drive->DeviceIoControl = &c_drive_DeviceIoControl;
    hook_file(c_drive);

    file_hook_t* x_drive = new_file_hook(L"X:");
    x_drive->DeviceIoControl = &x_drive_DeviceIoControl;
    hook_file(x_drive);

    hook("Kernel32.dll", "FindFirstVolumeW", &FakeFindFirstVolumeW, NULL, 5);
    hook("Kernel32.dll", "FindNextVolumeW", &FakeFindNextVolumeW, NULL, 5);
    hook("Kernel32.dll", "FindFirstVolumeA", &FakeFindFirstVolumeA, NULL, 5);
    hook("Kernel32.dll", "FindNextVolumeA", &FakeFindNextVolumeA, NULL, 5);
    hook("Kernel32.dll", "FindVolumeClose", &FakeFindVolumeClose, NULL, 5);
    hook("Kernel32.dll", "SetVolumeLabelA", &FakeSetVolumeLabelA, NULL, 5);
    hook("Kernel32.dll", "GetVolumeInformationA", &FakeGetVolumeInformationA, NULL, 5);

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
