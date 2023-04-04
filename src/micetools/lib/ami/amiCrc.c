#include "amiCrc.h"

#define CRC32_POLYNOMIAL 0xedb88320

AM_LIB_C_HEADER(amiCrc, AMI_CRC)

void amiCrc32RCreateTable(uint32_t *table) {
    for (int i = 0; i < 256; i++) {
        uint32_t value = (~i & 1) - 1 & CRC32_POLYNOMIAL;
        value = ((i >> 1 ^ ~value) & 1) - 1 & CRC32_POLYNOMIAL ^ value >> 1;
        value = ((i >> 2 ^ ~value) & 1) - 1 & CRC32_POLYNOMIAL ^ value >> 1;
        value = ((i >> 3 ^ ~value) & 1) - 1 & CRC32_POLYNOMIAL ^ value >> 1;
        value = ((i >> 4 ^ ~value) & 1) - 1 & CRC32_POLYNOMIAL ^ value >> 1;
        value = ((i >> 5 ^ ~value) & 1) - 1 & CRC32_POLYNOMIAL ^ value >> 1;
        value = ((i >> 6 ^ ~value) & 1) - 1 & CRC32_POLYNOMIAL ^ value >> 1;
        table[0] = ((i >> 7 ^ ~value) & 1) - 1 & CRC32_POLYNOMIAL ^ value >> 1;
        table++;
    }
}

uint32_t amiCrc32RGet(uint32_t *table, int length, void *data, uint32_t initial) {
    uint32_t value = ~initial;
    while (length > 0) {
        value = value >> 8 ^ table[(((unsigned char *)data)[0] ^ value) & 0xff];
        length += -1;
        ((unsigned char *)data)++;
    }
    return ~value;
}
