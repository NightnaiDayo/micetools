#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#include "../../lib/mxk/mxk.h"
#include "_devices.h"

#define N2_TAG_OFFSET 0
#define N2_PARAMSIZE_OFFSET 2
#define N2_COMMAND_OFFSET 4
#define N2_TAG_SIZE 2
#define N2_PARAMSIZE_SIZE 2
#define N2_COMMAND_SIZE 2
#define N2_CHECKSUM_SIZE 20
#define N2_AUTH_SIZE 20
#define N2_HEADER_SIZE (N2_TAG_SIZE + N2_PARAMSIZE_SIZE + N2_COMMAND_SIZE)

#define N2_TAG_RQU_COMMAND 0xC1
#define N2_TAG_RQU_AUTH_COMMAND 0xC2
#define N2_TAG_RSP_COMMAND 0xC3
#define N2_TAG_RSP_AUTH_COMMAND 0xC4
// TODO: Uncomment these if actually used, and adjust lazy +2 logic alongside
// #define N2_TAG_RQU_RSA_COMMAND 0xC5
// #define N2_TAG_RSP_RSA_COMMAND 0xC6

#define N2_ORD_ENABLE_SESSION 1
#define N2_ORD_DISABLE_SESSION 2
#define N2_ORD_SET_AUTH_KEY 3
#define N2_ORD_SET_ENC_KEY 4
// GAP!
#define N2_ORD_GET_AUTH_LEVEL 9
// GAP!
#define N2_ORD_GET_ERROR_CODE 11
#define N2_ORD_GET_VERSION 12
// GAP! (-> keychip info write?)
#define N2_ORD_READ_KEYCHIP_ID 14
// GAP! (-> gkey write?)
#define N2_ORD_ENCRYPT_WITH_GKEY 16  // keychip.encrypt
#define N2_ORD_DECRYPT_WITH_GKEY 17  // keychip.decrypt
#define N2_ORD_ADD_PLAY_COUNT 18
#define N2_ORD_READ_PLAY_COUNT 19
// GAP! (-> storage? is that still on pic?)
#define N2_ORD_GET_RANDOM_VALUE 24
#define N2_ORD_ENCRYPT_WITH_SKEY 25  // keychip.ssd.hostproof
#define N2_ORD_DECRYPT_WITH_SKEY 26

#define N2_SUCCESS 0
#define N2_INVALID_AUTH 1
#define N2_AUTHFAIL 2
#define N2_LV_ERROR 3
#define N2_BAD_PARAMETER 4
#define N2_EEP_WRITEFAIL 5
#define N2_BAD_TAG 6
#define N2_BAD_ORDINAL 7
#define N2_SUMFAIL 8
#define N2_EEPWRITE_DISABLE 9
#define N2_BAD_DATASIZE 10
#define N2_FAIL 11

#define N2_IO_BUFFER 0x1000

BYTE n2_in_buffer[N2_IO_BUFFER];
WORD n2_in_pointer = 0;
BYTE n2_out_buffer[N2_IO_BUFFER];
WORD n2_out_pointer = 0;

WORD n2_error_code = 0;

BYTE n2_auth_key[20] = { 0x96, 0xed, 0x31, 0xb2, 0x28, 0x71, 0x05, 0xa5, 0xa3, 0x30,
                         0x54, 0x0f, 0x25, 0xbe, 0xd8, 0x51, 0xa5, 0xc8, 0x36, 0x21 };

BYTE n2_session_nonce[20];
struct {
    BYTE key[16];
    BYTE iv[16];
} n2_enc_key = {
    .key = { 0x76, 0xec, 0x42, 0xb6, 0xae, 0x0c, 0xb0, 0x48, 0x10, 0x51, 0x71, 0xad, 0x8c, 0xb2,
             0xfb, 0x07 },
    .iv = { 0x8e, 0x30, 0x0b, 0xa4, 0x2e, 0x15, 0x4b, 0xaf, 0x65, 0x15, 0x32, 0xf5, 0x70, 0x04,
            0x1f, 0x5b },
};

#define fix_endian(x) (((x & 0xff00) >> 8) | (((x)&0xff) << 8))

typedef struct {
    WORD tag;
    WORD paramSize;
    WORD (*handler)(WORD paramSize, LPBYTE dataIn, LPBYTE dataOut, LPWORD nOut);
} n2_command;

