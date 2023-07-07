#include "../hooks/gui.h"
#include "_devices.h"

/*
[0] = e0
[1] = dest?
[2] = dst?
[3] = length
[4] = op code
[...] length-1 bytes
[.] = sum

[0] = e0
[2] = dst?
[1] = dest?
[3] = length
[4] = status
[5] = op
[6] = report
[...]
[.] = sum

OP codes:
    3c: FUN_005735c0 (1)

    10: FUN_00580c00 (1)
    7c: FUN_00580c00 (2)
    3c: FUN_00580c00 (1)
    39: FUN_00580c00 (4)
    3b: FUN_00580c00 (1)

    31: FUN_00581350 (5)
    32: FUN_005813b0 (8)
    33: FUN_00581430 (8) -> something at [16]??
    39: FUN_005814b0 (4)
    3f: FUN_00581220 (7)

    7b: FUN_005812a0 (3)
    7c: FUN_00581310 (2)

01: was this just an error?
10:

31: Set button. [button] [r] [g] [b]
            0-7 = buttons
            8 = woofer
            9 = center
32: Set multiple. [00] [idx hi] [idx lo] [r] [g] [b]
33: Fade multiple?. [09] [?] [?] [r] [g] [b]
39: Set body light. [intensity] [-] [-]
3b:
3c: Commit?
3f:

7c:
7b:

initial setup: 7c 0-7, 32, 3c, 39, 3f, 3b
*/

unsigned char COLOURS[10][3];
double positions[10][2] = {
    { 0.337963, 0.470833 }, { 0.469444, 0.619792 }, { 0.469444, 0.828125 }, { 0.337037, 0.977083 },
    { 0.152778, 0.976042 }, { 0.020370, 0.828125 }, { 0.020370, 0.618750 }, { 0.151852, 0.472917 },
    { 0.5, 0.75 },          { 0.5, 0.5 },
};

typedef struct rs232c_recv_head {
    BYTE sync;
    BYTE src;
    BYTE dst;
    BYTE length;
    BYTE op;
} rs232c_recv_head_t;

#define log_level log_misc

