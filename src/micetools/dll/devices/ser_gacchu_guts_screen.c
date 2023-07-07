#include <stdio.h>

#include "_devices.h"

/*
Packet format:
    String packets:
        [0]: commandNum > 16 ? 0x81 : 0x01
        [...]: string. does not contain null.
        [n]: commandNum > 16 ? 0x8d : 0x0d
    Binary packets:
        [0]: 0x55
        [1]:
        [2]:
        [3]:
        [4]:
        [5]:
        [6]:
        [7]: Sum of ([1] through [6]) + 0xAA

Responses:
    Type(0) packets:
        [0]: 0x80 | (if 0x40 set, arg2 = 0x80, else arg2 = 0)
        [1]: arg0 & 0x7f
        [2]: (arg0 >> 7) & 0x7f
        [3]: arg1 & 0x7f
        [4]: (arg1 >> 7) & 0x7f

    Type(2) packets:
        [0]: 0x40 | (if 0x02 set, arg2 = 0, else arg2 = 0x80)
        [1]: arg0 & 0x3f
        [2]: (arg0 >> 6) & 0x3f
        [3]: arg1 & 0x3f
        [4]: (arg1 >> 6) & 0x3f
        [5]: ???

    Binary packets:
        // String packets (this seems.. like an impossible contradiction!?):
        //     [0]: 0x01
        //     [1]: 0x0d

        // [0]: 0x55
        // [1]: 0x54
        // [...?]      -> Buffer length reset to 2. Do these matter?
        // [9] 0x55
        // [10] 0x54

        // --- OR ---

        [0]: 0x55
        [1]: 0x54
        [2]: if low two bits set (& 3), [7] contains arg2, else arg2=0
        [3]: arg0 & 0xff
        [4]: arg0 >> 8
        [5]: arg1 & 0xff
        [6]: arg1 >> 8
        [7]: arg2
        [8]:
        [9]: Sum of ([1] through [8]) + 0xAA

        --- OR ---
        Whatever (005c1edb) is doing
        From the looks of it, 0x41 is some sort of continuation byte,
        every 10 bytes.
*/

/*

FQF[02x: arg0]                              = 11 (set frequency?)
FQF00                                       = 12 (get frequency?)
.5A[02x: touch panel effect][02x: ??]       = 16 (set config?)

*/

/*

First command received:
    = Screen command 8
    []Z[]
    01
        5A
    0D

Second command sent:
    = Screen command 16
    = ".5A0001"
    But the last four digits are replaced with "%02X%02X"

    [].5AFF00[]
    01
        2E 35 41 46 46 30 30
    0D

*/

static inline void response_ok(com_device_t* lpDev, char* szResponse) {
    int nBytes = szResponse ? strlen(szResponse) : 0;
    if (nBytes > 8) nBytes = 8;

    BYTE buffer[10];
    buffer[0] = 0x01;
    if (nBytes) memcpy(&buffer[1], szResponse, nBytes);
    buffer[nBytes + 1] = 0x0d;
    comdev_write(lpDev, buffer, (nBytes + 2) & 0xffff);
}

static inline void response_two_short(com_device_t* lpDev, SHORT arg0, SHORT arg1) {
    BYTE buffer[5];
    buffer[0] = 0x80;
    buffer[1] = arg0 & 0x7f;
    buffer[2] = (arg0 >> 7) & 0x7f;
    buffer[3] = arg1 & 0x7f;
    buffer[4] = (arg1 >> 7) & 0x7f;
    comdev_write(lpDev, buffer, 5);
}

static BYTE command_do[4] = {
    0xd0,
    0xce,
    0xb8,
    0xb1,
};

static DWORD WINAPI ser_gacchu_guts_screen(com_device_t* dev) {
    BYTE bHead;
    BYTE buffer[256];
    puts("Gacchu screen start");
    while (1) {
        comdev_read_blocking(dev, &bHead, 1);
        if (bHead == 0x55) {
            log_error(plfGacchuGuts, "Screen binary!");
            comdev_read_blocking(dev, buffer, 7);
            continue;
        }
        if (bHead != 0x01 && bHead != 0x81) {
            log_error(plfGacchuGuts, "Screen babble: %02x", bHead);
            continue;
        }
        WORD index = 0;
        BYTE tail = bHead == 0x01 ? 0x0D : 0x8D;
        do {
            comdev_read_blocking(dev, &buffer[index++], 1);
        } while (index < _countof(buffer) && buffer[index - 1] != tail);

        // comdev_read_blocking(dev, buffer, nBytes);
        // comdev_read_blocking(dev, &tail, 1);

        // Done and working (hopefully!)
        if (index == 2 && memcmp(buffer, "Z", 1) == 0) {
            response_ok(dev, "0");
        }
        // Not working :(
        else if (index == 6 && memcmp(buffer, "FQF", 3) == 0) {
            BYTE value = 0;
            _snscanf_s((const char*)&buffer[3], 2, "%02hhX", &value);
            if (value == 0) {
                log_game(plfGacchuGuts, "Got FQF00 command. What now...?");
                response_ok(dev, " ");
            } else {
                log_game(plfGacchuGuts, "Got FQF[%02x] command. What now...?", value);
                response_ok(dev, "0");  // Meant to do " " vs "0" I think?
            }
        }

        else if (index == 8 && memcmp(buffer, ".5A", 3) == 0) {
            log_game(plfGacchuGuts, "Got .5A command. What now...?");
            /*
            uVar9 = (arg0 * 1024) / (0x4000 - 1)
            */

            // At a guess, this is getting the display resolution.
            response_two_short(dev, 1024, 768);
        }

        else if (index == 5 && memcmp(buffer, command_do, 4) == 0) {
            log_game(plfGacchuGuts, "Got D0 command. What now...?");
            response_ok(dev, "0");  // Probably wrong
        } else {
            log_error(plfGacchuGuts, "%ls: Got Gacchu screen: %02x(%d):[%02x]%.*s\n",
                      dev->com->wName, bHead, index, buffer[0], index - 1, buffer);
            log_error(plfGacchuGuts, "%ls: Got Gacchu bytes: %02x %02x %02x %02x %02x\n",
                      dev->com->wName, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
        }
    }
}

void install_gacchu_guts_screen() { register_device("gacchu_guts_screen", ser_gacchu_guts_screen); }
