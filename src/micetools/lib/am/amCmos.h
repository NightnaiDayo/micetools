#pragma once

#include <Windows.h>

#include "../../dll/smbus.h"
#include "../_am.h"
#include "./amPlatform.h"

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

typedef struct AM_CMOS {
    BOOL m_bInit;
    HANDLE m_hMutex;
    HANDLE m_hDriver;
    AM_PLATFORM_CMOS_PARAM m_Param;
} AM_CMOS;

AM_CMOS_STATUS amCmosInit(AM_PLATFORM_CMOS_PARAM* lpParam);
AM_CMOS_STATUS amCmosExit(void);

AM_CMOS_STATUS amCmosReadByteInRearpart(BYTE addr, LPBYTE lpValue);
AM_CMOS_STATUS amCmosWriteByteInRearpart(BYTE addr, BYTE value);

AM_CMOS_STATUS amCmosSetPartition(BYTE addr, BYTE bitOffset, BYTE mbrCount, BYTE partition);

AM_CMOS_STATUS amCmosSetPartition_0(byte partition);
AM_CMOS_STATUS amCmosSetPartition_1(byte partition);