BYTE extra[0xff];
static DWORD WINAPI led_bd_thread(com_device_t* dev) {
    log_info(plfMaiLED, "%ls woke up", dev->com->wName);

    while (1) {
        rs232c_recv_head_t head;
        comdev_read_blocking(dev, (unsigned char*)&head, sizeof head);
        comdev_read_blocking(dev, extra, head.length);

        // log_info(plfMaiLED, "Bound %02x->%02x", head.src, head.dst);

        switch (head.op) {
            case 0x01:
                log_level(plfMaiLED, "01");
                comdev_write(dev, (unsigned char*)"\xe0\x01\x11\x03\x01\x01\x01\x18", 8);
                //                  syn dst src len sts op. rep chk
                break;

            case 0x10:
                log_level(plfMaiLED, "10");
                comdev_write(dev, (unsigned char*)"\xe0\x01\x11\x03\x01\x10\x01\x27", 8);
                //                  syn dst src len sts op. rep chk
                break;

            case 0x31:
                COLOURS[extra[0]][0] = extra[1];
                COLOURS[extra[0]][1] = extra[2];
                COLOURS[extra[0]][2] = extra[3];

                log_level(plfMaiLED, "31: %02x = (%02x %02x %02x)", extra[0], extra[1], extra[2],
                            extra[3]);
                comdev_write(dev, (unsigned char*)"\xe0\x01\x11\x03\x01\x31\x01\x48", 8);
                //                  syn dst src len sts op. rep chk
                break;
            case 0x32:
                log_level(plfMaiLED, "32: %02x %02x %02x %02x %02x %02x %02x", extra[0], extra[1],
                            extra[2], extra[3], extra[4], extra[5], extra[6], extra[7]);

                for (unsigned char i = extra[2] - 1; i < extra[1]; i++) {
                    COLOURS[i][0] = extra[3];
                    COLOURS[i][1] = extra[4];
                    COLOURS[i][2] = extra[5];
                }

                comdev_write(dev, (unsigned char*)"\xe0\x01\x11\x03\x01\x32\x01\x49", 8);
                //                  syn dst src len sts op. rep chk
                break;
            case 0x33:
                log_level(plfMaiLED, "33: %02x %02x %02x %02x %02x %02x %02x", extra[0], extra[1],
                            extra[2], extra[3], extra[4], extra[5], extra[6], extra[7]);
                comdev_write(dev, (unsigned char*)"\xe0\x01\x11\x03\x01\x33\x01\x4a", 8);
                //                  syn dst src len sts op. rep chk
                COLOURS[extra[0]][0] = extra[extra[5]];
                COLOURS[extra[0]][1] = extra[extra[6]];
                COLOURS[extra[0]][2] = extra[extra[7]];
                break;

            case 0x39:
                log_level(plfMaiLED, "39: %02x %02x %02x", extra[0], extra[1], extra[2]);
                comdev_write(dev, (unsigned char*)"\xe0\x01\x11\x03\x01\x39\x01\x50", 8);
                //                  syn dst src len sts op. rep chk

                COLOURS[9][0] = extra[0];
                COLOURS[9][1] = extra[0];
                COLOURS[9][2] = extra[0];
                break;
            case 0x3b:
                log_level(plfMaiLED, "3b");
                comdev_write(dev, (unsigned char*)"\xe0\x01\x11\x03\x01\x3b\x01\x52", 8);
                //                  syn dst src len sts op. rep chk
                break;
            case 0x3c:
                log_level(plfMaiLED, "3c (I am %ls)", dev->com->wName);
                comdev_write(dev, (unsigned char*)"\xe0\x01\x11\x03\x01\x3c\x01\x53", 8);
                //                  syn dst src len sts op. rep chk
                break;
            case 0x3f:
                log_level(plfMaiLED, "3f: %02x %02x %02x %02x %02x %02x", extra[0], extra[1],
                            extra[2], extra[3], extra[4], extra[5], extra[6]);
                comdev_write(dev, (unsigned char*)"\xe0\x01\x11\x03\x01\x3f\x01\x56", 8);
                //                  syn dst src len sts op. rep chk
                break;

            case 0x7c:
                // extra[0] goes from 0 to 7
                // Could this be some sort of calibration for the buttons?
                log_level(plfMaiLED, "7c: %02x", extra[0]);
                comdev_write(dev, (unsigned char*)"\xe0\x01\x11\x04\x01\x7c\x01\x00\x94", 9);
                // \/ causes 7b to be used
                // comdev_write(dev, (unsigned char*)"\xe0\x01\x11\x04\x01\x7c\x01\x10\xa4", 9);
                //                  syn dst src len sts op. rep --- chk
                break;
            case 0x7b:
                log_level(plfMaiLED, "7b: %02x %02x %02x", extra[0], extra[1], extra[2]);
                comdev_write(dev, (unsigned char*)"\xe0\x01\x11\x03\x01\x7b\x01\x92", 8);
                //                  syn dst src len sts op. rep chk
                break;

            default:
                log_error(plfMaiLED, "Unknown op %02x (%d)", head.op, head.length - 1);
                break;
        }
    }
}

static DWORD WINAPI led_null_thread(com_device_t* dev) {
    while (1) Sleep(10000000);
}

void __stdcall led_overlay(unsigned int hookType, IDirect3DDevice9* dev) {
    // maimai is DX9, so no point handling anything else anyway
    if (hookType != UI_HOOK_DX9) return;

    D3DDEVICE_CREATION_PARAMETERS cparams;
    RECT rect;

    dev->lpVtbl->GetCreationParameters(dev, &cparams);
    GetClientRect(cparams.hFocusWindow, &rect);

    if (GetAsyncKeyState(VK_LBUTTON) & 1) {
        POINT cursor;
        GetCursorPos(&cursor);

        // log_info("led_overlay", "x: %d, y: %d", cursor.x, cursor.y);
        // log_info("led_overlay", "x%: %f, y%: %f", (float)cursor.x / (float)rect.right,
        //          (float)(cursor.y - 26) / (float)rect.bottom);
    }

    for (unsigned char i = 0; i < 10; i++) {
        int x = (int)(rect.right * positions[i][0]);
        int y = (int)(rect.bottom * positions[i][1]);
        draw_rect(dev, x - 25, y - 25, 50, 50, COLOURS[i][0], COLOURS[i][1], COLOURS[i][2]);
    }
}

void install_led_bd() {
    register_device("mailed", led_bd_thread);

    // register_gui_hook(&led_overlay);
}
