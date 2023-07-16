#pragma once

#include <Windows.h>
#include <stdint.h>

#include "../_am.h"
#include "amPlatform.h"

AM_LIB_H_HEADER(amBackup, AM_BACKUP)

typedef struct AM_BACKUP {
    BOOL m_Init;
    uint32_t m_CrcTable[256];
} AM_BACKUP;

typedef enum AM_BACKUP_STATUS {
    AM_BACKUP_STATUS_OK = 0,
    AM_BACKUP_STATUS_NO_DEVICE = -21,
    AM_BACKUP_STATUS_INVALID_ADDR = -22,
    AM_BACKUP_STATUS_BAD_CRC = -23,
    AM_BACKUP_STATUS_BAD_ID = -24,
    AM_BACKUP_STATUS_INVALID_STORAGE = -25,
    AM_BACKUP_STATUS_INVALID_PARAM = -27,
    AM_BACKUP_STATUS_NO_INIT = -28,
    AM_BACKUP_STATUS_ALREADY_INIT = -29,
} AM_BACKUP_STATUS;

typedef struct AM_BACKUP_RECORD {
    AM_PLATFORM_NV_DEVICE m_Device;
    WORD m_wAddress;
    WORD m_wCount;
    BOOL m_bHasCrc;
    BOOL m_bHasID;
    CHAR m_sID[4];
} AM_BACKUP_RECORD;

typedef enum AM_BACKUP_RECORD_N {
    AM_BACKUP_RECORD_STATIC = 0,
    AM_BACKUP_RECORD_CREDIT = 1,
    AM_BACKUP_RECORD_NETWORK_ETH0 = 2,
    AM_BACKUP_RECORD_NETWORK_ETH1 = 3,
    AM_BACKUP_RECORD_HISTORY = 4,
    AM_BACKUP_RECORD_ALPB_CARD_ID = 5,
    AM_BACKUP_RECORD_ALPB_COMPUTER_NAME = 6,
    AM_BACKUP_RECORD_BACKUP = 7,
    AM_BACKUP_RECORD_HM_PEAK = 8,
    AM_BACKUP_RECORD_TIMEZONE = 9,
    AM_BACKUP_RECORD_ERROR_LOG = 10,
    AM_BACKUP_RECORD_ALPB_DEV_CONFIG = 11,
} AM_BACKUP_RECORD_N;

AM_BACKUP_STATUS amBackupInit(void);
AM_BACKUP_STATUS amBackupExit(void);

AM_BACKUP_STATUS amBackupWrite(AM_PLATFORM_NV_DEVICE_CONFIG *lpConfig, LPVOID lpBuffer,
                               WORD wAddress, WORD wCount);
AM_BACKUP_STATUS amBackupRecordWrite(AM_BACKUP_RECORD *lpConfig, LPVOID lpBuffer);
AM_BACKUP_STATUS amBackupRecordWriteDup(AM_BACKUP_RECORD *lpConfig, LPVOID lpBuffer);
AM_BACKUP_STATUS amBackupRecordValidate(AM_BACKUP_RECORD *lpConfig, LPVOID lpBuffer);

AM_BACKUP_STATUS amBackupRead(AM_PLATFORM_NV_DEVICE_CONFIG *lpConfig, WORD wAddress,
                              LPVOID lpBuffer, WORD wCount);
AM_BACKUP_STATUS amBackupRecordRead(AM_BACKUP_RECORD *lpConfig, LPVOID lpBuffer);
AM_BACKUP_STATUS amBackupRecordReadDup(AM_BACKUP_RECORD *lpConfig, LPVOID lpBuffer);
AM_BACKUP_STATUS amBackupRecordCheckValid(AM_BACKUP_RECORD *lpConfig, LPVOID lpBuffer);

AM_BACKUP_RECORD *amBackupGetRecord(DWORD dwWhich);
