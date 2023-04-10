#pragma once

#include <stdint.h>
#include <varargs.h>

#include "mxkCrypt.h"
#include "../../segastructs.h"

#pragma pack(push, 1)
typedef struct {
    uint32_t m_Crc;
    uint8_t m_Unk04[12];
    uint8_t m_KeyId[16];
    AM_APPBOOT_128 m_AppBoot;
} N2KeychipId_t;
#pragma pack(pop)

#define N2_ADDR 0x30

#define N2_AUTH_KEY_SIZE 20
#define N2_NONCE_SIZE 20
#define N2_DATA_MAX 506

#define N2_FOOTER_RQU_SIZE SHA1_SUM_SIZE
#define N2_FOOTER_SIGN_SIZE (N2_NONCE_SIZE + HMAC_SUM_SIZE)

typedef char N2Nonce_t[N2_NONCE_SIZE];
typedef char N2AuthKey_t[N2_AUTH_KEY_SIZE];
typedef char (*PN2Nonce_t)[N2_NONCE_SIZE];
typedef char (*PN2AuthKey_t)[N2_AUTH_KEY_SIZE];

typedef unsigned short N2Tag_t;
typedef unsigned short N2Command_t;
typedef unsigned char N2Byte_t;

void _randomBytes(void *buf, int nbytes);

int mxkN2UtilVCatenateData(unsigned char *out, size_t *count, unsigned int numStreams,
                           va_list args);
void mxkN2UtilCatenateData(unsigned char *concatinated, size_t *count, unsigned int numStreams,
                           ...);

void mxkN2UtilSha1Many(PSha1Sum_t sum, int numStreams, ...);
int mxkN2UtilHmacPacket(PSha1Sum_t packetSha, PHmacSum_t hmacSalt, PN2Nonce_t nonce, PHmacKey_t key,
                        PHmacSum_t computedHash);

int mxkN2CmdWriteCommand(PN2Nonce_t nonce, PHmacSum_t hmacSalt, N2Tag_t tag, N2Command_t command,
                         PHmacKey_t hmacKey, unsigned char addr, N2Byte_t *data,
                         unsigned short nbytes);

int mxkN2CmdEnableSession(void);
int mxkN2CmdSetAuthKey(PN2AuthKey_t authKey);

int mxkN2CmdInit(void);
int mxkN2CmdSetDeviceInfo(void);
int mxkN2Authentication(void);
int mxkN2CmdReadKeychipID(N2KeychipId_t *lpKeychipId);

int mxkN2Init(void);
void mxkN2Exit(void);