#pragma pack(push, 1)
typedef struct {
    DWORD m_Crc;
    BYTE Rsv04[12];
    char m_KeyId[16];
    struct {
        DWORD m_Crc;
        DWORD m_Format;
        char m_GameId[4];
        BYTE m_Region;
        BYTE m_ModelType;
        BYTE m_SystemFlag;
        BYTE Rsv0f;
        //
        char m_PlatformId[3];
        BYTE m_DvdFlag;
        DWORD m_NetworkAddr;
        BYTE Unk00[88];
        BYTE m_Seed[16];
    } m_Appboot;
} N2_KEYCHIP_INFO, *PN2_KEYCHIP_INFO;
#pragma pack(pop)

N2_KEYCHIP_INFO n2_keychip_info = {
    .m_KeyId = { 'A', '7', '2', 'E', '-', '0', '2', 'D', '1', '1', '2', '6', '1', '1', '1', '6' },
    .m_Appboot = {
        .m_Format = 1,
        .m_GameId = { 'S' , 'D', 'E', 'Y' },
        .m_Region = 0xff,
        .m_ModelType = 2,
        .m_SystemFlag = 0x24,
        .m_PlatformId = {'A', 'A', 'S'},
        .m_DvdFlag = 1,
        .m_NetworkAddr = (192 << 0) | (168 << 8) | (103 << 16) | (0 << 24),
    }
};

WORD n2_enable_session(WORD paramSize, LPBYTE dataIn, LPBYTE dataOut, LPWORD nOut) {
    *nOut = 20;

    RAND_bytes(n2_session_nonce, sizeof n2_session_nonce);
    memcpy(dataOut, n2_session_nonce, sizeof n2_session_nonce);

    log_misc("smb-n2", "Session open");
    return N2_SUCCESS;
}
WORD n2_set_auth_key(WORD paramSize, LPBYTE dataIn, LPBYTE dataOut, LPWORD nOut) {
    *nOut = 0;
    log_misc("smb-n2", "Auth key set");

    BYTE pt[32];
    mxkCryptDecryptAes128CBC(n2_enc_key.key, n2_enc_key.iv, dataIn, pt, sizeof pt);
    memcpy(n2_auth_key, pt, sizeof n2_auth_key);

    return N2_SUCCESS;
}
WORD n2_set_enc_key(WORD paramSize, LPBYTE dataIn, LPBYTE dataOut, LPWORD nOut) {
    *nOut = 0;
    log_misc("smb-n2", "Enc key set");

    BYTE pt[32];
    mxkCryptDecryptAes128CBC(n2_enc_key.key, n2_enc_key.iv, dataIn, pt, sizeof pt);
    memcpy(n2_enc_key.key, pt, sizeof n2_enc_key.key);
    memcpy(n2_enc_key.iv, pt + sizeof n2_enc_key.key, sizeof n2_enc_key.iv);

    return N2_SUCCESS;
}
WORD n2_get_auth_level(WORD paramSize, LPBYTE dataIn, LPBYTE dataOut, LPWORD nOut) {
    *nOut = 1;
    dataOut[0] = 3;  // TODO: ?
    log_misc("smb-n2", "Auth level get");
    return N2_SUCCESS;
}
WORD n2_get_error_code(WORD paramSize, LPBYTE dataIn, LPBYTE dataOut, LPWORD nOut) {
    *nOut = 2;
    ((PWORD)dataOut)[0] = fix_endian(n2_error_code);
    log_misc("smb-n2", "Error get");
    return N2_SUCCESS;
}
WORD n2_read_keychip_id(WORD paramSize, LPBYTE dataIn, LPBYTE dataOut, LPWORD nOut) {
    DWORD nbytes = (dataIn[0] << 24) | (dataIn[1] << 16) | (dataIn[2] << 8) | dataIn[3];

    *nOut = (nbytes & 0xff) + 2;

    n2_keychip_info.m_Appboot.m_Crc = amCrc32RGet(sizeof n2_keychip_info.m_Appboot - 4, (LPBYTE)&n2_keychip_info.m_Appboot + 4, 0);
    n2_keychip_info.m_Crc = amCrc32RGet(sizeof n2_keychip_info - 4, (LPBYTE)&n2_keychip_info + 4, 0);

    mxkCryptEncryptAes128CBC(n2_enc_key.key, n2_enc_key.iv, dataOut + 2, (LPBYTE)&n2_keychip_info, nbytes);

    log_misc("smb-n2", "Read keychip ID: %08x", nbytes);
    return N2_SUCCESS;
}
WORD n2_get_version(WORD paramSize, LPBYTE dataIn, LPBYTE dataOut, LPWORD nOut) {
    *nOut = 2;
    dataOut[0] = 0x01;
    dataOut[1] = 0x04;
    return N2_SUCCESS;
}

