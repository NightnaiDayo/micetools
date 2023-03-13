#include <Windows.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "../../dll/devices/smb_n2.h"
#include "../ami/ami.h"
#include "mxkCrypt.h"
#include "mxkSmbus.h"

#define N2_ADDR 0x30

typedef struct {
    unsigned char m_Key[16];
    unsigned char m_IV[16];
} AESKey_t;

typedef struct {
    unsigned short m_Tag;
    unsigned short m_ParamSize;
    unsigned short m_Command;
    unsigned char m_Body[506];
} N2Packet_t;

BOOL MXK_HAS_INIT = FALSE;
BOOL MXK_N2_INIT = FALSE;
typedef struct {
    unsigned char m_Addr;
    unsigned char m_HmacKey[20];
    AESKey_t m_AESKey;
    unsigned char m_AuthKey[20];
    unsigned char m_EncKey[16];
    unsigned char Unk[16];
    unsigned char m_HmacOut[20];
} N2DeviceInfo_t;
N2DeviceInfo_t N2DeviceInfo;

unsigned char N2_KEY_HMAC[2][20] = {
    { 0x2c, 0xf8, 0x3e, 0x5e, 0x51, 0xf0, 0x60, 0x1b, 0xf6, 0xb1,
      0x49, 0x11, 0x3a, 0xaf, 0x36, 0xe1, 0x51, 0x1c, 0x16, 0x05 },
    { 0xba, 0x15, 0x0f, 0xec, 0x79, 0x81, 0x65, 0xbe, 0x55, 0x81,
      0x1d, 0x1e, 0x1f, 0x11, 0xee, 0xb0, 0xf4, 0xd4, 0x20, 0x24 }
};

AESKey_t N2_KEY_AES[2] = { { { 0xd7, 0xf6, 0x86, 0xfb, 0x63, 0x44, 0xff, 0xa5, 0x60, 0x9d, 0x4e,
                               0xf0, 0x18, 0xe9, 0x45, 0xb4 },
                             { 0x12, 0x6c, 0xb1, 0xdb, 0x9e, 0x00, 0x8c, 0xc6, 0xae, 0xa1, 0x73,
                               0xec, 0xe7, 0x2f, 0x5a, 0xc4 } },
                           { { 0xa1, 0x1a, 0xc4, 0x4d, 0xcd, 0x48, 0x4f, 0xed, 0x70, 0xcc, 0x3f,
                               0x5d, 0x94, 0x5b, 0xbe, 0xb3 },
                             { 0x9c, 0x5c, 0xba, 0x7f, 0xb0, 0x15, 0xc7, 0x69, 0xcb, 0xb4, 0x41,
                               0x19, 0x97, 0x2b, 0x45, 0x9f } } };

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

    unsigned char hmacKey[20];
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
void _randomBytes(unsigned char *buf, int nbytes) {
    FILETIME fileTime;
    amtime_t amTime;

    for (int i = 0; i < nbytes; i++) {
        amiTimerGet(&amTime);
        GetSystemTimeAsFileTime(&fileTime);
        buf[i] = (amTime.microseconds ^ fileTime.dwLowDateTime ^ fileTime.dwHighDateTime) & 0xff;
    }
}
void mxkN2GetPacketNonce(unsigned char *nonce) { _randomBytes(nonce, 20); }

