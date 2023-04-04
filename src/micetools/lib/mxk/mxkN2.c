#include "mxkN2.h"

#include <Windows.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "../../dll/devices/smb_n2.h"
#include "../ami/ami.h"
#include "mxkCrypt.h"
#include "mxkSmbus.h"

typedef struct {
    unsigned char m_Key[16];
    unsigned char m_IV[16];
} AESKey_t;

#pragma pack(push, 1)
typedef struct {
    unsigned short m_Tag;
    unsigned short m_ParamSize;
    unsigned short m_Command;
    unsigned char m_Body[N2_DATA_MAX];
} N2Packet_t;
typedef struct {
    union {
        struct {
            Sha1Sum_t m_ShaSum;
        } rqu;
        struct {
            N2Nonce_t m_Nonce;
            HmacSum_t m_Hmac;
        } sign;
    };
} N2PacketFooter_t;
#pragma pack(pop)

BOOL MXK_HAS_INIT = FALSE;
BOOL MXK_N2_INIT = FALSE;
#define CheckN2Init                               \
    do {                                          \
        if (!MXK_N2_INIT) {                       \
            amiDebugLog("Error: Uninitialized."); \
            return -3;                            \
        }                                         \
    } while (0);
typedef struct {
    unsigned char m_Addr;
    HmacKey_t m_HmacKey;
    AESKey_t m_AESKey;
    N2AuthKey_t m_AuthKey;
    AESKey_t m_EncKey;
    HmacSum_t m_HmacSalt;
} N2DeviceInfo_t;
N2DeviceInfo_t N2DeviceInfo;

unsigned char N2_KEY_HMAC[2][20] = {
    { 0x2c, 0xf8, 0x3e, 0x5e, 0x51, 0xf0, 0x60, 0x1b, 0xf6, 0xb1,
      0x49, 0x11, 0x3a, 0xaf, 0x36, 0xe1, 0x51, 0x1c, 0x16, 0x05 },
    { 0xba, 0x15, 0x0f, 0xec, 0x79, 0x81, 0x65, 0xbe, 0x55, 0x81,
      0x1d, 0x1e, 0x1f, 0x11, 0xee, 0xb0, 0xf4, 0xd4, 0x20, 0x24 }
};

AESKey_t N2_KEY_AES[2] = {
    { { 0xd7, 0xf6, 0x86, 0xfb, 0x63, 0x44, 0xff, 0xa5, 0x60, 0x9d, 0x4e, 0xf0, 0x18, 0xe9, 0x45,
        0xb4 },
      { 0x12, 0x6c, 0xb1, 0xdb, 0x9e, 0x00, 0x8c, 0xc6, 0xae, 0xa1, 0x73, 0xec, 0xe7, 0x2f, 0x5a,
        0xc4 } },
    { { 0xa1, 0x1a, 0xc4, 0x4d, 0xcd, 0x48, 0x4f, 0xed, 0x70, 0xcc, 0x3f, 0x5d, 0x94, 0x5b, 0xbe,
        0xb3 },
      { 0x9c, 0x5c, 0xba, 0x7f, 0xb0, 0x15, 0xc7, 0x69, 0xcb, 0xb4, 0x41, 0x19, 0x97, 0x2b, 0x45,
        0x9f } },
};

int mxkN2CmdInit(void) {
    if (MXK_N2_INIT) {
        amiDebugLog("Error: Already initialized.");
        return -4;
    }
    ZeroMemory(&N2DeviceInfo, sizeof N2DeviceInfo);
    MXK_N2_INIT = TRUE;
    return 0;
}

