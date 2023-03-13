#pragma once

#include <stdbool.h>

#define MXK_BLOCK_SIZE 16

// TODO: ??
extern unsigned int KEYCHIP_STATUS;
extern unsigned char KEYCHIP_ERROR;

enum {
    SetKeyS = 0,          // mxkPacketReqSetKeyS            [0] [...
    SetKeyR = 1,          // mxkPacketReqSetKeyR            [1] [...
    SetIV = 2,            // mxkPacketReqSetIv              [2] [...
    Decrypt = 3,          // mxkPacketReqDecrypt            [3] [...
    Encrypt = 4,          // mxkPacketReqEncrypt            [4] [...
    GetAppBootInfo = 5,   // mxkPacketReqGetAppBootInfo     [5] [0] [...
    EepromWrite = 6,      // mxkPacketReqEepromWrite        [6] [x] [...
    EepromRead = 7,       // mxkPacketReqEepromRead         [7] [x] [...
    NvramWrite = 8,       // mxkPacketReqNvramWrite         [8] [x] [x] [y] [...
    NvramRead = 9,        // mxkPacketReqNvramRead          [9] [x] [x] [y] [...
    AddPlayCount = 10,    // mxkPacketReqAddPlayCount       [10] [...
    FlashRead = 11,       // mxkPacketReqFlashRead          [11] [x] [x] [x] [y] [y] [y] [...
    FlashErase = 12,      // mxkPacketReqFlashErase         [12] [x] [x] [x] [...
    KcCmd13 = 13,         // mxkPacketReq-13                [13] [x] [x] [x] [...
    FlashWrite = 14,      // mxkPacketReqFlashWrite         [14] [x] [x] [x] [y] [y] [y] [...
    KcCmd15 = 15,         //
    KcCmd16 = 16,         //
    KcCmd17 = 17,         //
    KcCmd18 = 18,         //
    KcCmd19 = 19,         //
    KcGetVersion = 20,    // mxkPacketReqGetVersion         [20] [...
    SetMainId = 21,       // mxkPacketReqSetMainId          [21] [...
    GetMainId = 22,       // mxkPacketReqGetMainId          [22] [...
    SetKeyId = 23,        // mxkPacketReqSetKeyId           [23] [...
    GetKeyId = 24,        // mxkPacketReqGetKeyId           [24] [...
    GetPlayCounter = 25,  // mxkPacketReqGetPlayCounter     [25] [...
};

typedef enum {
    MXK_CACHE_USE = 0,
    MXK_CACHE_BUST = 1,
} MXK_CACHE;

typedef enum {
    MXK_STATUS_INVALID_PARAM_1 = -1,

    MXK_STATUS_OK = 0,
    MXK_STATUS_ERROR = 1,
    MXK_STATUS_NO_INIT = 2,
    MXK_STATUS_INVALID_PARAM = 3,
    // 5?
    MXK_STATUS_CRYPT_NG = 4,
    MXK_STATUS_SEND_NG = 6,
    MXK_STATUS_RECV_NG = 7,
} MXK_STATUS;

typedef enum {
    MXK_TRANSPORT_STATUS_OK = 0,
    MXK_TRANSPORT_STATUS_BUSY = 1,

    MXK_TRANSPORT_STATUS_NG = -1,
    // -3?
    MXK_TRANSPORT_STATUS_NO_INIT = -2,
    MXK_TRANSPORT_STATUS_SEND_NG = -4,
    MXK_TRANSPORT_STATUS_RECV_NG = -5,
} MXK_TRANSPORT_STATUS;

typedef enum {
    PARALLEL_STATUS_IRQ = 0x04,
    PARALLEL_STATUS_ERROR = 0x08,
    PARALLEL_STATUS_SELECT_IN = 0x10,
    PARALLEL_STATUS_PAPER_OUT = 0x20,
    PARALLEL_STATUS_ACK = 0x40,
    PARALLEL_STATUS_BUSY = 0x80,
} PARALLEL_STATUS;

typedef enum {
    PARALLEL_CONTROL_STROBE = 0x01,
    PARALLEL_CONTROL_AUTO_LF = 0x02,
    PARALLEL_CONTROL_INITIALISE = 0x04,
    PARALLEL_CONTROL_SELECT = 0x08,
    PARALLEL_CONTROL_IRQACK = 0x10,
    PARALLEL_CONTROL_BIDI = 0x20,
} PARALLEL_CONTROL;

typedef enum {
    PARALLEL_EC_FIFO_EMPTY = 0x01,
    PARALLEL_EC_FIFO_FULL = 0x02,
    PARALLEL_EC_ECP_SERVICE = 0x04,
    PARALLEL_EC_DMA_ENABLE = 0x08,
    PARALLEL_EC_ECP_INTERRUPT = 0x10,
} PARALLEL_EXTENDED_CONTROL;
typedef enum {
    PARALLEL_EC_MODE_STANDARD = 0x00,
    PARALLEL_EC_MODE_BYTE,
    PARALLEL_EC_MODE_PARALLEL_FIFO,
    PARALLEL_EC_MODE_ECP_FIFO,
    PARALLEL_EC_MODE_EPP,
    PARALLEL_EC_MODE_RESERVED,
    PARALLEL_EC_MODE_FIFO_TEST,
    PARALLEL_EC_MODE_CONFIGURATION,
} PARALLEL_EXTENDED_CONTROL_MODE;

bool mxkValidString(const char* string, unsigned int length);
