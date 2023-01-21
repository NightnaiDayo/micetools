#include <Windows.h>
#include <stdio.h>

#include "../lib/am/amEeprom.h"

#define EEPROM_SIZE 0x2000

BOOL miceDumpEEPROM() {
    amEepromDebugLevel = 1;

    fprintf(stderr, "Dumping EEPROM using %s\n", amEepromVersion);

    if (amEepromInit(NULL) != AM_EEPROM_STATUS_OK) {
        puts("amEepromInit failed");
        return FALSE;
    }

    BYTE data[0x20];
    for (WORD addr = 0; addr < EEPROM_SIZE; addr += sizeof data) {
        if (amEepromRead(addr, data, sizeof data) != AM_EEPROM_STATUS_OK) continue;
        printf("%04x: ", addr);
        for (int i = 0; i < 0x20; i++) printf("%02x ", data[i]);
        printf("  ");
        for (int i = 0; i < 0x20; i++) {
            if (' ' <= data[i] && data[i] <= '~') printf("%c", data[i]);
            else printf(".");
        }

        puts("");
    }

    amEepromExit();
    return TRUE;
}
