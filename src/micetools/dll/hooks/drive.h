#pragma once
#include "common.h"

void hook_drives();

// MBR
#define MBR_FLAG_NONE 0x00
#define MBR_FLAG_BOOTABLE 0x80

#define MBR_FS_NONE 0x00
#define MBR_FS_EXT_CHS 0x05
#define MBR_FS_FAT16 0x06
#define MBR_FS_NTFS 0x07
#define MBR_FS_EXT_LBA 0x0F

#pragma pack(1)
typedef struct mbr {
    BYTE bootstrap_code[446];
#pragma pack(1)
    struct {
        BYTE status;
        BYTE start_chs[3];
        BYTE type;
        BYTE end_chs[3];
        DWORD lba;
        DWORD sectors;
    } partitions[4];
    BYTE sig[2];
} mbr_t;

// SEGA
#define SPD_VERSION 1
#define SBR_VERSION 1

enum spd_slot {
    SPD_Original0 = 0x10,
    SPD_Original1 = 0x11,
    SPD_Patch0 = 0x20,
    SPD_Patch1 = 0x21,
    SPD_OS = 0x30,
    SPD_AppData = 0x40,
};
typedef uint8_t spd_slot_t;