#pragma once
#include <Windows.h>

extern unsigned char KEY_R[16];
extern unsigned char KEY_S[16];
extern unsigned char BILLING_PRIVKEY[917];
extern unsigned char BILLING_PUBKEY[162];
extern unsigned char BILLING_CACERT[817];
extern unsigned char KEYCHIP_ID[16];
extern unsigned char MAIN_ID[16];

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

// Structs
#pragma pack(1)
typedef struct {
    DWORD crc;
    DWORD format;
    char game_id[4];
    BYTE region;
    BYTE model_type;
    BYTE system_flag;
    BYTE _;
    char platform_id[3];
    BYTE dvd_flag;
    DWORD network_addr;
    BYTE __[216];
    BYTE seed[16];
} appboot_t;

// Crypt
void mxkSetKeyS(unsigned char* key_s);
void mxkSetKeyR(unsigned char* key_r);
void mxkSwapKeys();

void mxkCryptEncryptData(unsigned char* ct, unsigned char* pt);
void mxkCryptDecryptData(unsigned char* ct, unsigned char* pt);

void mxkSign(void* buffer, size_t nbytes, unsigned char* signature);
void mxkSignValue(unsigned int value, unsigned char* signature);

// Transport
static BOOL mxkTransportWaitStrobeReady(HANDLE mxparallel);
static BOOL mxkTransportWaitStrobeRelease(HANDLE mxparallel);
static void mxkTransportCtrlPortInAndOut(HANDLE mxparallel, BYTE flag);
static void mxkTransportCtrlPortInOrOut(HANDLE mxparallel, BYTE flag);

BOOL mxkTransportSend(HANDLE mxparallel, unsigned char* data, DWORD nbytes);
HRESULT mxkTransportRecv(HANDLE mxparallel, unsigned char* data, DWORD nbytes);

BOOL mxkSendPacket(HANDLE mxparallel, unsigned char* packet);
BOOL mxkRecvPacket(HANDLE mxparallel, unsigned char* packet);
void mxkTransportInitPic(HANDLE mxparallel);

// MXK Packet
void mxkPacketReqSetKeyS(unsigned char* packet);
void mxkPacketReqSetKeyR(unsigned char* packet);
void mxkPacketReqGetAppBootInfo(unsigned char* packet);
void mxkPacketReqEepromRead(unsigned char* packet, unsigned char page);
void mxkPacketReqGetVersion(unsigned char* packet);
void mxkPacketReqSetMainId(unsigned char* packet);
void mxkPacketReqGetMainId(unsigned char* packet);
void mxkPacketReqGetKeyId(unsigned char* packet);
void mxkPacketReqGetPlayCounter(unsigned char* packet);
void mxkPacketReqFlashRead(unsigned char* packet, unsigned int address, unsigned int nbytes);
void mxkPacketReqNvramRead(unsigned char* packet, unsigned short addr, unsigned char blocks);

// MXK
BOOL mxkExchengeAesKey(HANDLE mxparallel);
BOOL mxkVersion(HANDLE mxparallel, unsigned short* version);
BOOL mxkSetMainId(HANDLE mxparallel, unsigned char* main_id);
BOOL mxkGetMainId(HANDLE mxparallel, const unsigned char* main_id);
BOOL mxkGetKeyId(HANDLE mxparallel, unsigned char* main_id);
BOOL mxkGetAppBootInfo(HANDLE mxparallel, appboot_t* appboot);
BOOL mxkGetPlayCounter(HANDLE mxparallel, DWORD* play_counter);
BOOL mxkFlashRead(HANDLE mxparallel, unsigned int address, unsigned int nbytes,
                  unsigned char* buffer);
BOOL mxkEepromRead(HANDLE mxparallel, unsigned char page, unsigned char* data);
BOOL mxkNvramRead(HANDLE mxparallel, unsigned short addr, unsigned char blocks,
                  unsigned char* data);
