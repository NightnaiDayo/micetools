#include <openssl/evp.h>
#include <openssl/pem.h>

#include "../../lib/util/hex.h"
#include "callbacks.h"

void mxkPcpDsCompute(pcpa_t* stream, void* data) { pcpaSetSendPacket(stream, "code", "54"); }
void mxkPcpSsdProof(pcpa_t* stream, void* data) { pcpaSetSendPacket(stream, "code", "54"); }
void mxkPcpSsdHostProof(pcpa_t* stream, void* data) {}

EVP_CIPHER_CTX* ctxEnc = NULL;
EVP_CIPHER_CTX* ctxDec = NULL;

void mxkPcpEncrypt(pcpa_t* stream, void* data) {
    char* ptHex = pcpaGetCommand(stream, KC_ENCRYPT);
    unsigned char pt[16];
    memset(pt, 0, sizeof(pt));
    hex_to_bin(ptHex, pt, strlen(ptHex) > 32 ? 32 : strlen(ptHex));

    if (ctxEnc == NULL) {
        ctxEnc = EVP_CIPHER_CTX_new();
        EVP_CipherInit_ex(ctxEnc, EVP_aes_128_cbc(), NULL, mxkKcfConfig.m_Key, mxkKcfConfig.m_Iv,
                          1);
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
void mxkPcpDecrypt(pcpa_t* stream, void* data) {
    char* ctHex = pcpaGetCommand(stream, KC_DECRYPT);
    unsigned char ct[16];
    memset(ct, 0, sizeof(ct));
    hex_to_bin(ctHex, ct, strlen(ctHex) > 32 ? 32 : strlen(ctHex));

    if (ctxDec == NULL) {
        ctxDec = EVP_CIPHER_CTX_new();
        memcpy(workingIv, mxkKcfConfig.m_Iv, 16);
    } else {
        EVP_CIPHER_CTX_cleanup(ctxDec);
    }
    EVP_CipherInit_ex(ctxDec, EVP_aes_128_cbc(), NULL, mxkKcfConfig.m_Key, workingIv, 0);
    memcpy(workingIv, ct, 16);

    int outl = 0;
    unsigned char pt[16];
    memset(pt, 0, sizeof(pt));
    EVP_DecryptUpdate(ctxDec, pt, &outl, ct, 16);

    char ptHex[33];
    bin_to_hex(ptHex, pt, 16);

    pcpaSetSendPacket(stream, KC_DECRYPT, ptHex);
}
void mxkPcpSetIv(pcpa_t* stream, void* data) {
    if (ctxEnc != NULL) {
        EVP_CIPHER_CTX_cleanup(ctxEnc);
    } else {
        ctxEnc = EVP_CIPHER_CTX_new();
    }
    EVP_CipherInit_ex(ctxEnc, EVP_aes_128_cbc(), NULL, mxkKcfConfig.m_Key, mxkKcfConfig.m_Iv, 1);

    memcpy(workingIv, mxkKcfConfig.m_Iv, 16);

    pcpaSetSendPacket(stream, KC_SETIV, "1");
}
