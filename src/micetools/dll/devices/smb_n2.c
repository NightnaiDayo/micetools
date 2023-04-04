#include "smb_n2.h"

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#include "../../lib/mxk/mxk.h"
#include "../../sysconf.h"
#include "_devices.h"

LOG_FACILITY _lf = {
    .m_name = "smb-n2",
};
PLOG_FACILITY plf = &_lf;

#define N2_IO_BUFFER 512

BYTE n2_auth_level = 1;
BYTE n2_in_buffer[N2_IO_BUFFER];
WORD n2_in_pointer = 0;
BYTE n2_out_buffer[N2_IO_BUFFER];
WORD n2_out_pointer = 0;

WORD n2_error_code = 0;

BYTE n2_auth_key[20] = { 0x96, 0xed, 0x31, 0xb2, 0x28, 0x71, 0x05, 0xa5, 0xa3, 0x30,
                         0x54, 0x0f, 0x25, 0xbe, 0xd8, 0x51, 0xa5, 0xc8, 0x36, 0x21 };

BYTE n2_session_salt[20];
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
    .m_KeyId = KEY_ID,
    .m_Appboot = {
        .m_Format = 1,
        .m_GameId = GAME_ID,
        .m_Region = 0xff,
        .m_ModelType = 2,
        .m_SystemFlag = 0x24,
        .m_PlatformId = HW_ID,
        .m_DvdFlag = 1,
        .m_NetworkAddr =
            (192 << 0) | (168 << 8) | (103 << 16) | (0 << 24),
    },
};

