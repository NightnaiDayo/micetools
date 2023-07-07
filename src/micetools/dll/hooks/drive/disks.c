#include "drive.h"

/* ================ BOOT SSD ================ */

// 60GB SSD used by maimai and Project Diva
physical_disk_t SSD_60GB = {
    .m_SerialNumber = 0x4d696365,

    .m_BusType = BusTypeAta,
    .m_BootPartition = 1,
    .m_HasSegaboot = TRUE,
    .m_BlockSize = BLOCK_SIZE_HDD,
    // Number of physical sectors. 59.9885 GiB
    .m_TotalSize = 125805015,  // = (sum of parts) + (n * 63)
    .m_DiskType = DiskType_HardDisk,
    .m_IsFormatted = true,
    .m_Partitions = {
        // 1.5GB boot partition
        { .m_Size = 0x300B85, .m_Filesystem = MBR_FS_NTFS },
        // 1.5GB recovery partition
        { .m_Size = 0x300BC4, .m_Filesystem = MBR_FS_NTFS },
    },
    .m_Extended = {
        // 512MB OS update
        { 0x102d83, MBR_FS_FAT16, SPD_OS, .m_ReadFunc = &ReadFunc_OSLogFiles, .m_WriteFunc = &WriteFunc_OS },
        // 2GB patch0
        { 0x403947, MBR_FS_FAT16, SPD_Patch0, .m_ReadFunc = &ReadFunc_Patch0, .m_WriteFunc = &WriteFunc_Patch0 },
        // 2GB patch1
        { 0x403947, MBR_FS_FAT16, SPD_Patch1, .m_ReadFunc = NULL },
        // 40GB appdata
        { 0x48ed459, MBR_FS_NTFS, SPD_AppData, .m_ReadFunc = NULL },
        // 16GB original0
        { 0x20014aa, MBR_FS_FAT16, SPD_Original0, .m_ReadFunc = &ReadFunc_Original0, .m_WriteFunc = &WriteFunc_Original0 },
        { 0 },
    },
};
// 30GB SSD used by all other RingEdge and RingEdge2 games
physical_disk_t SSD_30GB = {
    .m_SerialNumber = 0x4d696365,

    .m_BusType = BusTypeAta,
    .m_BootPartition = 1,
    .m_HasSegaboot = TRUE,
    .m_BlockSize = BLOCK_SIZE_HDD,
    // Number of physical sectors. 29.9902 GiB
    // TODO: Recapture these images! My numbers miss-match with Nibs', and neither decrypt
    // My size: 62894080     (lack 395: Failed reading the last chunk of sectors and gave up)
    // Nibs's size: 62894471 (lack 4  : Ignored locked sectors)
    // The following number is theorhetical
    .m_TotalSize = 62894475,  // = (sum of parts) + (n * 63)
    .m_DiskType = DiskType_HardDisk,
    .m_IsFormatted = true,
    .m_Partitions = {
        // 1GB boot partition
        { .m_Size = 0x201c84, .m_Filesystem = MBR_FS_NTFS },
        // 1GB recovery partition
        { .m_Size = 0x201cc3, .m_Filesystem = MBR_FS_NTFS },
    },
    .m_Extended = {
        // 512MB OS update
        { 0x102d83, MBR_FS_FAT16, SPD_OS, .m_ReadFunc = &ReadFunc_OSLogFiles, .m_WriteFunc = &WriteFunc_OS },
        // 2GB patch0
        { 0x403947, MBR_FS_FAT16, SPD_Patch0, .m_ReadFunc = &ReadFunc_Patch0, .m_WriteFunc = &WriteFunc_Patch0 },
        // 2GB patch1
        { 0x403947, MBR_FS_FAT16, SPD_Patch1, .m_ReadFunc = NULL },
        // 15.5GB appdata
        { 0x1eead23, MBR_FS_NTFS, SPD_AppData, .m_ReadFunc = NULL },
        // 8GB original0
        { 0x1002996, MBR_FS_FAT16, SPD_Original0, .m_ReadFunc = &ReadFunc_Original0, .m_WriteFunc = &WriteFunc_Original0 },
        { 0 },
    },
};
// 7.5GB SSD used by RingWide games (we're calling that 8GB for naming convenience)
physical_disk_t SSD_8GB = {
    .m_SerialNumber = 0x4d696365,

    .m_BusType = BusTypeAta,
    .m_BootPartition = 1,
    .m_HasSegaboot = TRUE,
    .m_BlockSize = BLOCK_SIZE_HDD,
    // Number of physical sectors. (Allocated) + (Disk slack)
    .m_TotalSize = 15711570 + 17070,
    .m_DiskType = DiskType_HardDisk,
    .m_IsFormatted = true,
    .m_Partitions = {
        // 1GB boot partition
        { .m_Size = 0x201c84, .m_Filesystem = MBR_FS_NTFS },
        // 1GB recovery partition
        { .m_Size = 0x201cc3, .m_Filesystem = MBR_FS_NTFS },
    },
    .m_Extended = {
        // 512MB OS update
        { 0x102d83, MBR_FS_FAT16, SPD_OS, .m_ReadFunc = &ReadFunc_OSLogFiles, .m_WriteFunc = &WriteFunc_OS },
        // 1GB patch0
        { 0x1f6041, MBR_FS_FAT16, SPD_Patch0, .m_ReadFunc = &ReadFunc_Patch0, .m_WriteFunc = &WriteFunc_Patch0 },
        // 1GB patch1
        { 0x1f6041, MBR_FS_FAT16, SPD_Patch1, .m_ReadFunc = NULL },
        // 337MB appdata
        { 0xa8a2c, MBR_FS_NTFS, SPD_AppData, .m_ReadFunc = NULL },
        // 2.7GB original0
        { 0x560a60, MBR_FS_FAT16, SPD_Original0, .m_ReadFunc = &ReadFunc_Original0, .m_WriteFunc = &WriteFunc_Original0 },
        { 0 },
    },
};

