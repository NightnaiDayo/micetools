#include <Windows.h>
#include <setupapi.h>
#include <shellapi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#pragma comment(lib, "Setupapi.lib")

#include "../lib/am/amEeprom.h"

BOOL change_region(BYTE region) {
    HANDLE mxsmbus = amEepromCreateDeviceFile(&MXSMBUS_GUID, NULL, 0);
    if (mxsmbus == INVALID_HANDLE_VALUE) return FALSE;

    BYTE data[0x20];
    if (!amEepromReadBlock(mxsmbus, 0, sizeof data, data)) {
        printf("Failed to read block 0");
        goto fail;
    }

    puts("Original content:");
    for (int i = 0; i < 0x20; i++) printf("%02x ", data[i]);
    puts("");

    data[12] = region;
    amEepromRepairChecksum(data);

    amEepromWriteBlock(mxsmbus, 0, sizeof data, data);
    amEepromReadBlock(mxsmbus, 0, sizeof data, data);

    puts("Tinkered content:");
    for (int i = 0; i < 0x20; i++) printf("%02x ", data[i]);
    puts("");

    CloseHandle(mxsmbus);
    return TRUE;
fail:
    CloseHandle(mxsmbus);
    return FALSE;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <region>\n", argv[0]);
        return -1;
    }
    BYTE region = argv[1][0] - '0';
    printf("Changing to region: %d\n", region);

    if (!change_region(region)) {
        printf("Failed to change region: %03x", GetLastError());
    } else {
        printf("Region changed!");
    }
    return 0;
}
