#include "amBackup.h"

#include "../ami/amiCrc.h"
#include "../ami/amiDebug.h"

AM_LIB_C_HEADER(amBackup, AM_BACKUP)

AM_BACKUP_STATUS amBackupInit(void) {
    if (amBackup.m_Init) return AM_BACKUP_STATUS_ALREADY_INIT;

    ZeroMemory(&amBackup, sizeof amBackup);
    amiCrc32RCreateTable(amBackup.m_CrcTable);

    amBackup.m_Init = TRUE;
    return AM_BACKUP_STATUS_OK;
}
AM_BACKUP_STATUS amBackupExit(void) {
    if (!amBackup.m_Init) return AM_BACKUP_STATUS_NO_INIT;
    ZeroMemory(&amBackup, sizeof amBackup);
    return AM_BACKUP_STATUS_OK;
}

static BOOL amBackupValidateAddress(AM_PLATFORM_NV_DEVICE_CONFIG *lpConfig, DWORD wAddress,
                                    WORD wCount) {
    if (lpConfig == NULL) return FALSE;
    if (wAddress < lpConfig->m_Base) return FALSE;
    if (wAddress + wCount > lpConfig->m_Base + lpConfig->m_Size) return FALSE;

    DWORD mask = lpConfig->m_BlockSize - 1;
    if ((wAddress & mask) != 0) return FALSE;
    if ((wCount & mask) != 0) return FALSE;

    if (wCount > lpConfig->m_Size) return FALSE;

    return TRUE;
}

AM_BACKUP_STATUS amBackupWrite(AM_PLATFORM_NV_DEVICE_CONFIG *lpConfig, LPVOID lpBuffer,
                               WORD wAddress, WORD wCount) {
    if (amBackup.m_Init == 0) return AM_BACKUP_STATUS_NO_INIT;
    if (lpConfig == NULL || lpBuffer == NULL) return AM_BACKUP_STATUS_INVALID_PARAM;

    if (!amBackupValidateAddress(lpConfig, wAddress, wCount)) {
        if (amBackupDebugLevel > 0) amiDebugLog("invalid address.");
        return AM_BACKUP_STATUS_INVALID_ADDR;
    }
    if (lpConfig->m_WriteFunc == NULL) {
        if (amBackupDebugLevel > 0) amiDebugLog("No Write Function Error!.");
        return AM_BACKUP_STATUS_INVALID_STORAGE;
    }
    return (lpConfig->m_WriteFunc)(wAddress, lpBuffer, wCount);
}

AM_BACKUP_STATUS amBackupRecordCheckValid(AM_BACKUP_RECORD *lpConfig, LPVOID lpBuffer) {
    if (!amBackup.m_Init) return AM_BACKUP_STATUS_NO_INIT;
    if (lpConfig == NULL || lpBuffer == NULL) return AM_BACKUP_STATUS_INVALID_PARAM;

    LPDWORD bufferAsDW = (LPDWORD)lpBuffer;

    if (lpConfig->m_bHasCrc) {
        DWORD calcCrc =
            amiCrc32RGet(amBackup.m_CrcTable, lpConfig->m_wCount - 4, bufferAsDW + 1, 0);
        if (calcCrc != bufferAsDW[0]) {
            if (amBackupDebugLevel > 0)
                amiDebugLog("crc error. NewCrc=%08X OrgCrc=%08X", calcCrc, bufferAsDW[0]);
            return AM_BACKUP_STATUS_BAD_CRC;
        }
        bufferAsDW++;
    }
    if (lpConfig->m_bHasID) {
        if (*(LPDWORD)(lpConfig->m_sID) != bufferAsDW[0]) {
            if (amBackupDebugLevel > 0)
                amiDebugLog("id error. NewId=%08x OrgId=%08x", *(LPDWORD)lpConfig->m_sID,
                            bufferAsDW[0]);
            return AM_BACKUP_STATUS_BAD_ID;
        }
    }
    return AM_BACKUP_STATUS_OK;
}