int mxkN2CmdSetDeviceInfo(void) {
    if (MXK_HAS_INIT) return -4;

    if (mxkN2CmdInit() != 0) return -5;

    HmacKey_t hmacKey;
    AESKey_t aesKey;
    for (int i = 0; i < sizeof hmacKey; i++) hmacKey[i] = N2_KEY_HMAC[0][i] ^ N2_KEY_HMAC[1][i];
    for (int i = 0; i < sizeof aesKey.m_Key; i++)
        aesKey.m_Key[i] = N2_KEY_AES[0].m_Key[i] ^ N2_KEY_AES[1].m_Key[i];
    for (int i = 0; i < sizeof aesKey.m_IV; i++)
        aesKey.m_IV[i] = N2_KEY_AES[0].m_IV[i] ^ N2_KEY_AES[1].m_IV[i];

    N2DeviceInfo_t deviceInfo = { 0 };
    deviceInfo.m_Addr = N2_ADDR;

    memcpy(&deviceInfo.m_HmacKey, hmacKey, sizeof hmacKey);
    memcpy(&deviceInfo.m_AESKey, &aesKey, sizeof aesKey);
    if (!MXK_N2_INIT)
        amiDebugLog("Error: Uninitialized.");
    else
        memcpy(&N2DeviceInfo, &deviceInfo, sizeof deviceInfo);

    MXK_HAS_INIT = TRUE;
    return 0;
}

// TODO: Better name
void _randomBytes(void *buf, int nbytes) {
    FILETIME fileTime;
    amtime_t amTime;

    for (int i = 0; i < nbytes; i++) {
        amiTimerGet(&amTime);
        GetSystemTimeAsFileTime(&fileTime);
        ((unsigned char *)buf)[i] =
            (amTime.microseconds ^ fileTime.dwLowDateTime ^ fileTime.dwHighDateTime) & 0xff;
    }
}
void mxkN2GetPacketNonce(PN2Nonce_t nonce) { _randomBytes(nonce, 20); }

int mxkN2UtilVCatenateData(unsigned char *out, size_t *count, unsigned int numStreams,
                           va_list args) {
    if (out == NULL || count == NULL) {
        amiDebugLog("Error: Invalid param.");
        return -2;
    }

    size_t offset = 0;
    size_t max = *count;
    for (size_t i = 0; i < numStreams && i < max; i++) {
        unsigned char *b = va_arg(args, unsigned char *);
        size_t n = va_arg(args, size_t);
        if (offset + n > max) n = max - offset;
        memcpy(out + offset, b, n);
        offset += n;
    }

    *count = offset;
    return 0;
}
void mxkN2UtilCatenateData(unsigned char *concatinated, size_t *count, unsigned int numStreams,
                           ...) {
    va_list args;
    va_start(args, numStreams);

    mxkN2UtilVCatenateData(concatinated, count, numStreams, args);

    va_end(args);
}

void mxkN2UtilSha1Many(PSha1Sum_t sum, int numStreams, ...) {
    va_list args;
    va_start(args, numStreams);

    Sha1Sum_t sumout;
    unsigned char concatinated[0x200];
    size_t size = sizeof concatinated;

    mxkN2UtilVCatenateData(concatinated, &size, numStreams, args);
    mxkCryptCalcHashWithSha1(concatinated, size, &sumout);
    memcpy_s(sum, sizeof *sum, sumout, sizeof sumout);

    va_end(args);
}

int mxkN2UtilHmacPacket(PSha1Sum_t packetSha, PHmacSum_t hmacSalt, PN2Nonce_t nonce, PHmacKey_t key,
                        PHmacSum_t computedHash) {
    HmacSum_t hash;
    unsigned char data_in[sizeof *packetSha + sizeof *hmacSalt + sizeof *nonce];

    size_t count = sizeof *packetSha + sizeof *hmacSalt + sizeof *nonce;
    size_t nout = HMAC_SUM_SIZE;
    mxkN2UtilCatenateData(data_in, &count, 3, packetSha, sizeof *packetSha, hmacSalt,
                          sizeof *hmacSalt, nonce, sizeof *nonce);

    int err = mxkCryptCalcHashWithHmacSha1(key, &hash, &nout, data_in, count);
    if (err == -2 || nout != HMAC_SUM_SIZE) return -2;

    memcpy_s(computedHash, sizeof *computedHash, hash, HMAC_SUM_SIZE);
    return 0;
}

