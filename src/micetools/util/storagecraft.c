#include <Windows.h>
#include <stdio.h>

#include "../dll/hooks/drive/drive.h"

#define SECTOR_SIZE BLOCK_SIZE_HDD

#define HEADS_PER_CYLINDER 255
#define SECTORS_PER_TRACK 63

#define TRUECRYPT "\"C:\\Program Files (x86)\\TrueCrypt\\TrueCrypt.exe\""
#define OSR_PASSWORD "nogamenolife"
#define OSR_MOUNTPOINT "R:"
#define OSR_FLAGS "/w /q /m ro"

BYTE WINDOWS_MBR_BOOTSTRAP[MBR_BOOTSTRAP_SIZE] = {
    0x33, 0xc0, 0x8e, 0xd0, 0xbc, 0x00, 0x7c, 0xfb, 0x50, 0x07, 0x50, 0x1f, 0xfc, 0xbe, 0x1b, 0x7c,
    0xbf, 0x1b, 0x06, 0x50, 0x57, 0xb9, 0xe5, 0x01, 0xf3, 0xa4, 0xcb, 0xbd, 0xbe, 0x07, 0xb1, 0x04,
    0x38, 0x6e, 0x00, 0x7c, 0x09, 0x75, 0x13, 0x83, 0xc5, 0x10, 0xe2, 0xf4, 0xcd, 0x18, 0x8b, 0xf5,
    0x83, 0xc6, 0x10, 0x49, 0x74, 0x19, 0x38, 0x2c, 0x74, 0xf6, 0xa0, 0xb5, 0x07, 0xb4, 0x07, 0x8b,
    0xf0, 0xac, 0x3c, 0x00, 0x74, 0xfc, 0xbb, 0x07, 0x00, 0xb4, 0x0e, 0xcd, 0x10, 0xeb, 0xf2, 0x88,
    0x4e, 0x10, 0xe8, 0x46, 0x00, 0x73, 0x2a, 0xfe, 0x46, 0x10, 0x80, 0x7e, 0x04, 0x0b, 0x74, 0x0b,
    0x80, 0x7e, 0x04, 0x0c, 0x74, 0x05, 0xa0, 0xb6, 0x07, 0x75, 0xd2, 0x80, 0x46, 0x02, 0x06, 0x83,
    0x46, 0x08, 0x06, 0x83, 0x56, 0x0a, 0x00, 0xe8, 0x21, 0x00, 0x73, 0x05, 0xa0, 0xb6, 0x07, 0xeb,
    0xbc, 0x81, 0x3e, 0xfe, 0x7d, 0x55, 0xaa, 0x74, 0x0b, 0x80, 0x7e, 0x10, 0x00, 0x74, 0xc8, 0xa0,
    0xb7, 0x07, 0xeb, 0xa9, 0x8b, 0xfc, 0x1e, 0x57, 0x8b, 0xf5, 0xcb, 0xbf, 0x05, 0x00, 0x8a, 0x56,
    0x00, 0xb4, 0x08, 0xcd, 0x13, 0x72, 0x23, 0x8a, 0xc1, 0x24, 0x3f, 0x98, 0x8a, 0xde, 0x8a, 0xfc,
    0x43, 0xf7, 0xe3, 0x8b, 0xd1, 0x86, 0xd6, 0xb1, 0x06, 0xd2, 0xee, 0x42, 0xf7, 0xe2, 0x39, 0x56,
    0x0a, 0x77, 0x23, 0x72, 0x05, 0x39, 0x46, 0x08, 0x73, 0x1c, 0xb8, 0x01, 0x02, 0xbb, 0x00, 0x7c,
    0x8b, 0x4e, 0x02, 0x8b, 0x56, 0x00, 0xcd, 0x13, 0x73, 0x51, 0x4f, 0x74, 0x4e, 0x32, 0xe4, 0x8a,
    0x56, 0x00, 0xcd, 0x13, 0xeb, 0xe4, 0x8a, 0x56, 0x00, 0x60, 0xbb, 0xaa, 0x55, 0xb4, 0x41, 0xcd,
    0x13, 0x72, 0x36, 0x81, 0xfb, 0x55, 0xaa, 0x75, 0x30, 0xf6, 0xc1, 0x01, 0x74, 0x2b, 0x61, 0x60,
    0x6a, 0x00, 0x6a, 0x00, 0xff, 0x76, 0x0a, 0xff, 0x76, 0x08, 0x6a, 0x00, 0x68, 0x00, 0x7c, 0x6a,
    0x01, 0x6a, 0x10, 0xb4, 0x42, 0x8b, 0xf4, 0xcd, 0x13, 0x61, 0x61, 0x73, 0x0e, 0x4f, 0x74, 0x0b,
    0x32, 0xe4, 0x8a, 0x56, 0x00, 0xcd, 0x13, 0xeb, 0xd6, 0x61, 0xf9, 0xc3, 0x49, 0x6e, 0x76, 0x61,
    0x6c, 0x69, 0x64, 0x20, 0x70, 0x61, 0x72, 0x74, 0x69, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x74, 0x61,
    0x62, 0x6c, 0x65, 0x00, 0x45, 0x72, 0x72, 0x6f, 0x72, 0x20, 0x6c, 0x6f, 0x61, 0x64, 0x69, 0x6e,
    0x67, 0x20, 0x6f, 0x70, 0x65, 0x72, 0x61, 0x74, 0x69, 0x6e, 0x67, 0x20, 0x73, 0x79, 0x73, 0x74,
    0x65, 0x6d, 0x00, 0x4d, 0x69, 0x73, 0x73, 0x69, 0x6e, 0x67, 0x20, 0x6f, 0x70, 0x65, 0x72, 0x61,
    0x74, 0x69, 0x6e, 0x67, 0x20, 0x73, 0x79, 0x73, 0x74, 0x65, 0x6d, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x44, 0x63, 0x46, 0xa4, 0x2b, 0xd1, 0x00, 0x00,
};

