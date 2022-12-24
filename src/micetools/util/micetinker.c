#include <Windows.h>
#include <setupapi.h>
#include <shellapi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#pragma comment(lib, "Setupapi.lib")

#include "../lib/am/amEeprom.h"

BYTE get_region() {
    HANDLE mxsmbus = amEepromCreateDeviceFile(&MXSMBUS_GUID, NULL, 0);
    if (mxsmbus == INVALID_HANDLE_VALUE) return FALSE;

    BYTE data[0x20];
    amEepromReadBlock(mxsmbus, 0, sizeof data, data);

    CloseHandle(mxsmbus);

    return data[12];
}

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
        BYTE region = get_region();
        printf("Current region: %02x\n", region);
        return 0;
    }
    BYTE region;

    if (argv[1][0] <= '9')
        region |= (argv[1][0] - '0') << 4;
    else if (argv[1][0] <= 'A')
        region |= (argv[1][0] - 'A' + 10) << 4;
    else
        region |= (argv[1][0] - 'a' + 10) << 4;

    if (argv[1][1] <= '9')
        region |= argv[1][1] - '0';
    else if (argv[1][1] <= 'F')
        region |= argv[1][1] - 'A' + 10;
    else
        region |= argv[1][1] - 'a' + 10;

    printf("Changing to region: %02x\n", region);

    if (!change_region(region)) {
        printf("Failed to change region: %03x", GetLastError());
    } else {
        printf("Region changed!");
    }
    return 0;
}