int mxkN2CmdWriteData(unsigned char addr, void *data, unsigned short nbytes,
                      unsigned short *nbytesOut) {
    unsigned int ptr_;
    unsigned char nstep;
    int status;
    unsigned short position;

    if (mxkSmbusRequestMutex() != 0) {
        amiDebugLog("Error: mxkSmbusRequestMutex");
        return -6;
    }

    status = 0;
    position = 0;
    if (nbytes != 0) {
        do {
            ptr_ = position;
            if ((int)(nbytes - ptr_) < 5) {
                nstep = (byte)(nbytes - ptr_);
                if (nstep != 1) goto LAB_0040ae51;
                status = mxkSmbusWriteByte(addr, 0xcc, ((unsigned char *)data)[ptr_]);
            } else {
                nstep = 4;
            LAB_0040ae51:
                status = mxkSmbusI2CWriteBlock(addr, 0xcc, &((unsigned char *)data)[ptr_], nstep);
            }
            if (status != 0) {
                amiDebugLog("Error: Data write failed. Position %d. ErrorCode %d.", position,
                            status);

                break;
            }
            position += nstep;
            Sleep(0);
        } while (position < nbytes);
    }
    *nbytesOut = position;

    if (mxkSmbusReleaseMutex() != 0) {
        amiDebugLog("Error: mxkSmbusReleaseMutex");
        return -6;
    }
    return 0;
}

int mxkN2CmdWriteCommand(PN2Nonce_t requestNonce, PHmacSum_t hmacSalt, N2Tag_t tag,
                         N2Command_t command, PHmacKey_t hmacKey, unsigned char addr,
                         N2Byte_t *data, unsigned short nbytes) {
    Sha1Sum_t sha1sum;
    N2Packet_t packet;
    int ret;

    if (data == NULL && nbytes != 0) return -2;

    N2PacketFooter_t *footer = (N2PacketFooter_t *)(packet.m_Body + nbytes);

    ZeroMemory(&packet, sizeof packet);
    unsigned short paramsize =
        N2_HEADER_SIZE + nbytes +
        (tag == N2_TAG_RQU_COMMAND ? N2_FOOTER_RQU_SIZE : N2_FOOTER_SIGN_SIZE);

    N2Tag_t realTag = tag == N2_TAG_RQU_COMMAND ? N2_TAG_RQU_COMMAND : N2_TAG_RQU_AUTH_COMMAND;
    packet.m_Tag = _byteswap_ushort(realTag);
    packet.m_ParamSize = _byteswap_ushort(paramsize);
    packet.m_Command = _byteswap_ushort(command);

    if (data != NULL) memcpy_s(packet.m_Body, sizeof packet.m_Body, data, nbytes);

    mxkN2UtilSha1Many(&sha1sum, 1, &packet, nbytes + N2_HEADER_SIZE);

    if (tag == N2_TAG_RQU_COMMAND) {
        memcpy_s(footer->rqu.m_ShaSum, sizeof footer->rqu.m_ShaSum, sha1sum, sizeof sha1sum);
    } else {
        if (requestNonce == NULL || hmacKey == NULL) return -2;

        mxkN2GetPacketNonce(requestNonce);
        memcpy_s(footer->sign.m_Nonce, sizeof footer->sign.m_Nonce, requestNonce,
                 sizeof *requestNonce);

        mxkN2UtilHmacPacket(&sha1sum, hmacSalt, &footer->sign.m_Nonce, hmacKey,
                            &footer->sign.m_Hmac);
    }

    unsigned short nBytesOut;
    ret = mxkN2CmdWriteData(addr, &packet, paramsize, &nBytesOut);
    if (ret != 0) {
        amiDebugLog("Error: Data write failed. ErrorCode %d.", ret);
        return -8;
    }
    return 0;
}