// Assumes a SECTOR_SIZE of 512
#define _KiB(x) (DWORD)(x * 2)
#define _MiB(x) (DWORD)(x * 1024 * 2)
#define _GiB(x) (DWORD)(x * 1024 * 1024 * 2)
#define _GMKiB(g, m, k) (_GiB(g) + _MiB(m) + _KiB(k))

physical_disk_t newSSD = {
    .m_SerialNumber = 0x00144DB0,

    .m_BusType = BusTypeAta,
    .m_BootPartition = 1,
    .m_HasSegaboot = TRUE,
    .m_BlockSize = BLOCK_SIZE_HDD,
    .m_TotalSize = 0,  // Force re-calculation
    .m_DiskType = DiskType_HardDisk,
    .m_IsFormatted = true,
    // .m_Partitions = {
    //     // 1.5GB boot partition
    //     { .m_Size = _GMKiB(1, 513, 450.5), .m_Filesystem = MBR_FS_NTFS },
    //     // 1.5GB OS recovery
    //     { .m_Size = _GMKiB(1, 513, 482), .m_Filesystem = MBR_FS_NTFS },
    // },
    // .m_Extended = {
    //     { _GMKiB(0, 517, 705.5), MBR_FS_FAT16, SPD_OS },      // 512MB OS update
    //     { _GMKiB(0, 2055, 163.5), MBR_FS_FAT16, SPD_Patch0 },  // 2GB patch0
    //     { _GMKiB(0, 2055, 163.5), MBR_FS_FAT16, SPD_Patch1 },  // 2GB patch1
    //     { _GMKiB(36, 474, 556.5), MBR_FS_NTFS, SPD_AppData }, // 40GB appdata
    //     { _GMKiB(16, 2, 597), MBR_FS_FAT16, SPD_Original0 }, // 16GB original0
    //     { 0 },  // End of table
    // },
    .m_Partitions = {
        { .m_Size = 40960, .m_Filesystem = MBR_FS_NTFS },
    },
    .m_Extended = { 0 },
};