int mxkN2UtilVCatenateData(unsigned char *out, size_t *count, unsigned int numStreams,
                           va_list args) {
    if (out == NULL || count == NULL) {
        amiDebugLog("Error: Invalid param.");
        return -2;
    }

    size_t offset = 0;
    size_t max = *count;
    for (size_t i = 0; i < numStreams && i < max; i++) {
        size_t n = va_arg(args, size_t);
        unsigned char *b = va_arg(args, unsigned char *);
        if (i + n > max) n = max - i;
        memcpy(out + offset, b, i);
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

void mxkN2UtilSha1Many(unsigned char *sum, int numStreams, ...) {
    va_list args;
    va_start(args, numStreams);

    unsigned char sumout[20];
    unsigned char concatinated[0x200];
    size_t size = sizeof concatinated;

    mxkN2UtilVCatenateData(concatinated, &size, numStreams, args);
    mxkCryptCalcHashWithSha1(concatinated, size, sumout);
    memcpy_s(sum, 20, sumout, 20);

    va_end(args);
}

int mxkN2UtilHmacPacket(void *val1, unsigned char *sha1, void *nonce, void *key,
                        unsigned char *hash_out) {
    unsigned char hash[20];
    unsigned char data_in[60];

    size_t count = 60;
    size_t nout = 20;
    mxkN2UtilCatenateData(data_in, &count, 3, val1, 20, sha1, 20, nonce, 20);

    int iVar1 = mxkCryptCalcHashWithHmacSha1(key, hash, &nout, data_in, count);
    if (iVar1 != -2 && nout == 20) {
        memcpy(hash_out, hash, 20);
        return 0;
    }
    return -2;
}

int mxkN2CmdWriteData(unsigned char addr, unsigned char *data, unsigned short nbytes,
                      unsigned short *nbytesOut) {
    unsigned int ptr_;
    unsigned char nstep;
    int status;
    unsigned short position;

    if (mxkSmbusRequestMutex() != 0) return -6;

    status = 0;
    position = 0;
    if (nbytes != 0) {
        do {
            ptr_ = position;
            if ((int)(nbytes - ptr_) < 5) {
                nstep = (byte)(nbytes - ptr_);
                if (nstep != 1) goto LAB_0040ae51;
                status = mxkSmbusWriteByte(addr, 0xcc, data[ptr_]);
            } else {
                nstep = 4;
            LAB_0040ae51:
                status = mxkSmbusI2CWriteBlock(addr, 0xcc, data + ptr_, nstep);
            }
            if (status != 0) {
                amiDebugLog("Error: Data write failed. Position %d. ErrorCode %d.\n", position,
                            status);

                break;
            }
            position += nstep;
            Sleep(0);
        } while (position < nbytes);
    }
    *nbytesOut = position;

    if (!mxkSmbusReleaseMutex()) return -6;
    return 0;
}

int mxkN2CmdWriteCommand(unsigned char *param_1, unsigned char *packet, unsigned short tag,
                         unsigned short command, unsigned char *auth_key, unsigned char addr,
                         void *data, size_t nbytes) {
    int iVar1;
    unsigned short real_tag;
    unsigned short paramsize;

    N2Packet_t cmd;

    if (data == NULL && nbytes != 0) return -2;

    memset(&cmd, 0, 0x200);
    paramsize = (tag == N2_TAG_RQU_COMMAND ? 20 : 0) + 0x14 + (nbytes & 0xffff) + 6;
    real_tag = (tag != N2_TAG_RQU_COMMAND) + 0xc1;
    cmd.m_Tag = real_tag * 0x100 | real_tag >> 8;
    cmd.m_ParamSize = (unsigned short)paramsize << 8 | (unsigned short)paramsize >> 8;
    cmd.m_Command = command << 8 | command >> 8;
    if (data != NULL) memcpy_s(cmd.m_Body, sizeof cmd.m_Body, data, nbytes);

    // SHA1(header | data)
    unsigned char packet_sha[20];
    mxkN2UtilSha1Many(packet_sha, 1, &cmd, nbytes + 6);

    if (tag == N2_TAG_RQU_COMMAND) {
        memcpy(cmd.m_Body, packet_sha, 20);
    } else {
        unsigned char nonce[20];
        mxkN2GetPacketNonce(nonce);
        memcpy(cmd.m_Body + nbytes, nonce, 20);
        mxkN2UtilHmacPacket(packet_sha, param_1, nonce, auth_key, cmd.m_Body + nbytes + 20);

        puts("Dodgy HMAC");
        exit(1);
    }

    unsigned short nWrote;
    iVar1 = mxkN2CmdWriteData(addr, (unsigned char *)&cmd, paramsize, &nWrote);
    if (iVar1 == 0) {
        iVar1 = 0;
    } else {
        amiDebugLog("Error: Data write failed. ErrorCode %d.", iVar1);
        iVar1 = -8;
    }
    return iVar1;
}

int mxkN2CmdEnableSession(void) {
    int ret;
    unsigned char local_18[20];

    if (!MXK_N2_INIT) {
        amiDebugLog("Error: Uninitialized.");
        return -3;
    }

    ret = mxkN2CmdWriteCommand(NULL, local_18, N2_TAG_RQU_COMMAND, N2_ORD_ENABLE_SESSION, NULL,
                               N2DeviceInfo.m_Addr, NULL, 0);
    if (ret != 0) return ret;

    Sleep(32);
    // TODO: ASAP
    // ret = mxkN2CmdReadResponce(local_18, NULL, N2_TAG_RQU_COMMAND, N2_ORD_ENABLE_SESSION, NULL,
    //                            N2DeviceInfo.m_Addr, N2DeviceInfo.m_HmacOut, 20);
    if (ret != 0) return ret;
    Sleep(16);
    return 0;
}

int mxkN2Authentication(void) {
    // TODO: ASAP
    // unsigned char auth_key[20];
    // unsigned char enc_key[32];

    if (!MXK_HAS_INIT) {
        amiDebugLog("No Init Error!!");
        return -3;
    }

    int ErrorCode;

    ErrorCode = mxkN2CmdEnableSession();
    if (ErrorCode != 0) {
        // mxkN2GetErrorCode();
        amiDebugLog("Error: mxkN2CmdEnableSession(). ErrorCode %d", ErrorCode);
        return -5;
    }

    // _randomBytes(auth_key, sizeof auth_key);
    // ErrorCode = mxkN2CmdSetAuthKey(auth_key);
    // if (ErrorCode != 0) {
    //     mxkN2GetErrorCode();
    //     amiDebugLog("Error: mxkN2CmdSetAuthKey(). ErrorCode %d", ErrorCode);
    //     return -5;
    // }

    // _randomBytes(enc_key, sizeof enc_key);
    // ErrorCode = mxkN2CmdSetEncKey(enc_key);
    // if (ErrorCode != 0) {
    //     mxkN2GetErrorCode();
    //     amiDebugLog("Error: mxkN2CmdSetEncKey().ErrorCode %d", ErrorCode);
    //     return -5;
    // }

    // unsigned char auth_level;
    // ErrorCode = mxkN2CmdGetAuthLevel(&auth_level);
    // if (ErrorCode == 0 && auth_level == 3) return 0;

    // mxkN2GetErrorCode();
    // amiDebugLog("Error: mxkN2CmdGetAuthLevel().ErrorCode %d", ErrorCode);
    return -5;
}