int mxkN2CmdReadData(unsigned char addr, N2Packet_t *lpPacket, unsigned short nReadSize,
                     unsigned short *lpBytesRead) {
    int err;
    *lpBytesRead = 0;

    if (nReadSize < 4 || lpPacket == NULL) {
        amiDebugLog("Error: Invalid param.");
        return -2;
    }
    if (mxkSmbusRequestMutex() != 0) return -6;

    unsigned char *buffer = (unsigned char *)lpPacket;

    // Read until m_ParamSize
    err = mxkSmbusI2CReadBlock(addr, 0xc3, buffer, 4);

    unsigned short readPosition = 4;
    if (err == 0) {
        unsigned short paramSize = _byteswap_ushort(lpPacket->m_ParamSize);
        if (nReadSize < paramSize - 4) {
            amiDebugLog("mxkSmbusI2CReadBlock data : %04X %04X", lpPacket->m_Tag,
                        lpPacket->m_ParamSize);
            amiDebugLog("Error: Data size is too small.(readSize:%04X, m_paramSize:%04X)",
                        nReadSize, (lpPacket->m_ParamSize << 8 | lpPacket->m_ParamSize >> 8) - 4);
            return -11;
        }

        if (4 < paramSize) {
            while (readPosition < paramSize) {
                unsigned short blockSize = paramSize - readPosition;
                if (blockSize > 4) blockSize = 4;

                if (blockSize == 1)
                    err = mxkSmbusReadByte(addr, buffer + readPosition, 0xc3);
                else
                    err = mxkSmbusI2CReadBlock(addr, 0xc3, buffer + readPosition, blockSize & 0xff);

                if (err != 0) {
                    amiDebugLog("Error: Data read failed. Position %d. ErrorCode %d.", readPosition,
                                err);
                    break;
                }

                readPosition += blockSize;
            }
        }
    }

    *lpBytesRead = readPosition;
    if (mxkSmbusReleaseMutex() != 0) {
        amiDebugLog("Error: ReleaseMutex(). ErrorCode %d.", GetLastError());
        return -6;
    }
    if (err) return -1;
    return 0;
}

bool mxkN2CmdIsValidOutParam(N2Packet_t *packet, unsigned tag, unsigned short paramSize) {
    unsigned short tagPacket = _byteswap_ushort(packet->m_Tag);
    unsigned short paramSizePacket = _byteswap_ushort(packet->m_ParamSize);
    unsigned short returnCode = _byteswap_ushort(packet->m_Command);
    if (tagPacket == tag && paramSizePacket == paramSize && returnCode == 0) return true;

    amiDebugLog("tag: %04x, paramSize: %d, returnCode: %04x", tagPacket, paramSizePacket,
                returnCode);
    return false;
}