sbr_t SegaBootRecord0 = { 0 };
sbr_t SegaBootRecord1 = { 0 };
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
void init_pd(physical_disk_t* pConfig) {
    // Apply default block sizes
    if (!pConfig->m_BlockSize) {
        switch (pConfig->m_DiskType) {
            case DiskType_HardDisk:
                pConfig->m_BlockSize = BLOCK_SIZE_HDD;
                break;
            case DiskType_Flash:
                pConfig->m_BlockSize = BLOCK_SIZE_FLASH;
                break;
            case DiskType_CdRom:
                pConfig->m_BlockSize = BLOCK_SIZE_CDROM;
                break;
            default:
                printf("Unable to guess block size for drive %d", pConfig->m_DriveNumber);
                break;
        }
    }

    // Non-SCSI devices don't have a SCSI type to report
    if (pConfig->m_BusType != BusTypeScsi) pConfig->m_DeviceType = DeviceTypeUnknown;

    // If we need to initialise the partition tables, do so
    if (pConfig->m_IsFormatted) {
        DWORD partitionNumber = 1;
        DWORD currentLBA = MBR_LBA_GAP;

        // Init MBR
        for (int i = 0; i < 4; i++) {
            if (pConfig->m_Partitions[i].m_Size == 0) break;

            pConfig->m_Partitions[i].m_PartitionNumber = partitionNumber++;
            pConfig->m_Partitions[i].m_PhysicalLBA = currentLBA;

            pConfig->m_Partitions[i].m_Volume.m_pDrive = pConfig;
            pConfig->m_Partitions[i].m_Volume.m_pPartition = &(pConfig->m_Partitions[i]);

            currentLBA += pConfig->m_Partitions[i].m_Size;
        }

        // If we have any extended partitions
        DWORD extendedPartNo = 0;
        if (pConfig->m_Extended[0].m_Size) {
            if (partitionNumber == 5) {
                printf("Fatal: Too many paritions in drive %d!", pConfig->m_DriveNumber);
                exit(1);
            }
            pConfig->m_Partitions[partitionNumber - 1].m_Filesystem = MBR_FS_EXT_LBA;
            pConfig->m_Partitions[partitionNumber - 1].m_PhysicalLBA = currentLBA;
            extendedPartNo = partitionNumber;

            // Note: We don't increment partitionNumber, to keep the presence of this partition
            // transparent elsewhere.
        }
        DWORD extendedStart = currentLBA;

        // Init extended partitions
        for (int i = 0;; i++) {
            if (!pConfig->m_Extended[i].m_Size) break;

            pConfig->m_Extended[i].m_PartitionNumber = partitionNumber++;

            currentLBA += EXT_HEADER_GAP;
            pConfig->m_Extended[i].m_PhysicalLBA = currentLBA;
            pConfig->m_Extended[i].m_SlotLBA = currentLBA;
            currentLBA += pConfig->m_Extended[i].m_Size;

            pConfig->m_Extended[i].m_Volume.m_pDrive = pConfig;
            pConfig->m_Extended[i].m_Volume.m_pPartition = &(pConfig->m_Extended[i]);

            sbr_slot_t* pslot = get_sbr_slot(pConfig->m_Extended[i].m_SPDContent);
            if (pslot != NULL) {
                DWORD slot_size = ((LONGLONG)pslot->segcount * (LONGLONG)pslot->segsize) /
                                  (long long)pConfig->m_BlockSize;
                DWORD slot_offset = pConfig->m_Extended[i].m_Size - slot_size;
                pConfig->m_Extended[i].m_SlotLBA += slot_offset;
            }
        }

        // Back-fill, if needed
        if (extendedPartNo) {
            pConfig->m_Partitions[extendedPartNo - 1].m_Size = currentLBA - extendedStart;
        }

        pConfig->m_TotalSize = currentLBA;
    } else {
        // Raw disks have just a single spanning volume
        pConfig->m_Partitions[0].m_PartitionNumber = 1;
        pConfig->m_Partitions[0].m_PhysicalLBA = 0;
        pConfig->m_Partitions[0].m_Size = pConfig->m_TotalSize;
        pConfig->m_Partitions[0].m_Volume.m_pDrive = pConfig;
        pConfig->m_Partitions[0].m_Volume.m_pPartition = &(pConfig->m_Partitions[0]);
    }
}