WORD n2_enable_session(WORD paramSize, LPBYTE dataIn, LPBYTE dataOut, LPWORD nOut) {
    *nOut = 20;

    RAND_bytes(n2_session_salt, sizeof n2_session_salt);
    memcpy(dataOut, n2_session_salt, sizeof n2_session_salt);

    log_misc(plf, "Session open");
    return N2_SUCCESS;
}
WORD n2_set_auth_key(WORD paramSize, LPBYTE dataIn, LPBYTE dataOut, LPWORD nOut) {
    *nOut = 0;
    log_misc(plf, "Auth key set");

    BYTE pt[32];
    mxkCryptDecryptAes128CBC(n2_enc_key.key, n2_enc_key.iv, dataIn, pt, sizeof pt);
    memcpy(n2_auth_key, pt, sizeof n2_auth_key);

    n2_auth_level = 2;

    return N2_SUCCESS;
}
WORD n2_set_enc_key(WORD paramSize, LPBYTE dataIn, LPBYTE dataOut, LPWORD nOut) {
    *nOut = 0;
    log_misc(plf, "Enc key set");

    BYTE pt[32];
    mxkCryptDecryptAes128CBC(n2_enc_key.key, n2_enc_key.iv, dataIn, pt, sizeof pt);
    memcpy(n2_enc_key.key, pt, sizeof n2_enc_key.key);
    memcpy(n2_enc_key.iv, pt + sizeof n2_enc_key.key, sizeof n2_enc_key.iv);

    n2_auth_level = 3;

    return N2_SUCCESS;
}
WORD n2_get_auth_level(WORD paramSize, LPBYTE dataIn, LPBYTE dataOut, LPWORD nOut) {
    *nOut = 1;
    dataOut[0] = n2_auth_level;
    log_misc(plf, "Auth level get");
    return N2_SUCCESS;
}
WORD n2_get_error_code(WORD paramSize, LPBYTE dataIn, LPBYTE dataOut, LPWORD nOut) {
    *nOut = 2;
    ((PWORD)dataOut)[0] = fix_endian(n2_error_code);
    log_misc(plf, "Error get");
    return N2_SUCCESS;
}
WORD n2_read_keychip_id(WORD paramSize, LPBYTE dataIn, LPBYTE dataOut, LPWORD nOut) {
    DWORD nbytes = (dataIn[0] << 24) | (dataIn[1] << 16) | (dataIn[2] << 8) | dataIn[3];

    *nOut = (nbytes & 0xff) + 2;

    n2_keychip_info.m_Appboot.m_Crc = amiCrc32RCalc(sizeof n2_keychip_info.m_Appboot - 4,
                                                    (LPBYTE)&n2_keychip_info.m_Appboot + 4, 0);
    n2_keychip_info.m_Crc =
        amiCrc32RCalc(sizeof n2_keychip_info - 4, (LPBYTE)&n2_keychip_info + 4, 0);

    mxkCryptEncryptAes128CBC(n2_enc_key.key, n2_enc_key.iv, dataOut + 2, (LPBYTE)&n2_keychip_info,
                             nbytes);

    log_misc(plf, "Read keychip ID: %08x", nbytes);
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

    for (size_t i = 0; i < 17; i++) printf(" %02x", dataIn[i]);
    puts("");
    for (size_t i = 0; i < 17; i++) printf(" %02x", temp[i]);
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
    EVP_MD_CTX* ctx;
    unsigned int outlen;

    n2_install_commands();

    log_info(plf, "Processing command: %04x/%04x (%d bytes)", tag, command, paramSize);

    ZeroMemory(n2_out_buffer, sizeof n2_out_buffer);

    // We need to sign with the old key, so need a copy
    BYTE auth_key[20];
    memcpy(auth_key, n2_auth_key, sizeof auth_key);

    WORD result = N2_SUCCESS;
    WORD bodyLength = 0;

    WORD expectedExtraData = N2_CHECKSUM_SIZE + N2_HEADER_SIZE;
    if (tag == N2_TAG_RQU_AUTH_COMMAND) expectedExtraData += N2_AUTH_SIZE;

    unsigned char sha1sum[20];
    ctx = EVP_MD_CTX_create();
    EVP_DigestInit(ctx, EVP_sha1());
    EVP_DigestUpdate(ctx, n2_in_buffer, N2_HEADER_SIZE + paramSize - expectedExtraData);
    EVP_DigestFinal_ex(ctx, sha1sum, &outlen);
    EVP_MD_CTX_destroy(ctx);

    HMAC_CTX hmac_ctx;
    if (tag == N2_TAG_RQU_COMMAND) {
        if (memcmp(sha1sum, n2_in_buffer + paramSize - N2_CHECKSUM_SIZE, N2_CHECKSUM_SIZE) != 0) {
            log_error(plf, "SHA1Sum chek failed!");
            result = N2_SUMFAIL;
        }
    } else {
        unsigned char nonce[20];
        memcpy_s(nonce, 20, n2_in_buffer + paramSize - N2_CHECKSUM_SIZE - N2_AUTH_SIZE, 20);
        unsigned char hmac[20];

        HMAC_CTX_init(&hmac_ctx);
        HMAC_Init_ex(&hmac_ctx, auth_key, sizeof auth_key, EVP_sha1(), NULL);
        HMAC_Update(&hmac_ctx, sha1sum, N2_CHECKSUM_SIZE);
        HMAC_Update(&hmac_ctx, n2_session_salt, N2_CHECKSUM_SIZE);
        HMAC_Update(&hmac_ctx, nonce, N2_CHECKSUM_SIZE);
        HMAC_Final(&hmac_ctx, hmac, &outlen);
        HMAC_CTX_cleanup(&hmac_ctx);

        if (memcmp(hmac, n2_in_buffer + paramSize - N2_AUTH_SIZE, N2_AUTH_SIZE) != 0) {
            log_error(plf, "HMAC check failed!");
            result = N2_AUTHFAIL;
        }
    }

    if (result == N2_SUCCESS) {
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
    }

    WORD paramSizeOut = bodyLength + expectedExtraData;
    ((PWORD)n2_out_buffer)[0] = fix_endian(tag + 2);
    ((PWORD)n2_out_buffer)[1] = fix_endian(paramSizeOut);
    ((PWORD)n2_out_buffer)[2] = fix_endian(result);

    WORD fixed_command = fix_endian(command);

    // Calculate a SHA1 of the packet, and append it
    ctx = EVP_MD_CTX_create();
    EVP_DigestInit(ctx, EVP_sha1());
    EVP_DigestUpdate(ctx, n2_out_buffer, N2_HEADER_SIZE);
    EVP_DigestUpdate(ctx, &fixed_command, 2);
    EVP_DigestUpdate(ctx, n2_out_buffer + N2_HEADER_SIZE, bodyLength);
    EVP_DigestFinal_ex(ctx, n2_out_buffer + N2_HEADER_SIZE + bodyLength, &outlen);
    EVP_MD_CTX_destroy(ctx);

    // At this point: packet = header | data | SHA(header | command | data)

    if (tag == N2_TAG_RQU_AUTH_COMMAND) {
        // TODO: Further investigation might be needed here.
        // The following code checks out against mxkeychip, but repeating the
        // packet sum twice feels wrong.

        // Calculate an HMAC, and append it
        HMAC_CTX_init(&hmac_ctx);
        HMAC_Init_ex(&hmac_ctx, auth_key, sizeof auth_key, EVP_sha1(), NULL);

        // SHA1(header | command | data)
        HMAC_Update(&hmac_ctx, n2_out_buffer + N2_HEADER_SIZE + bodyLength, N2_CHECKSUM_SIZE);
        HMAC_Update(&hmac_ctx, n2_out_buffer + N2_HEADER_SIZE + bodyLength, N2_CHECKSUM_SIZE);
        // Request nonce
        HMAC_Update(&hmac_ctx, n2_in_buffer + paramSize - N2_AUTH_SIZE - N2_CHECKSUM_SIZE,
                    N2_CHECKSUM_SIZE);

        HMAC_Final(&hmac_ctx, n2_out_buffer + N2_HEADER_SIZE + bodyLength + N2_CHECKSUM_SIZE,
                   &outlen);
        HMAC_CTX_cleanup(&hmac_ctx);

        // Update session salt
        memcpy_s(n2_session_salt, sizeof n2_session_salt,
                 n2_out_buffer + N2_HEADER_SIZE + bodyLength, N2_CHECKSUM_SIZE);
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
            log_error(plf, "Unsupported command code: %04x (%d bytes)", code, nbytes);
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
            log_error(plf, "Unsupported i2c command code: %04x (%d bytes)", code, nbytes);
            return FALSE;
        }
    }
    log_error(plf, "Unsupported write mode: %01x (%02x)", cmd, code);
    return FALSE;
}

BOOL smbus_N2_read(ich9_cmd_t cmd, WORD code, BYTE nbytes, BYTE* data) {
    // TODO: Unaligned mxk reads poke into this for the final byte!
    log_error(plf, "Unsupported read mode: %01x (%02x)", cmd, code);
    return FALSE;
}