int mxkN2CmdReadResponce(PN2Nonce_t requestNonce, PHmacSum_t hmacSalt, N2Tag_t tag,
                         N2Command_t command, PHmacKey_t hmacKey, unsigned char addr, void *bufOut,
                         unsigned short nOut) {
    int ret;
    unsigned short nBytesRead;
    Sha1Sum_t sha1sum;
    N2Packet_t packet;

    ZeroMemory(&packet, sizeof packet);
    unsigned short footerSize =
        tag == N2_TAG_RQU_COMMAND ? N2_FOOTER_RQU_SIZE : N2_FOOTER_SIGN_SIZE;
    ret = mxkN2CmdReadData(addr, &packet, nOut + N2_HEADER_SIZE + footerSize, &nBytesRead);
    if (ret != 0) {
        amiDebugLog("Error: Data read failed. ErrorCode %d.", ret);
        return -7;
    }

    if (!mxkN2CmdIsValidOutParam(
            &packet, tag == N2_TAG_RQU_COMMAND ? N2_TAG_RSP_COMMAND : N2_TAG_RSP_AUTH_COMMAND,
            nBytesRead)) {
        amiDebugLog("Error: Invalid output parameter.");
        return -9;
    }

    unsigned short fixedCommand = _byteswap_ushort(command);

    mxkN2UtilSha1Many(&sha1sum, 3, &packet, N2_HEADER_SIZE, &fixedCommand, sizeof fixedCommand,
                      packet.m_Body, nOut);

    N2PacketFooter_t *footer = (N2PacketFooter_t *)(packet.m_Body + nOut);

    if (tag == N2_TAG_RQU_COMMAND) {
        if (memcmp(footer->rqu.m_ShaSum, sha1sum, sizeof sha1sum) != 0) {
            amiDebugLog("Error: Invalid hash value.");
            return -10;
        }
    } else {
        if (requestNonce == NULL || hmacKey == NULL) return -2;

        HmacSum_t computedHash;
        mxkN2UtilHmacPacket(&sha1sum, &footer->sign.m_Nonce, requestNonce, hmacKey, &computedHash);
        if (memcmp(footer->sign.m_Hmac, computedHash, sizeof computedHash) != 0) {
            amiDebugLog("Error: Invalid hash value.");
            return -10;
        }
        memcpy_s(requestNonce, sizeof *requestNonce, packet.m_Body + nOut, 20);

        if (hmacSalt != NULL)
            memcpy_s(hmacSalt, sizeof *hmacSalt, &footer->sign.m_Nonce,
                     sizeof footer->sign.m_Nonce);
    }

    if (bufOut != NULL && nOut != 0) {
        memcpy_s(bufOut, nOut, packet.m_Body,
                 ((nBytesRead - footerSize) - N2_HEADER_SIZE) & 0xffff);
    }
    return 0;
}

int mxkN2CmdEnableSession(void) {
    N2Nonce_t nonce;

    CheckN2Init;

    int err;
    err = mxkN2CmdWriteCommand(&nonce, NULL, N2_TAG_RQU_COMMAND, N2_ORD_ENABLE_SESSION, NULL,
                               N2DeviceInfo.m_Addr, NULL, 0);
    if (err != 0) return err;

    Sleep(32);
    err = mxkN2CmdReadResponce(&nonce, NULL, N2_TAG_RQU_COMMAND, N2_ORD_ENABLE_SESSION, NULL,
                               N2DeviceInfo.m_Addr, &N2DeviceInfo.m_HmacSalt, 20);
    if (err != 0) return err;
    Sleep(16);
    return 0;
}

int mxkN2CmdGetErrorCode(unsigned short *errorCode) {
    N2Nonce_t nonce;

    CheckN2Init;

    int err;
    err = mxkN2CmdWriteCommand(&nonce, NULL, N2_TAG_RQU_COMMAND, N2_ORD_GET_ERROR_CODE, NULL,
                               N2DeviceInfo.m_Addr, NULL, 0);
    if (err != 0) return err;

    Sleep(32);
    unsigned short rawErrorCode;
    err = mxkN2CmdReadResponce(&nonce, NULL, N2_TAG_RQU_COMMAND, N2_ORD_GET_ERROR_CODE, NULL,
                               N2DeviceInfo.m_Addr, &rawErrorCode, 2);
    if (err != 0) return err;

    Sleep(16);
    *errorCode = _byteswap_ushort(rawErrorCode);
    return 0;
}

