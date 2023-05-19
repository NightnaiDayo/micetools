#include <Windows.h>
#include <setupapi.h>
#include <shellapi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../lib/am/amEeprom.h"
#include "../lib/am/amSram.h"

BOOL wipe_eeprom() {
    if (amEepromInit(NULL) != AM_EEPROM_STATUS_OK) return FALSE;

    BYTE data[0x20];
    memset(data, 0xff, sizeof data);

    // Static = 0 and 32
    // Game data is 1000-1fff = block 128-255
    for (int i = 0; i < 256; i++) {
        if (i == 0) continue;
        if (i == 32) continue;
        printf("Wiping EEPROM block %d\n", i);
        amEepromWrite(i * 0x20, data, sizeof data);
    }

    amEepromExit();
    return TRUE;
}
BOOL wipe_sram() {
    if (amSramInit() != AM_SRAM_STATUS_OK) return FALSE;

    DWORD nbytes = amSram.m_size.LowPart;

    printf("Wiping %d bytes of SRAM\n", nbytes);
    BYTE* buffer = malloc(nbytes);
    ZeroMemory(buffer, nbytes);
    amSramWrite(buffer, 0, nbytes);
    free(buffer);

    amSramExit();
    return TRUE;
}

// void wait_for_dip() {
//     puts("Press Test or System to continue");
//     amDipswInit();
// }

int main(int argc, char** argv) {
    wipe_eeprom();
    wipe_sram();
    return 0;
}
