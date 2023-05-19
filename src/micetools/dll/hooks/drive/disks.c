#include "drive.h"

physical_disk_t SSD = {
    .m_SerialNumber = 0x00144DB0,

    .m_BusType = BusTypeAta,
    .m_BootPartition = 1,
    .m_HasSegaboot = TRUE,
    .m_BlockSize = BLOCK_SIZE_HDD,
    .m_TotalSize = 64 * 1024 * 1024 * (1024 / BLOCK_SIZE_HDD),
    .m_DiskType = DiskType_HardDisk,
    .m_IsFormatted = true,
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
        { 0x102d83, MBR_FS_FAT16, SPD_OS, .m_ReadFunc = &ReadFunc_OSLogFiles, .m_WriteFunc = &WriteFunc_OS },      // 512MB OS update
        { 0x403947, MBR_FS_FAT16, SPD_Patch0, .m_ReadFunc = &ReadFunc_Patch0, .m_WriteFunc = &WriteFunc_Patch0 },  // 2GB patch0
        { 0x403947, MBR_FS_FAT16, SPD_Patch1, .m_ReadFunc = NULL },  // 2GB patch1
        { 0x48ed459, MBR_FS_NTFS, SPD_AppData, .m_ReadFunc = NULL }, // 40GB something
        {
            // 16GB partition for the game
            // The real value here should be "0x20014aa,"
            0x20014aa,  // 16GB, FiNALE

            // Instead, we're going to just allocate ~8GB, the exact size of SDCQ
            // 0xeafc00,  // ~8GB. Lol. Lmao.
            MBR_FS_FAT16,
            SPD_Original0,
            .m_ReadFunc = &ReadFunc_Original0,
            .m_WriteFunc = &WriteFunc_Original0,
        },
        { 0 },
    },
};
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

physical_disk_t* PHYSICAL_DISKS[] = {
    &SSD,
    // &UPDATE_USB,
    // &DOWNLOAD_USB,
    // &APM_HDD,

    // &MAI_USB_DONGLE,

    // &LOG_USB,
    // &ALPHA_DVD,
};
