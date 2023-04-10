#pragma once

#include <Windows.h>

#include "../../segastructs.h"

#pragma pack(push, 1)
typedef struct _AM_KCF {
    AM_APPBOOT m_Header;
    BYTE m_AppData[216];
    BYTE m_Seed[16];
    BYTE m_Key[16];
    BYTE m_Iv[16];
    BYTE Unk[128];
} AM_KCF, *PAM_KCF;
#pragma pack(pop)

BOOL MiceParseKcf(LPCSTR lpFileName, PAM_KCF pKcf);
