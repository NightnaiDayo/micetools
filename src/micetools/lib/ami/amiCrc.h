#pragma once

#include "../_am.h"

AM_LIB_H_HEADER(amiCrc, AMI_CRC)

typedef struct _AMI_CRC {
    unsigned int m_init;
    unsigned int m_table[256];
} AMI_CRC;

void amiCrc32RCreateTable(unsigned int *table);
unsigned int amiCrc32RGet(unsigned int *table, int length, unsigned char *data,
                          unsigned int initial);

#define amCrc32RCreateTable()                     \
    do {                                          \
        if (!amiCrc.m_init) {                     \
            amiCrc32RCreateTable(amiCrc.m_table); \
            amiCrc.m_init = 1;                    \
        }                                         \
    } while (0)
#define amCrc32RGet(length, data, initial) \
    amiCrc32RGet(amiCrc.m_table, (length), (data), (initial))
