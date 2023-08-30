#include "uint64.h"

bool UINT64IsZero(UINT64 *pU64) {
    int sum = 0;
    for (int i = 0; i < 8; i++) sum += ((LPBYTE)pU64)[i];
    return sum == 0;
}
UINT64 UINT64Add(UINT64 *pU64_1, UINT64 *pU16_2) {
    UINT64 res = 0;
    unsigned int carry = 0;
    for (int i = 7; i >= 0; --i) {
        carry += ((LPBYTE)pU64_1)[i] + ((LPBYTE)pU16_2)[i];
        ((LPBYTE)res)[i] = carry;
        carry >>= 8;
    }
    return res;
}
UINT32 UINT64ModUInt32(UINT64 *pU64, UINT32 u32) {
    UINT32 res = 0;
    for (int i = 0; i < 8; i++) res = ((res << 8) + ((LPBYTE)pU64)[i]) % u32;
    return res;
}
UINT64 UINT64DivideUInt32(UINT64 *pU64, UINT32 u32) {
    UINT64 res = 0;
    unsigned int carry = 0;
    for (int i = 0; i < 8; i++) {
        ((LPBYTE)&res)[i] = ((carry << 8) + ((LPBYTE)pU64)[i]) / u32;
        carry = ((carry << 8) + ((LPBYTE)pU64)[i]) % u32;
    }
    return res;
}
void UINT64ToA(char *lpStr, UINT64 *pU64) {
    char buf[64];
    int i = 0;
    char *tail = buf;

    UINT64 tmp;
    memcpy(&tmp, pU64, sizeof tmp);
    while (!UINT64IsZero(&tmp)) {
        *tail++ = UINT64ModUInt32(&tmp, 10) + '0';
        tmp = UINT64DivideUInt32(&tmp, 10);
        i++;
    }
    for (; i < 17; i++) *lpStr++ = '0';
    while (tail != buf) *lpStr++ = *--tail;
}

UINT64 IDDataToUINT64(LPBYTE lpIdData) {
    UINT64 res;
    memcpy(&res, lpIdData, sizeof res);
    return res;
}
