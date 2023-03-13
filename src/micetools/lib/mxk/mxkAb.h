#pragma once
#include <Windows.h>
#include <stdbool.h>

#include "../ami/ami.h"
#include "mxkDefs.h"

#pragma pack(push, 1)
typedef struct {
    DWORD crc;
    DWORD format;
    char game_id[4];
    BYTE region;
    BYTE model_type;
    BYTE system_flag;
    BYTE _;
    char platform_id[3];
    BYTE dvd_flag;
    DWORD network_addr;
    BYTE __[216];
    BYTE seed[16];
} appboot_t;
#pragma pack(pop)

typedef struct {
    bool m_useFlash;
    bool m_cacheDirty;
    appboot_t m_Cache;
} APP_BOOT;
extern APP_BOOT AppBoot;

MXK_STATUS mxkGetAppBootInfo(void);

MXK_STATUS mxkAbSystemFlag(MXK_CACHE cache, unsigned char* systemFlag, unsigned char* err);
MXK_STATUS mxkAbGameId(MXK_CACHE cache, char* gameId, unsigned char* err);
