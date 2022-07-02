unsigned int CRC_TABLE[256];
unsigned int CTC_TABLE_BUILT = 0;

#define CRC32_POLYNOMIAL 0xedb88320

void crc32_build_table() {
    if (CTC_TABLE_BUILT) return;
    CTC_TABLE_BUILT = 1;

    for (int i = 0; i < 256; i++) {
        unsigned int value = (~i & 1) - 1 & CRC32_POLYNOMIAL;
        value = (((int)i >> 1 ^ ~value) & 1) - 1 & CRC32_POLYNOMIAL ^ value >> 1;
        value = (((int)i >> 2 ^ ~value) & 1) - 1 & CRC32_POLYNOMIAL ^ value >> 1;
        value = (((int)i >> 3 ^ ~value) & 1) - 1 & CRC32_POLYNOMIAL ^ value >> 1;
        value = (((int)i >> 4 ^ ~value) & 1) - 1 & CRC32_POLYNOMIAL ^ value >> 1;
        value = (((int)i >> 5 ^ ~value) & 1) - 1 & CRC32_POLYNOMIAL ^ value >> 1;
        value = (((int)i >> 6 ^ ~value) & 1) - 1 & CRC32_POLYNOMIAL ^ value >> 1;
        CRC_TABLE[i] = (((int)i >> 7 ^ ~value) & 1) - 1 & CRC32_POLYNOMIAL ^ value >> 1;
    }
}

unsigned int crc32(int length, unsigned char *data, unsigned int initial) {
    unsigned int value = ~initial;
    while (length--) {
        value = value >> 8 ^ CRC_TABLE[(*data ^ value) & 0xff];
        data++;
    }
    return ~value;
}
