#pragma once

#include <Windows.h>

#include "../_am.h"

AM_LIB_H_HEADER(amSram, AM_SRAM)

typedef struct _AM_SRAM {
    DWORD m_init;
    HANDLE m_handle;
    LARGE_INTEGER m_size;
    DWORD m_sectorSize;
    HANDLE m_mutex;
    int m_readTimeout;
    int m_writeTimeout;
    int Unk20;
    PSID m_sid;
} AM_SRAM;

typedef enum {
    AM_SRAM_STATUS_OK = 0,
    AM_SRAM_STATUS_NG = -1,
    AM_SRAM_STATUS_ERR_INVALID_PARAM = -2,
    AM_SRAM_STATUS_ERR_NO_INIT = -3,
    AM_SRAM_STATUS_ERR_ALREADY_INIT = -4,
    AM_SRAM_STATUS_ERR_PROTOCOL_VER = -5,
    AM_SRAM_STATUS_ERR_SYS = -6,
    AM_SRAM_STATUS_ERR_READ = -7,
    AM_SRAM_STATUS_ERR_WRITE = -8,
    AM_SRAM_STATUS_ERR_COPY = -9,
    AM_SRAM_STATUS_ERR_DOMAIN = -10,
    AM_SRAM_STATUS_ERR_GET_MUTEX = -11,
    AM_SRAM_STATUS_ERR_NOT_SUPPORT = -12,
} AM_SRAM_STATUS;

#define AM_SRAM_MAX_SECTOR_SIZE 0x800

AM_SRAM_STATUS amSramCreateMutex(AM_SRAM *device);
AM_SRAM_STATUS amSramInit(void);
AM_SRAM_STATUS amSramExit(void);

AM_SRAM_STATUS amSramReadInternal(AM_SRAM *device, LONG addressLow, LONG addressHigh, LPVOID buffer,
                                  DWORD nBytes);
AM_SRAM_STATUS amSramWriteInternal(AM_SRAM *device, LONG addressLow, LONG addressHigh,
                                   LPCVOID buffer, DWORD nBytes);

AM_SRAM_STATUS amSramRead(DWORD src, LPVOID pDst, DWORD size);
AM_SRAM_STATUS amSramWrite(LPCVOID pSrc, DWORD dst, DWORD size);

// TODO:

void amSramCopy(void);
void amSramGetSectorSize(void);
void amSramGetSize(void);
void amSramSetTimeout(void);
