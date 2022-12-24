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

#define CardType_Mifare 0x10
#define CardType_FeliCa 0x20

#pragma pack(1)
typedef struct NFCMifare {
    BYTE type;
    BYTE id_len;
    DWORD uid;
} NFCMifare_t;

#pragma pack(1)
typedef struct NFCFelica {
    BYTE type;
    BYTE id_len;
    uint64_t IDm;
    uint64_t PMm;
} NFCFelica_t;

#pragma pack(1)
typedef struct MifareBlock {
    BYTE bytes[16];
} MifareBlock_t;

#pragma pack(1)
typedef struct MifareSector {
    MifareBlock_t blocks[4];
} MifareSector_t;

#pragma pack(1)
typedef struct MifareMemory_t {
    MifareSector_t sectors[16];
} MifareMemory_t;

#pragma pack(1)
typedef struct FelicaBlock {
    BYTE bytes[16];
} FelicaBlock_t;

#pragma pack(1)
typedef struct FelicaMemory {
    FelicaBlock_t dataBlocks[15];
    FelicaBlock_t systemBlocks[9];
} FelicaMemory_t;

MifareMemory_t mifareMemory;
FelicaMemory_t felicaMemory;
BYTE luid[10] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x23, 0x45, 0x67, 0x89 };
#define PMm_VALUE 0x00F1000000014300ULL

#define FelicaSystemBlock_RC 0x00
#define FelicaSystemBlock_MAC 0x01
#define FelicaSystemBlock_ID 0x02
#define FelicaSystemBlock_D_ID 0x03
#define FelicaSystemBlock_SER_C 0x04
#define FelicaSystemBlock_SYS_C 0x05
#define FelicaSystemBlock_CKV 0x06
#define FelicaSystemBlock_CK 0x07
#define FelicaSystemBlock_MC 0x08

void populate_felica(NFCFelica_t* card) {
    card->type = CardType_FeliCa;
    card->id_len = sizeof(card->IDm) + sizeof(card->PMm);
    card->IDm = _byteswap_uint64(0x012E4CD8A30A39B3ULL);
    card->PMm = _byteswap_uint64(PMm_VALUE);

    // Key name
    felicaMemory.dataBlocks[0x0D].bytes[0] = 0x00;
    felicaMemory.dataBlocks[0x0D].bytes[1] = 0x02;
    felicaMemory.dataBlocks[0x0D].bytes[2] = 'N';
    felicaMemory.dataBlocks[0x0D].bytes[3] = 'B';
    felicaMemory.dataBlocks[0x0D].bytes[4] = 'G';
    felicaMemory.dataBlocks[0x0D].bytes[5] = 'I';
    felicaMemory.dataBlocks[0x0D].bytes[6] = 'C';
    felicaMemory.dataBlocks[0x0D].bytes[7] = '0';
    // Setup the fake blowfish data
    felicaMemory.dataBlocks[0x0D].bytes[8] = 0x89;
    felicaMemory.dataBlocks[0x0D].bytes[9] = 0x00;
    felicaMemory.dataBlocks[0x0D].bytes[10] = 0x00;
    felicaMemory.dataBlocks[0x0D].bytes[11] = 0x00;
    felicaMemory.dataBlocks[0x0D].bytes[12] = 0x00;
    felicaMemory.dataBlocks[0x0D].bytes[13] = 0x00;
    felicaMemory.dataBlocks[0x0D].bytes[14] = 0x00;
    felicaMemory.dataBlocks[0x0D].bytes[15] = 0x00;

    BYTE block0[16] = {
        0xC2, 0x1C, 0xCB, 0xC7, 0x58, 0xCA, 0x81, 0xB7,
        0xC0, 0x0B, 0x8E, 0x3A, 0x45, 0x43, 0xFE, 0xFC,
    };
    memcpy(felicaMemory.dataBlocks[0].bytes, block0, 16);
    memset(felicaMemory.dataBlocks[0x0e].bytes, 0xFF, 16);
    BYTE blockID[16] = {
        // IDd (=IDm)
        0x01, 0x2E, 0x4C, 0xD8, 0xA3, 0x0A, 0x39, 0xB3,
        // ID
        0x00, 0x2a, 0x05, 0x73, 0x02, 0x01, 0x03, 0x00,
        // ^DFC^    ^~~~~~ arbitary value
    };
    memcpy(felicaMemory.systemBlocks[FelicaSystemBlock_ID].bytes, blockID, 16);
    BYTE blockDID[16] = {
        // IDd (=IDm)
        0x01,
        0x2E,
        0x4C,
        0xD8,
        0xA3,
        0x0A,
        0x39,
        0xB3,
        // PMm
        0x00,
        0xF1,
        0x00,
        0x00,
        0x00,
        0x01,
        0x43,
        0x00,
    };
    memcpy(felicaMemory.systemBlocks[FelicaSystemBlock_D_ID].bytes, blockDID, 16);
}
void populate_mifare(NFCMifare_t* card) {
    card->type = CardType_Mifare;
    card->id_len = sizeof(card->uid);
    card->uid = _byteswap_ulong(0x01020304);

    // TODO: Better state haha
    // Flash the card memory
    for (BYTE i = 0; i < 10; i++) {
        BYTE b = luid[i];
        mifareMemory.sectors[0].blocks[2].bytes[i + 6] = b;
        mifareMemory.sectors[0].blocks[1].bytes[i + 6] = b;
    }
}

