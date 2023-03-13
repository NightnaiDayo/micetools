#include "_devices.h"

static BYTE read_one(com_device_t* dev) {
    while (!comdev_available(dev)) Sleep(50);
    BYTE data;
    comdev_read(dev, &data, 1);
    return data;
}

BYTE extra[0xff];

/**
 * 51 and 55 are guesses based on the code. Makes sense given their opcodes
 * too.
 * TODO: Validate against a real board
 */
enum TN32_Opcode {
    TN32Op_Unknown20 = 0x20,  // where did this come from??
    TN32Op_GetFWVersion = 0x30,
    TN32Op_GetHWVersion = 0x32,

    TN32Op_RadioOn = 0x40,
    TN32Op_RadioOff = 0x41,
    TN32Op_Poll = 0x42,
    TN32Op_MifareSelectTag = 0x43,
    TN32Op_Unknown44 = 0x44,  // Present in code, not seen used

    TN32Op_SetKeyBana = 0x50,
    TN32Op_GetKeyBana = 0x51,
    TN32Op_ReadBlock = 0x52,
    TN32Op_Unknown53 = 0x53,  // Present in code, not seen used
    TN32Op_SetKeyAime = 0x54,
    TN32Op_GetKeyAime = 0x55,

    // Firmware update (I think)
    TN32Op_Unknown60 = 0x60,  // Present in code, not seen used
    TN32Op_Unknown61 = 0x61,  // Present in code, not seen used
    TN32Op_Reset = 0x62,
    TN32Op_Unknown63 = 0x63,
    TN32Op_Unknown64 = 0x64,

    TN32Op_Unknown70 = 0x70,  // Present in code, not seen used
    TN32Op_FelicaEncap = 0x71,

    // No responses to either
    LedUnknown80 = 0x80,
    LedSetColour = 0x81,

    LedGetInfo = 0xf0,
    LedF2 = 0xf2,
    LedF3 = 0xf3,
    LedF4 = 0xf4,
    LedReset = 0xf5,
};

char* OpcodeNames[256];

const char FWVer[] = "TN32MSEC003S F/W Ver1.2";
const char HWVer[] = "TN32MSEC003S H/W Ver3.0";

#define CardType_Mifare 0x10
#define CardType_FeliCa 0x20

#pragma pack(push, 1)
typedef struct NFCMifare {
    BYTE type;
    BYTE id_len;
    DWORD uid;
} NFCMifare_t;

typedef struct NFCFelica {
    BYTE type;
    BYTE id_len;
    uint64_t IDm;
    uint64_t PMm;
} NFCFelica_t;

typedef struct MifareBlock {
    BYTE bytes[16];
} MifareBlock_t;

typedef struct MifareSector {
    MifareBlock_t blocks[4];
} MifareSector_t;

typedef struct MifareMemory_t {
    MifareSector_t sectors[16];
} MifareMemory_t;

typedef struct FelicaBlock {
    BYTE bytes[16];
} FelicaBlock_t;

typedef struct FelicaMemory {
    FelicaBlock_t dataBlocks[15];
    FelicaBlock_t systemBlocks[9];
} FelicaMemory_t;
#pragma pack(pop)

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

BYTE AIME_KEY[6];
BYTE BANA_KEY[6];