WORD n2_encrypt_with_gkey(WORD paramSize, LPBYTE dataIn, LPBYTE dataOut, LPWORD nOut) {
    *nOut = 16;
    BYTE temp[16];
    mxkCryptDecryptAes128CBC(n2_enc_key.key, n2_enc_key.iv, dataIn + 1, temp, sizeof temp);

    for(size_t i=0;i<17;i++)printf(" %02x",dataIn[i]);
    puts("");
    for(size_t i=0;i<17;i++)printf(" %02x",temp[i]);
    puts("");

    // Do Gkey encryption

    mxkCryptEncryptAes128CBC(n2_enc_key.key, n2_enc_key.iv, dataOut, temp, sizeof temp);
    return N2_SUCCESS;
}
WORD n2_decrypt_with_gkey(WORD paramSize, LPBYTE dataIn, LPBYTE dataOut, LPWORD nOut) {
    *nOut = 16;
    BYTE temp[16];
    mxkCryptDecryptAes128CBC(n2_enc_key.key, n2_enc_key.iv, dataIn + 1, temp, sizeof temp);

    // Do Gkey decryption

    mxkCryptEncryptAes128CBC(n2_enc_key.key, n2_enc_key.iv, dataOut, temp, sizeof temp);
    return N2_SUCCESS;
}
WORD n2_add_play_count(WORD paramSize, LPBYTE dataIn, LPBYTE dataOut, LPWORD nOut) {
    *nOut = 0;
    return N2_SUCCESS;
}
WORD n2_read_play_count(WORD paramSize, LPBYTE dataIn, LPBYTE dataOut, LPWORD nOut) {
    *nOut = 4;
    return N2_SUCCESS;
}
WORD n2_get_random_value(WORD paramSize, LPBYTE dataIn, LPBYTE dataOut, LPWORD nOut) {
    *nOut = 16;
    RAND_bytes(dataOut, 16);
    return N2_SUCCESS;
}
WORD n2_encrypt_with_skey(WORD paramSize, LPBYTE dataIn, LPBYTE dataOut, LPWORD nOut) {
    *nOut = 16;
    BYTE temp[16];
    mxkCryptDecryptAes128CBC(n2_enc_key.key, n2_enc_key.iv, dataIn + 1, temp, sizeof temp);

    // Do Skey encryption

    mxkCryptEncryptAes128CBC(n2_enc_key.key, n2_enc_key.iv, dataOut, temp, sizeof temp);
    return N2_SUCCESS;
}
WORD n2_decrypt_with_skey(WORD paramSize, LPBYTE dataIn, LPBYTE dataOut, LPWORD nOut) {
    *nOut = 16;
    BYTE temp[16];
    mxkCryptDecryptAes128CBC(n2_enc_key.key, n2_enc_key.iv, dataIn + 1, temp, sizeof temp);

    // Do Skey decryption

    mxkCryptEncryptAes128CBC(n2_enc_key.key, n2_enc_key.iv, dataOut, temp, sizeof temp);
    return N2_SUCCESS;
}


n2_command N2_COMMANDS[0xff];
#define N2_INSTALL_COMMAND(ord, tag_, paramSize_, handler_) \
    do {                                                    \
        N2_COMMANDS[ord].tag = (tag_);                      \
        N2_COMMANDS[ord].paramSize = (paramSize_);          \
        N2_COMMANDS[ord].handler = &(handler_);             \
    } while (0)

