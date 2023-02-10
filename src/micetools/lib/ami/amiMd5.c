#include "amiMd5.h"

#include <Windows.h>

static unsigned char rtable[4][4] = {
    { 7, 12, 17, 22 },
    { 5, 9, 14, 20 },
    { 4, 11, 16, 23 },
    { 6, 10, 15, 21 },
};
static unsigned int md5_init[4] = {
    0x67452301,
    0xefcdab89,
    0x98badcfe,
    0x10325476,
};
static unsigned int sines[] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
};

#define ROT_LEFT(x, n) (((x) << (n)) | ((x >> (32 - (n)))))
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

void amiMd5Init(AmiMd5 *md5) {
    ZeroMemory(md5, sizeof *md5);
    md5->m_state[0] = md5_init[0];
    md5->m_state[1] = md5_init[1];
    md5->m_state[2] = md5_init[2];
    md5->m_state[3] = md5_init[3];
}

void amiMd5Transform(AmiMd5 *md5, unsigned char *buffer) {
    unsigned int a = md5->m_state[0];
    unsigned int b = md5->m_state[1];
    unsigned int c = md5->m_state[2];
    unsigned int d = md5->m_state[3];

    unsigned int *uBuffer = (unsigned int *)buffer;

    unsigned int tmp, i, j;

    for (i = 0; i < 16; i++) {
        tmp = a + F(b, c, d) + uBuffer[i] + sines[i];
        tmp = ROT_LEFT(tmp, rtable[0][i & 3]) + b;
        a = d;
        d = c;
        c = b;
        b = tmp;
    }

    for (i = 0, j = 1; i < 16; i++, j += 5) {
        tmp = a + G(b, c, d) + uBuffer[j & 15] + sines[i + 16];
        tmp = ROT_LEFT(tmp, rtable[1][i & 3]) + b;
        a = d;
        d = c;
        c = b;
        b = tmp;
    }

    for (i = 0, j = 5; i < 16; i++, j += 3) {
        tmp = a + H(b, c, d) + uBuffer[j & 15] + sines[i + 32];
        tmp = ROT_LEFT(tmp, rtable[2][i & 3]) + b;
        a = d;
        d = c;
        c = b;
        b = tmp;
    }

    for (i = 0, j = 0; i < 16; i++, j += 7) {
        tmp = a + I(b, c, d) + uBuffer[j & 15] + sines[i + 48];
        tmp = ROT_LEFT(tmp, rtable[3][i & 3]) + b;
        a = d;
        d = c;
        c = b;
        b = tmp;
    }

    md5->m_state[0] += a;
    md5->m_state[1] += b;
    md5->m_state[2] += c;
    md5->m_state[3] += d;
}
void amiMd5Update(AmiMd5 *md5, unsigned int nBytes, unsigned char *buffer) {
    int buflen = md5->m_length & 63;
    md5->m_length += nBytes;

    if (buflen + nBytes < 64) {
        memcpy(md5->m_buffer + buflen, buffer, nBytes);
        buflen += nBytes;
        return;
    }

    memcpy(md5->m_buffer + buflen, buffer, 64 - buflen);
    amiMd5Transform(md5, md5->m_buffer);
    buffer += 64 - buflen;
    nBytes -= 64 - buflen;
    while (nBytes >= 64) {
        amiMd5Transform(md5, buffer);
        buffer += 64;
        nBytes -= 64;
    }
    memcpy(md5->m_buffer, buffer, nBytes);
}

void amiMd5Finalise(AmiMd5 *md5) {
    int buflen = md5->m_length & 63;
    md5->m_buffer[buflen++] = 0x80;
    memset(md5->m_buffer + buflen, 0, 64 - buflen);
    if (buflen > 56) {
        amiMd5Transform(md5, md5->m_buffer);
        memset(md5->m_buffer, 0, 64);
        buflen = 0;
    }

    *(unsigned int *)(md5->m_buffer + 56) = 8 * md5->m_length;
    *(unsigned int *)(md5->m_buffer + 60) = 0;
    amiMd5Transform(md5, md5->m_buffer);
    for (int i = 0; i < 4; i++) ((unsigned int *)md5->m_digest)[i] = md5->m_state[i];
}