AM_BACKUP_STATUS amBackupRead(AM_PLATFORM_NV_DEVICE_CONFIG *lpConfig, WORD wAddress,
                              LPVOID lpBuffer, WORD wCount) {
    if (!amBackup.m_Init) return AM_BACKUP_STATUS_NO_INIT;
    if (lpConfig == NULL || lpBuffer == NULL) return AM_BACKUP_STATUS_INVALID_PARAM;

    if (!amBackupValidateAddress(lpConfig, wAddress, wCount)) {
        if (amBackupDebugLevel > 0) amiDebugLog("invalid address.");
        return AM_BACKUP_STATUS_INVALID_ADDR;
    }
    if (lpConfig->m_ReadFunc == NULL) {
        if (amBackupDebugLevel > 0) amiDebugLog("No Read Function Error!.");
        return AM_BACKUP_STATUS_INVALID_STORAGE;
    }
    return (lpConfig->m_ReadFunc)(wAddress, lpBuffer, wCount);
}

AM_BACKUP_STATUS amBackupRecordRead(AM_BACKUP_RECORD *lpConfig, LPVOID lpBuffer) {
    if (!amBackup.m_Init) return AM_BACKUP_STATUS_NO_INIT;
    if (lpConfig == NULL || lpBuffer == NULL) return AM_BACKUP_STATUS_INVALID_PARAM;

    AM_PLATFORM_NV_DEVICE_CONFIG *device = amPlatformGetNvDevice(lpConfig->m_Device);
    if (device == NULL) return AM_BACKUP_STATUS_NO_DEVICE;

    int err = amBackupRead(device, lpConfig->m_wAddress, lpBuffer, lpConfig->m_wCount);
    if (err < 0) return err;

    err = amBackupRecordCheckValid(lpConfig, lpBuffer);
    return err >= 0 ? AM_BACKUP_STATUS_OK : err;
}

AM_BACKUP_STATUS amBackupRecordReadDup(AM_BACKUP_RECORD *lpConfig, LPVOID lpBuffer) {
    if (!amBackup.m_Init) return AM_BACKUP_STATUS_NO_INIT;
    if (lpConfig == NULL || lpBuffer == NULL) return AM_BACKUP_STATUS_INVALID_PARAM;

    int err = amBackupRecordRead(lpConfig, lpBuffer);
    if (err > -1) return AM_BACKUP_STATUS_OK;

    if (err < -19) {
        err = amBackupRecordRead(lpConfig + 1, lpBuffer);
        if (-1 < err) return amBackupRecordWrite(lpConfig, lpBuffer);
    }
    return err;
}

AM_BACKUP_STATUS amBackupRecordValidate(AM_BACKUP_RECORD *lpConfig, LPVOID lpBuffer) {
    if (!amBackup.m_Init) return AM_BACKUP_STATUS_NO_INIT;
    if (lpConfig == NULL || lpBuffer == NULL) return AM_BACKUP_STATUS_INVALID_PARAM;

    LPBYTE lpData = (LPBYTE)lpBuffer;
    WORD wCount = lpConfig->m_wCount;
    if (lpConfig->m_bHasCrc) {
        wCount -= 4;
        lpData += 4;
    }

    if (lpConfig->m_bHasID) {
        *((LPDWORD)lpData) = *(LPDWORD)lpConfig->m_sID;
        lpData += 4;
        wCount -= 4;
    }

    if (lpConfig->m_bHasCrc) {
        *((LPDWORD)lpBuffer) = amiCrc32RGet(amBackup.m_CrcTable, wCount, lpData, 0);
    }

    return AM_BACKUP_STATUS_OK;
}

AM_BACKUP_STATUS amBackupRecordWrite(AM_BACKUP_RECORD *lpConfig, LPVOID lpBuffer) {
    if (!amBackup.m_Init) return AM_BACKUP_STATUS_NO_INIT;
    if (lpConfig == NULL || lpBuffer == NULL) return AM_BACKUP_STATUS_INVALID_PARAM;

    AM_PLATFORM_NV_DEVICE_CONFIG *device = amPlatformGetNvDevice(lpConfig->m_Device);
    if (device == NULL) return AM_BACKUP_STATUS_NO_DEVICE;

    int err = amBackupRecordValidate(lpConfig, lpBuffer);
    if (err < 0) return err;

    err = amBackupWrite(device, lpBuffer, lpConfig->m_wAddress, lpConfig->m_wCount);
    return err >= 0 ? AM_BACKUP_STATUS_OK : err;
}