void n2_install_commands() {
    static bool installed = false;
    if (installed) return;
    installed = true;

    ZeroMemory(N2_COMMANDS, sizeof N2_COMMANDS);
    N2_INSTALL_COMMAND(N2_ORD_ENABLE_SESSION, N2_TAG_RQU_COMMAND, 0, n2_enable_session);

    N2_INSTALL_COMMAND(N2_ORD_SET_AUTH_KEY, N2_TAG_RQU_AUTH_COMMAND, 32, n2_set_auth_key);
    N2_INSTALL_COMMAND(N2_ORD_SET_ENC_KEY, N2_TAG_RQU_AUTH_COMMAND, 32, n2_set_enc_key);

    N2_INSTALL_COMMAND(N2_ORD_GET_AUTH_LEVEL, N2_TAG_RQU_COMMAND, 0, n2_get_auth_level);

    N2_INSTALL_COMMAND(N2_ORD_GET_ERROR_CODE, N2_TAG_RQU_COMMAND, 0, n2_get_error_code);
    N2_INSTALL_COMMAND(N2_ORD_GET_VERSION, N2_TAG_RQU_COMMAND, 0, n2_get_version);

    N2_INSTALL_COMMAND(N2_ORD_READ_KEYCHIP_ID, N2_TAG_RQU_AUTH_COMMAND, 4, n2_read_keychip_id);

    N2_INSTALL_COMMAND(N2_ORD_ENCRYPT_WITH_GKEY, N2_TAG_RQU_AUTH_COMMAND, 17, n2_encrypt_with_gkey);
    N2_INSTALL_COMMAND(N2_ORD_DECRYPT_WITH_GKEY, N2_TAG_RQU_AUTH_COMMAND, 17, n2_decrypt_with_gkey);
    N2_INSTALL_COMMAND(N2_ORD_ADD_PLAY_COUNT, N2_TAG_RQU_AUTH_COMMAND, 1, n2_add_play_count);
    N2_INSTALL_COMMAND(N2_ORD_READ_PLAY_COUNT, N2_TAG_RQU_AUTH_COMMAND, 0, n2_read_play_count);

    N2_INSTALL_COMMAND(N2_ORD_GET_RANDOM_VALUE, N2_TAG_RQU_AUTH_COMMAND, 0, n2_get_random_value);
    N2_INSTALL_COMMAND(N2_ORD_ENCRYPT_WITH_SKEY, N2_TAG_RQU_AUTH_COMMAND, 17, n2_encrypt_with_skey);
    N2_INSTALL_COMMAND(N2_ORD_DECRYPT_WITH_SKEY, N2_TAG_RQU_AUTH_COMMAND, 17, n2_decrypt_with_skey);
}

void do_n2_command(WORD tag, WORD paramSize, WORD command, LPBYTE data) {
    n2_install_commands();

    log_info("smb-n2", "Processing command: %04x/%04x (%d bytes)", tag, command, paramSize);

    ZeroMemory(n2_out_buffer, sizeof n2_out_buffer);

    // We nede to sign with the old key, so need a copy
    BYTE auth_key[20];
    memcpy(auth_key, n2_auth_key, sizeof auth_key);

    WORD result = N2_SUCCESS;
    WORD bodyLength = 0;

    WORD expectedExtraData = N2_CHECKSUM_SIZE + N2_HEADER_SIZE;
    if (tag == N2_TAG_RQU_AUTH_COMMAND) expectedExtraData += N2_AUTH_SIZE;

    n2_command handler = N2_COMMANDS[command & 0xff];
    if (handler.handler == NULL) {
        result = N2_BAD_ORDINAL;
    } else if (handler.tag != tag) {
        result = N2_BAD_TAG;
    } else if (handler.paramSize + expectedExtraData != paramSize) {
        result = N2_BAD_DATASIZE;
    } else {
        result = (*handler.handler)(paramSize, n2_in_buffer + N2_HEADER_SIZE,
                                    n2_out_buffer + N2_HEADER_SIZE, &bodyLength);
    }

    WORD paramSizeOut = bodyLength + expectedExtraData;
    ((PWORD)n2_out_buffer)[0] = fix_endian(tag + 2);
    ((PWORD)n2_out_buffer)[1] = fix_endian(paramSizeOut);
    ((PWORD)n2_out_buffer)[2] = fix_endian(result);

    EVP_MD_CTX* ctx;
    unsigned int outlen;

    WORD fixed_command = fix_endian(command);

    // Calculate a SHA1 of the packet, and append it
    ctx = EVP_MD_CTX_create();
    EVP_DigestInit(ctx, EVP_sha1());
    EVP_DigestUpdate(ctx, n2_out_buffer, N2_HEADER_SIZE);
    EVP_DigestUpdate(ctx, &fixed_command, 2);
    EVP_DigestUpdate(ctx, n2_out_buffer + N2_HEADER_SIZE, bodyLength);
    EVP_DigestFinal_ex(ctx, n2_out_buffer + N2_HEADER_SIZE + bodyLength, &outlen);
    EVP_MD_CTX_destroy(ctx);

    if (tag == N2_TAG_RQU_AUTH_COMMAND) {
        BYTE crypto_buffer[N2_CHECKSUM_SIZE];

        // Calculate a new SHA1 of the packet, including the SHA1 we just appeneded
        ctx = EVP_MD_CTX_create();
        EVP_DigestInit(ctx, EVP_sha1());
        EVP_DigestUpdate(ctx, n2_out_buffer, N2_HEADER_SIZE);
        EVP_DigestUpdate(ctx, &fixed_command, 2);
        EVP_DigestUpdate(ctx, n2_out_buffer + N2_HEADER_SIZE, bodyLength);
        EVP_DigestFinal_ex(ctx, crypto_buffer, &outlen);
        EVP_MD_CTX_destroy(ctx);

        // Calculate an HMAC, and append it
        HMAC_CTX hmac_ctx;
        HMAC_CTX_init(&hmac_ctx);
        HMAC_Init_ex(&hmac_ctx, auth_key, sizeof auth_key, EVP_sha1(), NULL);

        // SHA1(header | data)
        HMAC_Update(&hmac_ctx, crypto_buffer, sizeof crypto_buffer);
        // SHA1(header | auth | packet))
        HMAC_Update(&hmac_ctx, n2_out_buffer + N2_HEADER_SIZE + bodyLength, N2_CHECKSUM_SIZE);
        // Request nonce
        HMAC_Update(&hmac_ctx, n2_in_buffer + paramSize - N2_AUTH_SIZE - N2_CHECKSUM_SIZE,
                    N2_CHECKSUM_SIZE);

        HMAC_Final(&hmac_ctx, n2_out_buffer + N2_HEADER_SIZE + bodyLength + N2_CHECKSUM_SIZE,
                   &outlen);
        HMAC_CTX_cleanup(&hmac_ctx);
    }
}