inline void set_chs(BYTE* chs, WORD cylinder, BYTE head, BYTE sector) {
    chs[0] = head;
    chs[1] = ((sector & 0x3f) | ((cylinder & 0x300) >> 2)) & 0xff;
    chs[2] = cylinder & 0xff;
}
inline void set_chs_from_lba(BYTE* chs, DWORD lba) {
    WORD cylinder = (lba / (HEADS_PER_CYLINDER * SECTORS_PER_TRACK)) & 0xffff;
    BYTE head = ((lba / SECTORS_PER_TRACK) % HEADS_PER_CYLINDER) & 0xff;
    BYTE sector = ((lba % SECTORS_PER_TRACK) + 1) & 0xff;
    set_chs(chs, cylinder, head, sector);
}
inline void fix_partition_chs(PMBR_PARTITION partition) {
    set_chs_from_lba(partition->start_chs, partition->lba);
    set_chs_from_lba(partition->end_chs, partition->lba + partition->sectors - 1);
}

BYTE writeBuffer[1024];
void write_buffer(HANDLE hDisk, DWORD nBlock, DWORD nBytes, physical_disk_t* pConfig) {
    LARGE_INTEGER offset;
    offset.HighPart = 0;
    offset.LowPart = nBlock;
    offset.QuadPart *= pConfig->m_BlockSize;
    SetFilePointerEx(hDisk, offset, NULL, FILE_BEGIN);
    printf("Performing write: %d@%08x%08x\n", nBytes, offset.HighPart, offset.LowPart);
    DWORD nOut;
    WriteFile(hDisk, writeBuffer, nBytes, &nOut, NULL);
    if (nOut != nBytes) {
        printf("WRITE FAILED!! %d\n", GetLastError());
        CloseHandle(hDisk);
        exit(1);
    }
}

void write_mbr(HANDLE hDisk, LONG nOffset, physical_disk_t* pConfig) {
    printf("Writing MBR at block %d\n", nOffset);
    PMBR_HEADER mbr = (PMBR_HEADER)writeBuffer;

    memset(mbr, 0, sizeof *mbr);
    memcpy_s(mbr->bootstrap_code, sizeof mbr->bootstrap_code, WINDOWS_MBR_BOOTSTRAP,
             sizeof WINDOWS_MBR_BOOTSTRAP);

    mbr->sig[0] = 0x55;
    mbr->sig[1] = 0xAA;

    for (size_t i = 0; i < 4; i++) {
        if (pConfig->m_Partitions[i].m_Size == 0) break;

        mbr->partitions[i].status =
            (pConfig->m_BootPartition == i + 1) ? MBR_FLAG_BOOTABLE : MBR_FLAG_NONE;
        mbr->partitions[i].type = pConfig->m_Partitions[i].m_Filesystem;
        mbr->partitions[i].lba = pConfig->m_Partitions[i].m_PhysicalLBA;
        mbr->partitions[i].sectors = pConfig->m_Partitions[i].m_Size;

        fix_partition_chs(&(mbr->partitions[i]));
    }

    write_buffer(hDisk, nOffset, sizeof *mbr, pConfig);
}
void write_ext_header(HANDLE hDisk, LONG nOffset, physical_disk_t* pConfig, DWORD nPartition) {
    printf("Writing extended header at block %d\n", nOffset);
    printf(" -> Part %d = %d\n", nPartition + 5, nOffset);

    PMBR_HEADER mbr = (PMBR_HEADER)writeBuffer;

    memset(mbr, 0, sizeof *mbr);
    mbr->sig[0] = 0x55;
    mbr->sig[1] = 0xAA;

    mbr->partitions[0].status = MBR_FLAG_NONE;
    mbr->partitions[0].type = pConfig->m_Extended[nPartition].m_Filesystem;
    mbr->partitions[0].lba = pConfig->m_Extended[nPartition].m_PhysicalLBA;
    mbr->partitions[0].sectors = pConfig->m_Extended[nPartition].m_Size;
    fix_partition_chs(&(mbr->partitions[0]));
    // Write the EBR value
    mbr->partitions[0].lba = EXT_HEADER_GAP;

    if (pConfig->m_Extended[nPartition + 1].m_Size) {
        mbr->partitions[1].status = MBR_FLAG_NONE;
        mbr->partitions[1].type = MBR_FS_EXT_CHS;
        mbr->partitions[1].lba = pConfig->m_Extended[nPartition + 1].m_PhysicalLBA - EXT_HEADER_GAP;
        mbr->partitions[1].sectors = pConfig->m_Extended[nPartition + 1].m_Size + EXT_HEADER_GAP;

        fix_partition_chs(&(mbr->partitions[1]));
        // Write the EBR values
        mbr->partitions[1].lba = pConfig->m_Extended[nPartition + 1].m_PhysicalLBA -
                                 pConfig->m_Extended[0].m_PhysicalLBA;

        mbr->partitions[1].end_chs[0] = 0xFE;
        mbr->partitions[1].end_chs[1] = 0xFF;
        mbr->partitions[1].end_chs[2] = 0xFF;
    }

    write_buffer(hDisk, nOffset, sizeof *mbr, pConfig);
}

