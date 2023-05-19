#pragma once

#include <stdint.h>

#pragma pack(push, 1)
typedef struct AM_APPBOOT {
    uint32_t m_Crc;
    uint32_t m_Format;
    char m_GameId[4];
    uint8_t m_Region;
    uint8_t m_ModelType;
    uint8_t m_SystemFlag;
    uint8_t Rsv0f;
    char m_PlatformId[3];
    uint8_t m_DvdFlag;
    uint32_t m_NetworkAddr;
} AM_APPBOOT;

typedef struct AM_APPBOOT_128 {
    AM_APPBOOT m_Header;
    uint8_t Rsv18[88];
    uint8_t m_Seed[16];
} AM_APPBOOT_128;

typedef struct AM_APPBOOT_256 {
    AM_APPBOOT m_Header;
    uint8_t Rsv18[216];
    uint8_t m_Seed[16];
} AM_APPBOOT_256;
#pragma pack(pop)