void nfc_poll(com_device_t* dev, comio_recv_head_t* req) {
    BYTE data[256];
    BYTE nbytes = 1;

    // felica present
    if (GetAsyncKeyState('L') < 0) {
        NFCFelica_t card;
        populate_felica(&card);
        memcpy(data + nbytes, &card, sizeof card);
        nbytes += sizeof card;
        data[0]++;
    }
    // mifare (aime, bana) present
    if (GetAsyncKeyState('P') < 0) {
        NFCMifare_t card;
        populate_mifare(&card);
        memcpy(data + nbytes, &card, sizeof card);
        nbytes += sizeof card;
        data[0]++;
    }

    comio_reply(dev, req, COMIO_STATUS_OK, nbytes, data);
}

DWORD WINAPI aime_bd_thread(com_device_t* dev) {
    log_info("aime_bd", "%ls woke up", dev->com->wName);
    bool radio = false;

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
                case RadioOn:
                    radio = true;
                    comio_reply(dev, &req, COMIO_STATUS_OK, 0, NULL);
                    break;
                case RadioOff:
                    radio = false;
                    comio_reply(dev, &req, COMIO_STATUS_OK, 0, NULL);
                    break;
                case Poll:
                    nfc_poll(dev, &req);
                    break;

                case 0x44:
                case 0x51:
                case MifareSelectTag:
                case Authenticate:
                    comio_reply(dev, &req, COMIO_STATUS_OK, 0, NULL);
                    break;

                // TODO: These
                case ReadBlock:
                    comio_reply(dev, &req, COMIO_STATUS_NG, 0, NULL);
                    break;
                case FelicaEncap:
                    comio_reply(dev, &req, COMIO_STATUS_NG, 0, NULL);
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
                    log_misc("nfc", "Set LED: #%02x%02x%02x", extra[0], extra[1], extra[2]);
                    printf("\033[48;2;%d;%d;%dm                                         \033[0m",
                           extra[0], extra[1], extra[2]);
                    // No response expected here!
                    break;
            }
        }

        Sleep(50);
    }
}

void install_aime_bd() {
    char* text = MiceConfig.devices.aime_bd;
    char* copy = (char*)malloc(strlen(text) + 1);
    memcpy_s(copy, strlen(text) + 1, text, strlen(text) + 1);

    char* next_token;
    char* token = strtok_s(copy, ",", &next_token);
    while (token != NULL) {
        BYTE com_port = atoi(token) & 0xFF;
        if (com_port) com_device_thread(new_com_device(com_port), aime_bd_thread);
        token = strtok_s(NULL, ",", &next_token);
    }

    free(copy);
}