BOOL smbus_N2_write(ich9_cmd_t cmd, WORD code, BYTE nbytes, LPBYTE data) {
    static WORD tag;
    static WORD paramsize = 0xffff;
    static WORD command;

    switch (cmd) {
        case ICH9_CMD_BLOCK: {
            switch (code >> 8) {
                case 0xcc: {
                    // One extra byte of data is stuffed into the command code
                    n2_in_buffer[n2_in_pointer++] = code & 0xff;
                    memcpy(n2_in_buffer + n2_in_pointer, data, nbytes);
                    n2_in_pointer += nbytes;

                    if (paramsize == 0xffff &&
                        n2_in_pointer >= N2_PARAMSIZE_OFFSET + N2_PARAMSIZE_SIZE) {
                        paramsize = ((PWORD)(n2_in_buffer + N2_PARAMSIZE_OFFSET))[0];
                        paramsize = fix_endian(paramsize);
                    }

                    if (n2_in_pointer >= paramsize) {
                        tag = ((PWORD)(n2_in_buffer + N2_TAG_OFFSET))[0];
                        tag = fix_endian(tag);
                        command = ((PWORD)(n2_in_buffer + N2_COMMAND_OFFSET))[0];
                        command = fix_endian(command);

                        do_n2_command(tag, paramsize, command, &n2_in_buffer[N2_HEADER_SIZE]);
                        paramsize = 0xffff;
                        n2_in_pointer = n2_out_pointer = 0;
                    }

                    return TRUE;
                }
            }
            log_error("smb-n2", "Unsupported command code: %04x (%d bytes)", code, nbytes);
            return FALSE;
        }
        case ICH9_CMD_I2C_READ: {
            switch (code) {
                case 0xc3: {
                    memcpy(data, n2_out_buffer + n2_out_pointer, nbytes);
                    n2_out_pointer += nbytes;
                    return TRUE;
                }
            }
            log_error("smb-n2", "Unsupported i2c command code: %04x (%d bytes)", code, nbytes);
            return FALSE;
        }
    }
    log_error("smb-n2", "Unsupported write mode: %01x (%02x)", cmd, code);
    return FALSE;
}

BOOL smbus_N2_read(ich9_cmd_t cmd, WORD code, BYTE nbytes, BYTE* data) {
    log_error("smb-n2", "Unsupported read mode: %01x (%02x)", cmd, code);
    return FALSE;
}
