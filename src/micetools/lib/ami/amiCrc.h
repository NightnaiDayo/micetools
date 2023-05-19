#pragma once

#include <stdint.h>

#include "../_am.h"

AM_LIB_H_HEADER(amiCrc, AMI_CRC)

typedef struct AMI_CRC {
    uint32_t m_init;
    uint32_t m_table[256];
} AMI_CRC;

void amiCrc32RCreateTable(uint32_t *table);
uint32_t amiCrc32RGet(uint32_t *table, int length, void *data, uint32_t initial);

#define amiCrc32RInit()                           \
    do {                                          \
        if (!amiCrc.m_init) {                     \
            amiCrc32RCreateTable(amiCrc.m_table); \
            amiCrc.m_init = 1;                    \
        }                                         \
    } while (0)
#define amiCrc32RCalc(length, data, initial) \
    amiCrc32RGet(amiCrc.m_table, (length), (data), (initial))