/* ================ EXTRA HDD ================ */
// TODO: Get real sizes for this
physical_disk_t APM_HDD = {
    .m_BusType = BusTypeUsb,
    .m_HasSegaboot = false,
    .m_BlockSize = BLOCK_SIZE_HDD,
    .m_TotalSize = 64 * 1024 * 1024 * (1024 / BLOCK_SIZE_HDD),
    .m_DiskType = DiskType_HardDisk,
    .m_IsFormatted = true,
    .m_Partitions = {
        // ~64GB big block
        {
            .m_Size = 0x801eb80,
            .m_Filesystem = MBR_FS_NTFS,
            .m_Volume = {
                .m_Name = "APM",
                .m_MountPoint = 'I',  // the APM loader mounts this for us
            }
        },
    },
    .m_Extended = {{ 0 }},
};

/* ================ ASSORTED USBS ================ */
physical_disk_t UPDATE_USB = {
    .m_BusType = BusTypeUsb,
    .m_VID = "13FE",
    .m_PID = "4200",
    .m_BlockSize = BLOCK_SIZE_FLASH,
    .m_TotalSize = 64 * 1024 * ((1024 * 1024) / BLOCK_SIZE_FLASH),
    .m_IsFormatted = true,
    .m_DiskType = DiskType_Flash,
    .m_Partitions = {
        // 59050 MB update partitions
        {
            .m_Size = 0xe6b410,
            .m_Filesystem = MBR_FS_NTFS,
            .m_Volume = {
                .m_Name = "SEGA_INS",
            },
        },
    },
    .m_Extended = {{ 0 }},
};
physical_disk_t DOWNLOAD_USB = {
    .m_BusType = BusTypeUsb,
    .m_VID = "13FE",
    .m_PID = "4200",
    .m_BlockSize = BLOCK_SIZE_FLASH,
    .m_TotalSize = 64 * 1024 * ((1024 * 1024) / BLOCK_SIZE_FLASH),
    .m_IsFormatted = true,
    .m_DiskType = DiskType_Flash,
    .m_Partitions = {
        // 59050 MB update partitions
        {
            .m_Size = 0xe6b410,
            .m_Filesystem = MBR_FS_NTFS,
            .m_Volume = {
                .m_Name = "SEGA_DL",
            },
        },
    },
    .m_Extended = {{ 0 }},
};
physical_disk_t LOG_USB = {
    .m_BusType = BusTypeUsb,
    .m_VID = "13FE",
    .m_PID = "4200",
    .m_BlockSize = BLOCK_SIZE_FLASH,
    .m_TotalSize = 16 * 1024 * ((1024 * 1024) / BLOCK_SIZE_FLASH),
    .m_IsFormatted = true,
    .m_DiskType = DiskType_Flash,
    .m_Partitions = {
        // 16 GB partition to store logs
        {
            .m_Size = 0x400000,
            .m_Filesystem = MBR_FS_NTFS,
            .m_Volume = {
                .m_Name = "SEGA_AM_LOG",
                .m_MountPoint = 'L',
            },
        },
    },
    .m_Extended = {{ 0 }},
};
physical_disk_t MAI_USB_DONGLE = {
    .m_BusType = BusTypeUsb,
    .m_VID = "13FE",
    .m_PID = "4200",
    .m_BlockSize = BLOCK_SIZE_FLASH,
    .m_TotalSize = 16 * 1024 * ((1024 * 1024) / BLOCK_SIZE_FLASH),
    .m_IsFormatted = true,
    .m_DiskType = DiskType_Flash,
    .m_Partitions = {
        // 4 GB partition
        {
            .m_Size = 0x100000,
            .m_Filesystem = MBR_FS_NTFS,
            .m_Volume = {
                .m_Name = "USBDONGLE",
                .m_MountPoint = 'U',
            },
        },
    },
    .m_Extended = {{ 0 }},
};

/* ================ DVD DRIVES ================ */
physical_disk_t ALPHA_DVD = {
    .m_BusType = BusTypeScsi,
    .m_DeviceType = DeviceTypeCdRom,
    .m_VID = "",
    .m_PID = "",
    .m_BlockSize = BLOCK_SIZE_DVD,
    .m_TotalSize = 0,
    .m_IsFormatted = FALSE,
    .m_DiskType = DiskType_CdRom,
    .m_Partitions = { {
        .m_ReadFunc = NULL,
        .m_Volume = {
            .m_Name = "DVR5048_1_60_00",
            .m_MountPoint = 'Q',
        },
    } },
};

virtual_disk_t AVAILABLE_DISKS[] = {
    // IMPORTANT: Only one of these can be selected!
    { &SSD_60GB, "60GB Boot SSD", TRUE },
    { &SSD_30GB, "30GB Boot SSD", FALSE },
    { &SSD_8GB, "7.5GB Boot SSD", FALSE },
    // Extra HDD
    { &APM_HDD, "APM Secondary HDD", FALSE },
    // USBs
    { &UPDATE_USB, "Game update USB (SEGA_INS)", FALSE },
    { &DOWNLOAD_USB, "Game download USB (SEGA_DL)", FALSE },
    { &LOG_USB, "Debug log USB", FALSE },
    { &MAI_USB_DONGLE, "maimai bootstrap USB", FALSE },
    // DVDs
    { &ALPHA_DVD, "Alpha DVD", FALSE },

    { NULL, NULL, FALSE },
};
