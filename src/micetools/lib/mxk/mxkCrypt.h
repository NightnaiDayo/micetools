#pragma once
#include <Windows.h>

#include "../ami/ami.h"
#include "mxkDefs.h"

void mxkSetKeyS(unsigned char* key_s);
void mxkSetKeyR(unsigned char* key_r);
void mxkSwapKeys();

MXK_STATUS mxkCryptInit(void);

int mxkCryptCalcHashWithHmacSha1(void* key, unsigned char* md, size_t* nbuffer,
                                 unsigned char* buffer, size_t nin);
void mxkCryptCalcHashWithSha1(unsigned char* data, size_t nbytes, unsigned char* sum);

void mxkCryptCreateDigest(void);
void mxkCryptRsaSignVerify(void);

MXK_STATUS mxkCryptDecryptAes128CBC(const unsigned char* key, const unsigned char* iv,
                                    const unsigned char* ct, unsigned char* pt, size_t nbytes);
MXK_STATUS mxkCryptEncryptAes128CBC(const unsigned char* key, const unsigned char* iv,
                                    unsigned char* pt, const unsigned char* ct, size_t nbytes);

MXK_STATUS mxkCryptDecryptData(const unsigned char* ct, unsigned char* pt);
MXK_STATUS mxkCryptEncryptData(unsigned char* ct, const unsigned char* pt);

void mxkSign(void* buffer, size_t nbytes, unsigned char* signature);
void mxkSignValue(unsigned int value, unsigned char* signature);
