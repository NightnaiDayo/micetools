#include "_ds_sha.h"

#define KTN(n) ((n < 20) ? 0x5a827999 : (n < 40) ? 0x6ed9eba1 : (n < 60) ? 0x8f1bbcdc : 0xca62c1d6)

#define NLF(B, C, D, n)                         \
    ((n < 20)   ? ((B & C) | ((~B) & D))        \
     : (n < 40) ? (B ^ C ^ D)                   \
     : (n < 60) ? ((B & C) | (B & D) | (C & D)) \
                : (B ^ C ^ D))

/**
 * @brief This is almost identical to a typical SHA1 implementation, except the final step involving
 * the addition of constants is ommitted as per the datasheet.
 */
void ComputeDallasSha(unsigned char *MT, long *A, long *B, long *C, long *D, long *E) {
    unsigned long MTword[80];
    int i;
    long ShftTmp;
    long Temp;

    for (i = 0; i < 16; i++)
        MTword[i] =
            (MT[i * 4] << 24) | (MT[i * 4 + 1] << 16) | (MT[i * 4 + 2] << 8) | MT[i * 4 + 3];

    for (i = 16; i < 80; i++) {
        ShftTmp = MTword[i - 3] ^ MTword[i - 8] ^ MTword[i - 14] ^ MTword[i - 16];
        MTword[i] = ((ShftTmp << 1) & 0xfffffffe) | ((ShftTmp >> 31) & 0x00000001);
    }

    *A = 0x67452301;
    *B = 0xefcdab89;
    *C = 0x98badcfe;
    *D = 0x10325476;
    *E = 0xc3d2e1f0;

    for (i = 0; i < 80; i++) {
        ShftTmp = ((*A << 5) & 0xffffffe0) | ((*A >> 27) & 0x0000001f);
        Temp = NLF(*B, *C, *D, i) + *E + KTN(i) + MTword[i] + ShftTmp;
        *E = *D;
        *D = *C;
        *C = ((*B << 30) & 0xc0000000) | ((*B >> 2) & 0x3fffffff);
        *B = *A;
        *A = Temp;
    }
}