void write_pd(HANDLE hDisk, physical_disk_t* pConfig) {
    printf("Pre-allocating disk space (0x%x blocks)\n", pConfig->m_TotalSize);
    write_buffer(hDisk, pConfig->m_TotalSize - 1, pConfig->m_BlockSize, pConfig);

    // Write MBR headers
    write_mbr(hDisk, 0, pConfig);
    for (size_t i = 0; pConfig->m_Extended[i].m_Size; i++) {
        DWORD headerLBA = pConfig->m_Extended[i].m_PhysicalLBA - EXT_HEADER_GAP;
        write_ext_header(hDisk, headerLBA, pConfig, i);

        if (i == 0 && pConfig->m_HasSegaboot) {
            // SEGA Partition Description
            spd_t* spd = (spd_t*)writeBuffer;
            ZeroMemory(spd, sizeof *spd);

            spd->version = SPD_VERSION;
            for (size_t j = 0; pConfig->m_Extended[j].m_Size; j++) {
                spd->slots[j].block_size = pConfig->m_BlockSize & 0xFFFF;
                spd->slots[j].block_count = pConfig->m_Extended[j].m_Size;
                spd->slots[j].slot_content = pConfig->m_Extended[j].m_SPDContent;
                spd->slots[j].uk1 = pConfig->m_Extended[j].m_Filesystem == MBR_FS_FAT16 ? 0 : 1;
            }

            spd->crc = amiCrc32RCalc(sizeof *spd - 4, &(spd->version), 0);
            write_buffer(hDisk, headerLBA + SPD_OFFSET, sizeof *spd, pConfig);

            // TODO: Actually populate the SBRs

            // SEGA Boot Record 0 and 1. The two are a redundant copy of each other
            memcpy(writeBuffer, &SegaBootRecord0, sizeof SegaBootRecord0);
            write_buffer(hDisk, headerLBA + SBR0_OFFSET, sizeof SegaBootRecord0, pConfig);

            memcpy(writeBuffer, &SegaBootRecord1, sizeof SegaBootRecord1);
            write_buffer(hDisk, headerLBA + SBR1_OFFSET, sizeof SegaBootRecord1, pConfig);

            printf(" -> SEGA structures = %d+[1/2/3]\n", headerLBA);
        }
    }
}

void print_bar(LONGLONG nCurrent, LONGLONG nMax) {
    printf("[");
    DWORD filled = (DWORD)(((float)nCurrent / (float)nMax) * 40);
    DWORD i = 0;
    for (; i < filled; i++) printf("=");
    for (; i < 40; i++) printf(" ");
    printf("] %3.2f%%\r", (float)nCurrent / (float)nMax * 100);
}
void end_bar() { printf("[========================================] 100.00%%\n"); }

BOOL mount_OSR(LPSTR lpOSRPath) {
    char szTcCommand[1024];
    sprintf_s(szTcCommand, sizeof szTcCommand, "%s /p %s /v %s /l %s %s", TRUECRYPT, OSR_PASSWORD,
              lpOSRPath, OSR_MOUNTPOINT, OSR_FLAGS);

    int dTcRet = system(szTcCommand);
    if (dTcRet != 0) {
        printf("Failed to mount OSR: %d\n", dTcRet);
        printf("Command used was: %s\n", szTcCommand);
        return FALSE;
    }
    return TRUE;
}
BOOL dismount_OSR() {
    char szTcCommand[1024];
    sprintf_s(szTcCommand, sizeof szTcCommand, "%s /q /d %s", TRUECRYPT, OSR_MOUNTPOINT);

    int dTcRet = system(szTcCommand);
    if (dTcRet != 0) {
        printf("Failed to dismount OSR: %d\n", dTcRet);
        printf("Command used was: %s\n", szTcCommand);
        return FALSE;
    }
    return TRUE;
}

