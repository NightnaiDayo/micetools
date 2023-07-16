#include <Windows.h>
#include <stdio.h>

/*

Cmos:
00-01: 00 21 <- "00 21" = "Boot: USB, HDD", "10 20" = "Boot HDD,USB"
02-0F: 43 65 87 a9 0b 21 43 65  87 a9 3b 93 99 54
10-19: 35 25 00 00 00 40 4f 40 50 08
1A-1B: 1a 08
1C-1D: 1a 08 <- USB flags(?). 08 = normal, 09 = disabled
1E-1F: 1a 0a
20-2F: 03 14 1d 12 28 00 00 00 00 07 00 00 00 00 00 08
30-3F: 01 80 ce 10 14 00 8a 48 06 01 d4 ff ff 1f 04 00
40-4B: 60 a0 9f 0b a0 29 47 15 7d 80 90 3f
   4C: 00 <- SEGA flags. 00 = None, 02 = Security freeze, 03 = Freeze + Certified
   4D: 9d
   4E: 00 <- 00 = Boot HDD first, 01 = Boot USB first
   4F: b4
50-5F: 25 d6 e7 8f 7f 22 72 af b5 1a 00 8f 1c 3c c0 90
   60: 00
   61: 00
62-6F: 06 37 50 8c 66 bd ac 38 7b 52 4a 04 4c dc
70-7F: 82 62 0d 2a 5d 1c 32 6e 05 af 44 c3 45 2e 08 96


*/

#include "../lib/am/amCmos.h"
#include "../lib/am/amPlatform.h"

AM_PLATFORM_CMOS_PARAM config = {
    .index = { { 0, 0 }, { 0, 0 } },
    .m_MbrCount = 0,
};

BOOL miceDumpCMOS() {
    amCmosDebugLevel = 1;

    fprintf(stderr, "Dumping RTC CMOS using %s\n", amCmosVersion);

    AM_CMOS_STATUS err = amCmosInit(&config);
    if (err != AM_CMOS_STATUS_OK) {
        fprintf(stderr, "amCmosInit failed: %d\n", err);
        amCmosExit();
        return FALSE;
    }

    for (BYTE index = 0; index < 0x80; index++) {
        BYTE data;
        amCmosReadByteInRearpart(index, &data);
        printf(" %02x", data);
    }
    puts("");

    amCmosExit();
    return TRUE;
}