int mxkN2CmdSetAuthKey(PN2AuthKey_t authKey) {
    CheckN2Init;

    unsigned char authKeyPt[32];
    ZeroMemory(authKeyPt, sizeof authKeyPt);
    unsigned char authKeyCt[32];
    memcpy_s(authKeyPt, sizeof authKeyPt, authKey, sizeof *authKey);
    mxkCryptEncryptAes128CBC(N2DeviceInfo.m_AESKey.m_Key, N2DeviceInfo.m_AESKey.m_IV, authKeyCt,
                             authKeyPt, sizeof authKeyPt);

    N2Nonce_t nonce;
    int err;
    err = mxkN2CmdWriteCommand(&nonce, &N2DeviceInfo.m_HmacSalt, N2_TAG_RQU_AUTH_COMMAND,
                               N2_ORD_SET_AUTH_KEY, &N2DeviceInfo.m_HmacKey, N2DeviceInfo.m_Addr,
                               authKeyCt, sizeof authKeyCt);
    if (err != 0) return err;

    Sleep(96);
    err = mxkN2CmdReadResponce(&nonce, &N2DeviceInfo.m_HmacSalt, N2_TAG_RQU_AUTH_COMMAND,
                               N2_ORD_SET_AUTH_KEY, &N2DeviceInfo.m_HmacKey, N2DeviceInfo.m_Addr,
                               NULL, 0);
    if (err != 0) return err;
    Sleep(16);
    memcpy_s(N2DeviceInfo.m_AuthKey, sizeof N2DeviceInfo.m_AuthKey, authKey, sizeof *authKey);
    return 0;
}

int mxkN2CmdSetEncKey(AESKey_t *encKey) {
    CheckN2Init;

    unsigned char encKeyCt[sizeof *encKey];
    mxkCryptEncryptAes128CBC(N2DeviceInfo.m_AESKey.m_Key, N2DeviceInfo.m_AESKey.m_IV, encKeyCt,
                             encKey, sizeof encKeyCt);

    N2Nonce_t requestNonce;
    int err;
    err = mxkN2CmdWriteCommand(&requestNonce, &N2DeviceInfo.m_HmacSalt, N2_TAG_RQU_AUTH_COMMAND,
                               N2_ORD_SET_ENC_KEY, &N2DeviceInfo.m_AuthKey, N2DeviceInfo.m_Addr,
                               encKeyCt, sizeof encKeyCt);
    if (err != 0) return err;

    Sleep(96);
    err = mxkN2CmdReadResponce(&requestNonce, &N2DeviceInfo.m_HmacSalt, N2_TAG_RQU_AUTH_COMMAND,
                               N2_ORD_SET_ENC_KEY, &N2DeviceInfo.m_AuthKey, N2DeviceInfo.m_Addr,
                               NULL, 0);
    if (err != 0) return err;

    Sleep(16);
    memcpy_s(&N2DeviceInfo.m_EncKey, sizeof N2DeviceInfo.m_EncKey, encKey, sizeof *encKey);
    return 0;
}

int mxkN2CmdGetAuthLevel(unsigned char *authLevel) {
    CheckN2Init;

    unsigned char rawAuthlevel;
    N2Nonce_t nonce;

    int err;
    err = mxkN2CmdWriteCommand(&nonce, NULL, N2_TAG_RQU_COMMAND, N2_ORD_GET_AUTH_LEVEL, NULL,
                               N2DeviceInfo.m_Addr, NULL, 0);
    if (err != 0) return err;
    Sleep(32);
    err = mxkN2CmdReadResponce(&nonce, NULL, N2_TAG_RQU_COMMAND, N2_ORD_GET_AUTH_LEVEL, NULL,
                               N2DeviceInfo.m_Addr, &rawAuthlevel, 1);
    if (err != 0) return err;
    Sleep(16);
    err = 0;
    *authLevel = rawAuthlevel;
    return 0;
}

