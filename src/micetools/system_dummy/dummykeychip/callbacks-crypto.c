#include <openssl/evp.h>
#include <openssl/pem.h>

#include "../../lib/util/hex.h"
#include "callbacks.h"

// dummykeychip is not intended to be used in a production system. Therefore, we need only pick
// values here that will allow us to run game data. The only game I have observed requiring specific
// encryption and decryption results is Border Break, and as such those are the keys loaded here. If
// other games end up doing this, a more thorough alternative will be implemented.
static const unsigned char seed[16] = { 0xdb, 0x86, 0x37, 0x3a, 0x5a, 0x2e, 0x05, 0xb9,
                                        0x63, 0xc2, 0x82, 0xd7, 0x89, 0x12, 0x8d, 0x0d };
static const unsigned char key[16] = { 0x6a, 0xcb, 0x8d, 0xc9, 0x00, 0x49, 0x92, 0x7a,
                                       0xea, 0xcf, 0x71, 0xc9, 0x74, 0x0b, 0x6f, 0xf9 };
static const unsigned char iv[16] = { 0xa4, 0x7a, 0x66, 0x8e, 0xc0, 0xda, 0x67, 0x5e,
                                      0x10, 0xe3, 0xa3, 0xeb, 0xe5, 0x32, 0x8c, 0xf0 };

EVP_CIPHER_CTX* ctxEnc = NULL;
EVP_CIPHER_CTX* ctxDec = NULL;

void mdkPcpAbSeed(pcpa_t* stream, void* data) {
    pcpaSetBinaryMode(stream, binary_mode_send);
    pcpaSetSendBinaryBuffer(stream, seed, 16);
    pcpaSetSendPacket(stream, AB_SEED, "0");
    pcpaAddSendPacket(stream, "port", "40107");
    pcpaAddSendPacket(stream, "size", "16");
}
void mdkPcpDsCompute(pcpa_t* stream, void* data) {
    // TODO: We could, and maybe should, scan for %s_Table.dat files, and use one if we find it
    pcpaSetSendPacket(stream, DS_COMPUTE, "");
    pcpaAddSendPacket(stream, "code", "54");
}
void mdkPcpSsdProof(pcpa_t* stream, void* data) {
    pcpaSetSendPacket(stream, SSD_PROOF, "0");
    pcpaAddSendPacket(stream, "code", "54");
}
void mdkPcpSsdHostProof(pcpa_t* stream, void* data) {
    pcpaSetSendPacket(stream, SSD_HOSTPROOF, "0");
    pcpaAddSendPacket(stream, "code", "54");
}
void mdkPcpEncrypt(pcpa_t* stream, void* data) {
    char* ptHex = pcpaGetCommand(stream, KC_ENCRYPT);
    unsigned char pt[16];
    memset(pt, 0, sizeof(pt));
    hex_to_bin(ptHex, pt, strlen(ptHex) > 32 ? 32 : strlen(ptHex));

    if (ctxEnc == NULL) {
        ctxEnc = EVP_CIPHER_CTX_new();
        EVP_CipherInit_ex(ctxEnc, EVP_aes_128_cbc(), NULL, key, iv, 1);
    }

    int outl;
    unsigned char ct[16];
    memset(ct, 0, sizeof(ct));
    EVP_EncryptUpdate(ctxEnc, ct, &outl, pt, 16);

    char ctHex[33];
    bin_to_hex(ctHex, ct, 16);

    pcpaSetSendPacket(stream, KC_ENCRYPT, ctHex);
}

unsigned char workingIv[16];
void mdkPcpDecrypt(pcpa_t* stream, void* data) {
    char* ctHex = pcpaGetCommand(stream, KC_DECRYPT);
    unsigned char ct[16];
    memset(ct, 0, sizeof(ct));
    hex_to_bin(ctHex, ct, strlen(ctHex) > 32 ? 32 : strlen(ctHex));

    if (ctxDec == NULL) {
        ctxDec = EVP_CIPHER_CTX_new();
        memcpy(workingIv, iv, 16);
    } else {
        EVP_CIPHER_CTX_cleanup(ctxDec);
    }
    EVP_CipherInit_ex(ctxDec, EVP_aes_128_cbc(), NULL, key, workingIv, 0);
    memcpy(workingIv, ct, 16);

    int outl = 0;
    unsigned char pt[16];
    memset(pt, 0, sizeof(pt));
    EVP_DecryptUpdate(ctxDec, pt, &outl, ct, 16);

    char ptHex[33];
    bin_to_hex(ptHex, pt, 16);

    pcpaSetSendPacket(stream, KC_DECRYPT, ptHex);
}
void mdkPcpSetIv(pcpa_t* stream, void* data) {
    if (ctxEnc != NULL) {
        EVP_CIPHER_CTX_cleanup(ctxEnc);
    } else {
        ctxEnc = EVP_CIPHER_CTX_new();
    }
    EVP_CipherInit_ex(ctxEnc, EVP_aes_128_cbc(), NULL, key, iv, 1);

    memcpy(workingIv, iv, 16);

    pcpaSetSendPacket(stream, KC_SETIV, "1");
}