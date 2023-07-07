/**
 * Commands in:
 *
 * - Length prefix (ie 2)
 * - Command (ie "C2" or "Z8")
 * - 0x03
 *
 */

#include <stdio.h>

#include "_devices.h"

static DWORD WINAPI ser_gacchu_guts_card(com_device_t* com) {
    puts("GG CARD");
    BYTE nBytes;
    BYTE buffer[256];
    BYTE tail;
    while (1) {
        comdev_read_blocking(com, &nBytes, 1);
        comdev_read_blocking(com, buffer, nBytes);
        comdev_read_blocking(com, &tail, 1);

        printf("Got Gacchu command: %.*s\n", nBytes, buffer);
        BYTE response[1];
        response[0] = '\0';
        // comdev_write(com, response, 1);
    }
}

void install_gacchu_guts_card() { register_device("gacchu_guts_card", ser_gacchu_guts_card); }
