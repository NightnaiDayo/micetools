#pragma once

#include <Windows.h>

#include "../../dll/smbus.h"
#include "../_am.h"

AM_LIB_H_HEADER(amCmos, AM_CMOS)

typedef enum {
    AM_CMOS_STATUS_OK = 0,
    AM_CMOS_STATUS_NG = -1,
    AM_CMOS_STATUS_ERR_INVALID_PARAM = -2,
    AM_CMOS_STATUS_ERR_NO_INIT = -3,
    AM_CMOS_STATUS_ERR_ALREADY_INIT = -4,
    AM_CMOS_STATUS_ERR_SYS = -5,
    AM_CMOS_STATUS_ERR_OPEN_FAILED = -6,
    AM_CMOS_STATUS_ERR_LOCK = -7,
} AM_CMOS_STATUS;

typedef struct AM_CMOS_CONFIG {
    BYTE index[4];
    BYTE m_MbrCount;
} AM_CMOS_CONFIG;

typedef struct AM_CMOS {
    BOOL m_bInit;
    HANDLE m_hMutex;
    HANDLE m_hCmos;
    AM_CMOS_CONFIG m_Config;
} AM_CMOS;


AM_CMOS_STATUS amCmosInit(AM_CMOS_CONFIG* lpConfig);
AM_CMOS_STATUS amCmosExit(void);

AM_CMOS_STATUS amCmosReadByteInRearpart(BYTE param_1, LPBYTE param_2);
void amCmosWriteByteInRearpart(void);
void amCmosSetPartition(void);
