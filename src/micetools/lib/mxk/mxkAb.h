#pragma once
#include <Windows.h>
#include <stdbool.h>

#include "../../segastructs.h"
#include "../ami/ami.h"
#include "mxkDefs.h"

typedef struct {
    bool m_useFlash;
    bool m_cacheDirty;
    AM_APPBOOT_256 m_Cache;
} APP_BOOT;
extern APP_BOOT AppBoot;

MXK_STATUS mxkGetAppBootInfo(void);

MXK_STATUS mxkAbSystemFlag(MXK_CACHE cache, unsigned char* systemFlag, unsigned char* err);
MXK_STATUS mxkAbGameId(MXK_CACHE cache, char* gameId, unsigned char* err);