void install_OSR(HANDLE hDisk, LPSTR lpOSRPath) {
    if (!mount_OSR(lpOSRPath)) return;

    HANDLE hOSR = CreateFileA("\\\\.\\" OSR_MOUNTPOINT, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOSR == INVALID_HANDLE_VALUE) {
        printf("Failed to open OSR! %d\n", GetLastError());
        return;
    }

    DISK_GEOMETRY dg;
    DWORD nBytesReturned;
    DeviceIoControl(hOSR, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &dg, sizeof dg, &nBytesReturned,
                    NULL);
    LONGLONG nBytes =
        dg.BytesPerSector * dg.SectorsPerTrack * dg.TracksPerCylinder * dg.Cylinders.QuadPart;

    SetFilePointer(hDisk, 63 * 512, NULL, FILE_BEGIN);

    puts("Installing Operating System");

    BYTE block[4096];
    int print_tick = 0;
    for (LONGLONG i = 0; nBytes;) {
        DWORD nRead = sizeof block;
        if (nRead > nBytes) nRead = (DWORD)nBytes;

        ReadFile(hOSR, block, nRead, &nRead, NULL);
        if (nRead == 0) {
            if (GetLastError() == ERROR_SUCCESS) break;

            printf("Failed to read OSR. Code: %d\n", GetLastError());
            goto end;
            return;
        }

        DWORD nWrote = 0;
        WriteFile(hDisk, block, nRead, &nWrote, NULL);
        if (nWrote != nRead) {
            printf("Failed to write OSR to disk. Code: %d\n", GetLastError());
            goto end;
            return;
        }

        if (print_tick++ == 512) {
            print_tick = 0;
            print_bar(i, nBytes);
        }

        i += nRead;
    }
    end_bar();
end:
    CloseHandle(hOSR);
    dismount_OSR();
}
BOOL install_OSR_xcopy(LPSTR lpDiskPath, LPSTR lpOSRPath) {
    if (!mount_OSR(lpOSRPath)) return FALSE;

    printf("Installing OS using xcopy %s -> %s\n", OSR_MOUNTPOINT, lpDiskPath);

    char szXcCommand[1024];
    sprintf_s(szXcCommand, sizeof szXcCommand, "xcopy %s %s /e /c /h /y /q", OSR_MOUNTPOINT,
              lpDiskPath);

    int dTcRet = system(szXcCommand);
    if (dTcRet != 0) {
        printf("Failed to xcopy OSR: %d\n", dTcRet);
        printf("Command used was: %s\n", szXcCommand);
        return FALSE;
    }

    if (!dismount_OSR()) return FALSE;
    return TRUE;
}

int main(int argc, char* argv[]) {
    amiCrc32RInit();

    init_pd(&newSSD);

    HANDLE hDisk = INVALID_HANDLE_VALUE;
    SetLastError(0);

    hDisk = CreateFileA("H:\\NewDiskImage1.img", GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, NULL);

    // hDisk = CreateFileA("\\\\.\\PHYSICALDRIVE4", GENERIC_READ | GENERIC_WRITE,
    //                     FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if (hDisk == INVALID_HANDLE_VALUE) {
        printf("Failed to open disk! %d\n", GetLastError());
        return 1;
    }

    write_pd(hDisk, &newSSD);

    // install_OSR(hDisk, "H:\\Arcades\\RingOS\\RecoverOS\\0009\\Minint\\system32\\OSR");
    // install_OSR_xcopy(hDisk, "H:\\Arcades\\RingOS\\RecoverOS\\0009\\Minint\\system32\\OSR");

    // puts("Please mount OS at W:");
    // system("pause");
    // install_OSR_xcopy("W:", "H:\\Arcades\\RingOS\\RecoverOS\\0009\\Minint\\system32\\OSR");

    FlushFileBuffers(hDisk);
    CloseHandle(hDisk);
    return 0;
}
