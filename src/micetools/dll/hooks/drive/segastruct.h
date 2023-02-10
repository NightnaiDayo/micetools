#pragma once
#include <stdint.h>

#define SPD_VERSION 1
#define SBR_VERSION 1

// Block count offsets from the extended partition header
#define SPD_OFFSET 1
#define SBR0_OFFSET 2
#define SBR1_OFFSET 3

// Sega Partition Descriptor
#pragma pack(push, 1)
enum spd_slot {
    SPD_Original0 = 0x10,
    SPD_Original1 = 0x11,
    SPD_Patch0 = 0x20,
    SPD_Patch1 = 0x21,
    SPD_OS = 0x30,
    SPD_AppData = 0x40,
};
typedef uint8_t spd_slot_t;

typedef struct spd {
    uint32_t crc;
    uint8_t version;
    uint8_t _[11];
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

// Sega Boot Record
typedef struct {
    uint16_t year;
    uint8_t mon;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t _;
} slot_time_t;

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

enum {
    Slot_Check = 0x00,     // status=error
    Slot_Install = 0x01,   // status=install -> FAILED TO READ PREMADE BLOCK!!
    Slot_Complete = 0x02,  // status=complete
    Slot_Empty = 0x03,     // status=error
    Slot_Error = 0x04,     // status=error
    Slot_Invalid = 0xff,
};

typedef uint8_t slot_status_t;
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
#pragma pack(pop)