AM_BACKUP_STATUS amBackupRecordWriteDup(AM_BACKUP_RECORD *lpConfig, LPVOID lpBuffer) {
    if (!amBackup.m_Init) return AM_BACKUP_STATUS_NO_INIT;
    if (lpConfig == NULL || lpBuffer == NULL) return AM_BACKUP_STATUS_INVALID_PARAM;

    int err = amBackupRecordWrite(&(lpConfig[1]), lpBuffer);
    if (err + 19 <= 18) return err;

    return amBackupRecordWrite(&(lpConfig[0]), lpBuffer);
}

static AM_BACKUP_RECORD _amBackupStructs[12][2] = {
    // AM_BACKUP_RECORD_STATIC
    {
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_EEPROM,
            .m_wAddress = 0x000,
            .m_wCount = 0x20,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_EEPROM,
            .m_wAddress = 0x200,
            .m_wCount = 0x20,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
    },
    // AM_BACKUP_RECORD_CREDIT
    {
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_EEPROM,
            .m_wAddress = 0x020,
            .m_wCount = 0x20,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_EEPROM,
            .m_wAddress = 0x220,
            .m_wCount = 0x20,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
    },
    // AM_BACKUP_RECORD_NETWORK_ETH0
    {
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_EEPROM,
            .m_wAddress = 0x040,
            .m_wCount = 0x20,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_EEPROM,
            .m_wAddress = 0x240,
            .m_wCount = 0x20,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
    },
    // AM_BACKUP_RECORD_NETWORK_ETH1
    {
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_EEPROM,
            .m_wAddress = 0x060,
            .m_wCount = 0x20,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_EEPROM,
            .m_wAddress = 0x260,
            .m_wCount = 0x20,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
    },
    // AM_BACKUP_RECORD_HISTORY
    {
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_EEPROM,
            .m_wAddress = 0x080,
            .m_wCount = 0x10,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_EEPROM,
            .m_wAddress = 0x280,
            .m_wCount = 0x10,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
    },
    // AM_BACKUP_RECORD_ALPB_CARD_ID
    {
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_EEPROM,
            .m_wAddress = 0x090,
            .m_wCount = 0x10,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_EEPROM,
            .m_wAddress = 0x290,
            .m_wCount = 0x10,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
    },
    // AM_BACKUP_RECORD_ALPB_COMPUTER_NAME
    {
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_EEPROM,
            .m_wAddress = 0x0a0,
            .m_wCount = 0x20,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_EEPROM,
            .m_wAddress = 0x2a0,
            .m_wCount = 0x20,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
    },
    // AM_BACKUP_RECORD_BACKUP
    {
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_SRAM,
            .m_wAddress = 0x0000,
            .m_wCount = 0x200,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_SRAM,
            .m_wAddress = 0x1000,
            .m_wCount = 0x200,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
    },
    // AM_BACKUP_RECORD_HM_PEAK
    {
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_SRAM,
            .m_wAddress = 0x0200,
            .m_wCount = 0x200,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_SRAM,
            .m_wAddress = 0x1200,
            .m_wCount = 0x200,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
    },
    // AM_BACKUP_RECORD_TIMEZONE
    {
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_SRAM,
            .m_wAddress = 0x0400,
            .m_wCount = 0x200,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_SRAM,
            .m_wAddress = 0x1400,
            .m_wCount = 0x200,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
    },
    // AM_BACKUP_RECORD_ERROR_LOG
    {
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_SRAM,
            .m_wAddress = 0x0600,
            .m_wCount = 0x200,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_SRAM,
            .m_wAddress = 0x1600,
            .m_wCount = 0x200,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
    },
    // AM_BACKUP_RECORD_ALPB_DEV_CONFIG
    {
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_EEPROM,
            .m_wAddress = 0x0c0,
            .m_wCount = 0x40,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
        {
            .m_Device = AM_PLATFORM_NV_DEVICE_EEPROM,
            .m_wAddress = 0x2c0,
            .m_wCount = 0x40,
            .m_bHasCrc = TRUE,
            .m_bHasID = FALSE,
            .m_sID = 0,
        },
    },
};

AM_BACKUP_RECORD *amBackupGetRecord(DWORD dwWhich) {
    if (dwWhich < 12) return (AM_BACKUP_RECORD *)&(_amBackupStructs[dwWhich]);
    return NULL;
}