DWORD WINAPI aime_bd_thread(com_device_t* dev) {
    static int fwNumBytes = 0;

    log_info(plfAime, "%ls woke up", dev->com->wName);
    bool radio = false;

    while (1) {
        comio_recv_head_t req;
        unsigned char sum = comio_next_req(dev, &req, extra);

        log_info(plfAime, "(%d) %02x(%d) = %s", req.dst, req.op, req.length, OpcodeNames[req.op]);

        if (req.dst == 0x00 || req.dst == 0x01) {
            // Aime readers
            switch (req.op) {
                case TN32Op_Reset:
                    comio_reply(dev, &req, COMIO_STATUS_OK, 0, NULL);
                    break;
                case TN32Op_GetFWVersion:
                    // BYTE fver = 0x10;
                    // comio_reply(dev, &req, COMIO_STATUS_OK, 1, &fver);
                    comio_reply(dev, &req, COMIO_STATUS_OK, sizeof FWVer - 1, (LPBYTE)FWVer);
                    break;
                case TN32Op_GetHWVersion:
                    // BYTE hver = 0x10;
                    // comio_reply(dev, &req, COMIO_STATUS_OK, 1, &hver);
                    comio_reply(dev, &req, COMIO_STATUS_OK, sizeof HWVer - 1, (LPBYTE)HWVer);
                    break;
                case TN32Op_SetKeyAime:
                    if (req.length != sizeof AIME_KEY) {
                        comio_reply(dev, &req, COMIO_STATUS_NG, 0, NULL);
                    } else {
                        memcpy(AIME_KEY, extra, sizeof AIME_KEY);
                        comio_reply(dev, &req, COMIO_STATUS_OK, 0, NULL);

                        log_info(plfAime, "Aime key: %02x %02x %02x %02x %02x %02x", AIME_KEY[0],
                                 AIME_KEY[1], AIME_KEY[2], AIME_KEY[3], AIME_KEY[4], AIME_KEY[5]);
                    }
                    break;
                case TN32Op_SetKeyBana:
                    if (req.length != sizeof BANA_KEY) {
                        comio_reply(dev, &req, COMIO_STATUS_NG, 0, NULL);
                    } else {
                        memcpy(BANA_KEY, extra, sizeof BANA_KEY);
                        comio_reply(dev, &req, COMIO_STATUS_OK, 0, NULL);

                        log_info(plfAime, "Bana key: %02x %02x %02x %02x %02x %02x", BANA_KEY[0],
                                 BANA_KEY[1], BANA_KEY[2], BANA_KEY[3], BANA_KEY[4], BANA_KEY[5]);
                    }
                    break;
                case TN32Op_RadioOn:
                    radio = true;
                    comio_reply(dev, &req, COMIO_STATUS_OK, 0, NULL);
                    break;
                case TN32Op_RadioOff:
                    radio = false;
                    comio_reply(dev, &req, COMIO_STATUS_OK, 0, NULL);
                    break;
                case TN32Op_Poll:
                    nfc_poll(dev, &req);
                    break;

                case TN32Op_GetKeyBana:
                case TN32Op_GetKeyAime:
                    if (req.length != 5) {
                        //
                    }

                case TN32Op_Unknown44:
                case TN32Op_MifareSelectTag:
                    comio_reply(dev, &req, COMIO_STATUS_OK, 0, NULL);
                    break;

                case TN32Op_Unknown60:
                    // req.length == 0; start firmware update?
                    comio_reply(dev, &req, COMIO_STATUS_OK, 0, NULL);
                    break;
                case TN32Op_Unknown61:
                    // null-terminated line of the firmware hex!
                    comio_reply(dev, &req, COMIO_STATUS_OK, 0, NULL);
                    log_info(plfAime, "Recv firmware: %s", extra);
                    break;
                case TN32Op_Unknown63:
                    // req.length == 0; start binary firmware update?
                    fwNumBytes = 0;
                    comio_reply(dev, &req, COMIO_STATUS_OK, 0, NULL);
                    break;
                case TN32Op_Unknown64:
                    // Raw binary firmware data
                    // [req.length == 256 -> wraps to 00] [256 bytes]
                    if (req.length == 0) {
                        unsigned char update_buffer[256];
                        update_buffer[0] = sum;
                        comio_read(dev, update_buffer + 1, 255);
                        // We need to account for the actual checksum at the end of this lot
                        comio_read(dev, &sum, 1);

                        fwNumBytes += 256;
                    } else {
                        fwNumBytes += req.length;
                    }

                    // nxAuth is segfaulting past this. Rather than figure it out, let's just
                    // protect it from itself for now.
                    if (fwNumBytes >= 0x3000)
                        ;
                    else
                        comio_reply(dev, &req, COMIO_STATUS_OK, 0, NULL);
                    break;

                // TODO: These
                case TN32Op_ReadBlock:
                    comio_reply(dev, &req, COMIO_STATUS_NG, 0, NULL);
                    break;
                case TN32Op_FelicaEncap:
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
                    log_misc(plfAime, "Set LED: #%02x%02x%02x", extra[0], extra[1], extra[2]);
                    printf(
                        "\033[48;2;%d;%d;%dm                                 "
                        "        \033[0m",
                        extra[0], extra[1], extra[2]);
                    // No response expected here!
                    break;
            }
        }

        Sleep(50);
    }
}

void install_aime_bd() {
    ZeroMemory(OpcodeNames, sizeof OpcodeNames);
    OpcodeNames[TN32Op_Unknown20] = "Unknown20";
    OpcodeNames[TN32Op_GetFWVersion] = "GetFWVersion";
    OpcodeNames[TN32Op_GetHWVersion] = "GetHWVersion";
    OpcodeNames[TN32Op_RadioOn] = "RadioOn";
    OpcodeNames[TN32Op_RadioOff] = "RadioOff";
    OpcodeNames[TN32Op_Poll] = "Poll";
    OpcodeNames[TN32Op_MifareSelectTag] = "MifareSelectTag";
    OpcodeNames[TN32Op_Unknown44] = "Unknown44";
    OpcodeNames[TN32Op_SetKeyBana] = "SetKeyBana";
    OpcodeNames[TN32Op_GetKeyBana] = "GetKeyBana";
    OpcodeNames[TN32Op_ReadBlock] = "ReadBlock";
    OpcodeNames[TN32Op_Unknown53] = "Unknown53";
    OpcodeNames[TN32Op_SetKeyAime] = "SetKeyAime";
    OpcodeNames[TN32Op_GetKeyAime] = "GetKeyAime";
    OpcodeNames[TN32Op_Unknown60] = "Unknown60";
    OpcodeNames[TN32Op_Unknown61] = "Unknown61";
    OpcodeNames[TN32Op_Reset] = "Reset";
    OpcodeNames[TN32Op_Unknown70] = "Unknown70";
    OpcodeNames[TN32Op_FelicaEncap] = "FelicaEncap";

    OpcodeNames[LedReset] = "LedReset";
    OpcodeNames[LedGetInfo] = "LedGetInfo";
    OpcodeNames[LedSetColour] = "LedSetColour";

    register_device("aime_tn32msec", aime_bd_thread);
}
