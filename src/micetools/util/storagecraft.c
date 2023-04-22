#include <Windows.h>
#include <stdio.h>

#include "../dll/hooks/drive/drive.h"

physical_disk_t newSSD = {
    .m_SerialNumber = 0x00144DB0,

    .m_BusType = BusTypeAta,
    .m_BootPartition = 1,
    .m_HasSegaboot = TRUE,
    .m_BlockSize = BLOCK_SIZE_HDD,
    .m_TotalSize = 64 * 1024 * 1024 * (1024 / BLOCK_SIZE_HDD),
    .m_DiskType = DiskType_HardDisk,
    .m_IsFormatted = true,
    .m_Partitions = {
        // 1.5GB boot partition
        { .m_Size = 0x300B85, .m_Filesystem = MBR_FS_NTFS },
        // 1.5GB OS recovery
        { .m_Size = 0x300BC4, .m_Filesystem = MBR_FS_NTFS },
    },
    .m_Extended = {
        { 0x102d83, MBR_FS_FAT16, SPD_OS },      // 512MB OS update
        { 0x403947, MBR_FS_FAT16, SPD_Patch0 },  // 2GB patch0
        { 0x403947, MBR_FS_FAT16, SPD_Patch1 },  // 2GB patch1
        { 0x48ed459, MBR_FS_NTFS, SPD_AppData }, // 40GB appdata
        { 0x20014aa, MBR_FS_FAT16, SPD_Original0 }, // 16GB original0
        { 0 },  // End of table
    },
};

sbr_t SegaBootRecord0;
sbr_t SegaBootRecord1;
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
    } else {
        // Raw disks have just a single spanning volume
        pConfig->m_Partitions[0].m_PartitionNumber = 1;
        pConfig->m_Partitions[0].m_PhysicalLBA = 0;
        pConfig->m_Partitions[0].m_Size = pConfig->m_TotalSize;
        pConfig->m_Partitions[0].m_Volume.m_pDrive = pConfig;
        pConfig->m_Partitions[0].m_Volume.m_pPartition = &(pConfig->m_Partitions[0]);
    }
}

BYTE writeBuffer[1024];
void write_buffer(HANDLE hDisk, LONG nOffset, DWORD nBytes, physical_disk_t* pConfig) {
    LARGE_INTEGER offset;
    offset.QuadPart = nOffset * pConfig->m_BlockSize;
    SetFilePointerEx(hDisk, offset, NULL, FILE_BEGIN);
    DWORD nOut;
    WriteFile(hDisk, writeBuffer, nBytes, &nOut, NULL);
}

// TODO: Write start_chs lol
void write_mbr(HANDLE hDisk, LONG nOffset, physical_disk_t* pConfig) {
    printf("Writing MBR at block %d\n", nOffset);
    mbr_t* mbr = (mbr_t*)writeBuffer;

    memset(mbr, 0, sizeof *mbr);
    mbr->sig[0] = 0x55;
    mbr->sig[1] = 0xAA;

    for (size_t i = 0; i < 4; i++) {
        if (pConfig->m_Partitions[i].m_Size == 0) break;

        mbr->partitions[i].status =
            (pConfig->m_BootPartition == i + 1) ? MBR_FLAG_BOOTABLE : MBR_FLAG_NONE;
        mbr->partitions[i].type = pConfig->m_Partitions[i].m_Filesystem;
        mbr->partitions[i].lba = pConfig->m_Partitions[i].m_PhysicalLBA;
        mbr->partitions[i].sectors = pConfig->m_Partitions[i].m_Size;
    }

    write_buffer(hDisk, nOffset, sizeof *mbr, pConfig);
}
void write_ext_header(HANDLE hDisk, LONG nOffset, physical_disk_t* pConfig, DWORD nPartition) {
    printf("Writing extended header at block %d\n", nOffset);

    mbr_t* mbr = (mbr_t*)writeBuffer;

    memset(mbr, 0, sizeof *mbr);
    mbr->sig[0] = 0x55;
    mbr->sig[1] = 0xAA;

    mbr->partitions[0].status = MBR_FLAG_NONE;
    mbr->partitions[0].type = pConfig->m_Extended[nPartition].m_Filesystem;
    mbr->partitions[0].lba = EXT_HEADER_GAP;
    mbr->partitions[0].sectors = pConfig->m_Extended[nPartition].m_Size;

    if (pConfig->m_Extended[nPartition + 1].m_Size) {
        mbr->partitions[1].status = MBR_FLAG_NONE;
        // ! mxinstaller expects to see CHS here, then uses the LBA values
        mbr->partitions[1].type = MBR_FS_EXT_CHS;
        mbr->partitions[1].lba = pConfig->m_Extended[nPartition + 1].m_PhysicalLBA -
                                 pConfig->m_Extended[0].m_PhysicalLBA;
        mbr->partitions[1].sectors = pConfig->m_Extended[nPartition + 1].m_Size + EXT_HEADER_GAP;
    }

    write_buffer(hDisk, nOffset, sizeof *mbr, pConfig);
}

void write_pd(HANDLE hDisk, physical_disk_t* pConfig) {
    puts("Force-allocating disk space");
    write_buffer(hDisk, pConfig->m_TotalSize - 1, pConfig->m_BlockSize, pConfig);

    // Write MBR headers
    write_mbr(hDisk, 0, pConfig);
    for (size_t i = 0; pConfig->m_Extended[i].m_Size; i++) {
        DWORD headerLBA = pConfig->m_Extended[i].m_PhysicalLBA - EXT_HEADER_GAP;
        write_ext_header(hDisk, headerLBA, pConfig, i);

        if (i == 0 && pConfig->m_HasSegaboot) {
            puts("Writing segaboot");

            // SEGA Partition Description
            spd_t* spd = (spd_t*)writeBuffer;

            spd->version = SPD_VERSION;
            for (size_t j = 0; pConfig->m_Extended[j].m_Size; j++) {
                spd->slots[j].block_size = pConfig->m_BlockSize & 0xFFFF;
                spd->slots[j].block_count = pConfig->m_Extended[j].m_Size;
                spd->slots[j].slot_content = pConfig->m_Extended[j].m_SPDContent;
                spd->slots[j].uk1 = pConfig->m_Extended[j].m_Filesystem == MBR_FS_FAT16 ? 0 : 1;
            }

            spd->crc = amiCrc32RCalc(sizeof *spd - 4, &(spd->version), 0);
            write_buffer(hDisk, headerLBA + SPD_OFFSET, sizeof *spd, pConfig);

            // SEGA Boot Record 0 and 1. The two are a redundant copy of each other
            memcpy(writeBuffer, &SegaBootRecord0, sizeof SegaBootRecord0);
            write_buffer(hDisk, headerLBA + SBR0_OFFSET, sizeof SegaBootRecord0, pConfig);

            memcpy(writeBuffer, &SegaBootRecord1, sizeof SegaBootRecord1);
            write_buffer(hDisk, headerLBA + SBR1_OFFSET, sizeof SegaBootRecord1, pConfig);
        }
    }
}

int main(int argc, char* argv[]) {
    init_pd(&newSSD);

    HANDLE hDisk = INVALID_HANDLE_VALUE;

    hDisk = CreateFileA("H:\\NewDiskImage.img", GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);

    write_pd(hDisk, &newSSD);

    return 0;
}