int mxkN2GetErrorCode(void) {
    unsigned short errorCode;

    int err = mxkN2CmdGetErrorCode(&errorCode);
    if (err != 0) {
        amiDebugLog("Error: mxkN2CmdGetErrorCode(). ErrorCode %d", err);
        return -5;
    }

    amiDebugLog("N2 ErrorCode 0x%04X", errorCode);
    return 0;
}
int mxkN2CmdReadKeychipID(N2KeychipId_t *lpKeychipId) {
    CheckN2Init;

    N2Nonce_t nonce;

    unsigned char readData[sizeof *lpKeychipId + 2];

    N2Byte_t request[4];
    request[0] = 0;
    request[1] = 0;
    request[2] = 0;
    request[3] = sizeof *lpKeychipId;

    int err;
    err = mxkN2CmdWriteCommand(&nonce, &N2DeviceInfo.m_HmacSalt, N2_TAG_RQU_AUTH_COMMAND,
                               N2_ORD_READ_KEYCHIP_ID, &N2DeviceInfo.m_AuthKey, N2DeviceInfo.m_Addr,
                               request, sizeof request);
    if (err != 0) return err;
    Sleep(128);
    err = mxkN2CmdReadResponce(&nonce, &N2DeviceInfo.m_HmacSalt, N2_TAG_RQU_AUTH_COMMAND,
                               N2_ORD_READ_KEYCHIP_ID, &N2DeviceInfo.m_AuthKey, N2DeviceInfo.m_Addr,
                               readData, sizeof readData);

    if (err != 0) return err;
    Sleep(16);

    mxkCryptDecryptAes128CBC(N2DeviceInfo.m_EncKey.m_Key, N2DeviceInfo.m_EncKey.m_IV, readData + 2,
                             (unsigned char *)lpKeychipId, sizeof *lpKeychipId);
    return 0;
}

int mxkN2Authentication(void) {
    N2AuthKey_t authKey;
    AESKey_t enc_key;

    if (!MXK_HAS_INIT) {
        amiDebugLog("No Init Error!!");
        return -3;
    }

    int err;
    err = mxkN2CmdEnableSession();
    if (err != 0) {
        mxkN2GetErrorCode();
        amiDebugLog("Error: mxkN2CmdEnableSession(). ErrorCode %d", err);
        return -5;
    }

    unsigned char authLevel;
    mxkN2CmdGetAuthLevel(&authLevel);
    printf("Achieved auth level: %d\n", authLevel);

    _randomBytes(authKey, sizeof authKey);
    err = mxkN2CmdSetAuthKey(&authKey);
    if (err != 0) {
        mxkN2GetErrorCode();
        amiDebugLog("Error: mxkN2CmdSetAuthKey(). ErrorCode %d", err);
        return -5;
    }

    mxkN2CmdGetAuthLevel(&authLevel);
    printf("Achieved auth level: %d\n", authLevel);

    _randomBytes(&enc_key, sizeof enc_key);
    err = mxkN2CmdSetEncKey(&enc_key);
    if (err != 0) {
        mxkN2GetErrorCode();
        amiDebugLog("Error: mxkN2CmdSetEncKey().ErrorCode %d", err);
        return -5;
    }

    mxkN2CmdGetAuthLevel(&authLevel);
    printf("Achieved auth level: %d\n", authLevel);

    // unsigned char authLevel;
    err = mxkN2CmdGetAuthLevel(&authLevel);
    if (err == 0 && authLevel == 3) return 0;

    mxkN2GetErrorCode();
    amiDebugLog("Error: mxkN2CmdGetAuthLevel().ErrorCode %d", err);
    return -5;
}

void mxkN2Exit(void) { MXK_N2_INIT = FALSE; }

int mxkN2Init(void) {
    int err;
    err = mxkN2CmdSetDeviceInfo();
    if (err != 0) return err;

    err = mxkN2Authentication();
    if (err != 0) {
        unsigned short errorCode;
        err = mxkN2CmdGetErrorCode(&errorCode);
        if (err != 0) {
            amiDebugLog("Error: mxkN2CmdGetErrorCode(). ErrorCode %d", err);
        } else {
            amiDebugLog("N2 ErrorCode 0x%04X", errorCode);
        }
        return -1;
    }
    return 0;
}
