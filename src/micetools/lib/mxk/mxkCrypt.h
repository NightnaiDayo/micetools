#pragma once
#include <Windows.h>

#include "../ami/ami.h"
#include "mxkDefs.h"

#define SHA1_SUM_SIZE 20
#define HMAC_SUM_SIZE 20
#define HMAC_KEY_SIZE 20

typedef char Sha1Sum_t[SHA1_SUM_SIZE];
typedef char (*PSha1Sum_t)[SHA1_SUM_SIZE];
typedef char HmacSum_t[HMAC_SUM_SIZE];
typedef char (*PHmacSum_t)[HMAC_SUM_SIZE];
typedef char HmacKey_t[HMAC_SUM_SIZE];
typedef char (*PHmacKey_t)[HMAC_SUM_SIZE];

void mxkSetKeyS(unsigned char* key_s);
void mxkSetKeyR(unsigned char* key_r);
void mxkSwapKeys();

MXK_STATUS mxkCryptInit(void);

int mxkCryptCalcHashWithHmacSha1(PHmacKey_t key, PHmacSum_t md, size_t* nbuffer,
                                 unsigned char* buffer, size_t nin);
void mxkCryptCalcHashWithSha1(unsigned char* data, size_t nbytes, PSha1Sum_t sum);

void mxkCryptCreateDigest(void);
void mxkCryptRsaSignVerify(void);

MXK_STATUS mxkCryptDecryptAes128CBC(const unsigned char* key, const unsigned char* iv,
                                    const void* ct, void* pt, size_t nbytes);
MXK_STATUS mxkCryptEncryptAes128CBC(const unsigned char* key, const unsigned char* iv, void* ct,
                                    const void* pt, size_t nbytes);

MXK_STATUS mxkCryptDecryptData(const unsigned char* ct, unsigned char* pt);
MXK_STATUS mxkCryptEncryptData(unsigned char* ct, const unsigned char* pt);

void mxkSign(void* buffer, size_t nbytes, unsigned char* signature);
void mxkSignValue(unsigned int value, unsigned char* signature);
