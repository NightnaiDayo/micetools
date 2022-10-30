#include "_devices.h"

static BYTE read_one(com_device_t* dev) {
    while (!comdev_available(dev)) Sleep(50);
    BYTE data;
    comdev_read(dev, &data, 1);
    return data;
}

BYTE extra[0xff];

#define GetFWVersion 0x30
#define GetHWVersion 0x32
#define RadioOn 0x40
#define RadioOff 0x41
#define Poll 0x2
#define MifareSelectTag 0x43
#define Unknown1 0x44  // Present in code, not seen used
#define SetKeyBana 0x50
#define Unknown2 0x51  // Present in code, not seen used
#define ReadBlock 0x52
#define SetKeyAime 0x54
#define Authenticate 0x55
#define Unknown3 0x60  // Present in code, not seen used
#define Unknown4 0x61  // Present in code, not seen used
#define Reset 0x62
#define Unknown5 0x70  // Present in code, not seen used
#define FelicaEncap 0x71

#define LedReset 0xf5
#define LedGetInfo 0xf0
#define LedSetColour 0x81

#define FWVer "TN32MSEC003S F/W Ver1.2"
#define HWVer "TN32MSEC003S H/W Ver3.0"

DWORD WINAPI aime_bd_thread(com_device_t* dev) {
    log_warning("aime_bd", "%ls woke up", dev->com->wName);
    while (1) {
        comio_recv_head_t req;
        comio_next_req(dev, &req, extra);

        log_info("aime_bd", "(%d) %02x", req.dst, req.op);

        if (req.dst == 0x00 || req.dst == 0x01) {
            // Aime readers
            switch (req.op) {
                case Reset:
                    comio_reply(dev, &req, COMIO_STATUS_OK, 0, NULL);
                    break;
                case GetFWVersion:
                    comio_reply(dev, &req, COMIO_STATUS_OK, sizeof FWVer - 1, (LPBYTE)FWVer);
                    break;
                case GetHWVersion:
                    comio_reply(dev, &req, COMIO_STATUS_OK, sizeof HWVer - 1, (LPBYTE)HWVer);
                    break;
                case SetKeyAime:
                    log_info("aime_bd", "Aime key: %.*s", req.length, extra);
                    comio_reply(dev, &req, COMIO_STATUS_OK, 0, NULL);
                    break;
                case SetKeyBana:
                    log_info("aime_bd", "Bana key: %.*s", req.length, extra);
                    comio_reply(dev, &req, COMIO_STATUS_OK, 0, NULL);
                    break;
            }
        } else if (req.dst == 0x08 || req.dst == 0x09) {
            // LED sub-boards
            switch (req.op) {
                case LedReset:
                    comio_reply(dev, &req, COMIO_STATUS_OK, 0, NULL);
                    break;
                case LedGetInfo:
                    // TODO: I'm not sure what this actually means.
                    // 838-15084 is probably a part number
                    comio_reply(dev, &req, COMIO_STATUS_OK, 9, (BYTE*)"15084\xff\x10\x00\x12");
                    break;
                case LedSetColour:
                    // No response expected here!
                    break;
            }
        }

        Sleep(50);
    }
}

void install_aime_bd() {
    com_device_t* aime = new_com_device(2);
    com_device_thread(aime, aime_bd_thread);
}
