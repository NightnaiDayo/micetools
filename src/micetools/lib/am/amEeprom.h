#pragma once

#include <Windows.h>

#include "../../dll/smbus.h"
#include "../_am.h"

AM_LIB_H_HEADER(amEeprom, AM_EEPROM)

typedef enum {
    AM_EEPROM_STATUS_OK = 0,
    AM_EEPROM_STATUS_NG = -1,
    AM_EEPROM_STATUS_ERR_INVALID_PARAM = -2,
    AM_EEPROM_STATUS_ERR_NO_INIT = -3,
    AM_EEPROM_STATUS_ERR_ALREADY_INIT = -4,
    AM_EEPROM_STATUS_ERR_SYS = -5,
    AM_EEPROM_STATUS_ERR_READ = -6,
    AM_EEPROM_STATUS_ERR_WRITE = -7,
    AM_EEPROM_STATUS_ERR_TIMEOUT = -8,
    AM_EEPROM_STATUS_ERR_GET_MUTEX = -9,
    AM_EEPROM_STATUS_ERR_PROTOCOL_VER = -10,
} AM_EEPROM_STATUS;

typedef struct {
    int ReadTimeout;
    int WriteTimeout;
} AM_EEPROM_TIMEOUT;

typedef struct _AM_EEPROM {
    BOOL m_init;
    AM_EEPROM_TIMEOUT m_timeout;
    HANDLE m_mutex;
    HANDLE m_superio;
    BYTE m_prt;
    PSID m_sid;
} AM_EEPROM;
#define AM_EEPROM_DEFAULT_TIMEOUT 6

#define AM_EEPROM_ADDR 0x57

DEFINE_GUID(MXSMBUS_GUID, 0x5C49E1FE, 0x3FEC, 0x4B8D, 0xA4, 0xB5, 0x76, 0xBE, 0x70, 0x25, 0xD8,
            0x42);
DEFINE_GUID(PLATFORM_GUID, 0x86E0D1E0, 0x8089, 0x11D0, 0x9C, 0xE4, 0x08, 0x00, 0x3e, 0x30, 0x1F,
            0x73);

HANDLE amEepromCreateDeviceFile(const GUID *guid, LPCSTR resource, DWORD member_index);
AM_EEPROM_STATUS amEepromGetDriverVersion(AM_EEPROM *device, LPDWORD version);
AM_EEPROM_STATUS amEepromCreateMutex(AM_EEPROM *device);

BOOL amEepromI2CReadBlock(AM_EEPROM *device, WORD reg, BYTE nBytes, LPBYTE buffer);
BOOL amEepromI2CWriteBlock(AM_EEPROM *device, WORD reg, BYTE nBytes, LPBYTE buffer);

AM_EEPROM_STATUS amEepromInit(AM_EEPROM_TIMEOUT *timeout);
AM_EEPROM_STATUS amEepromExit(void);
AM_EEPROM_STATUS amEepromRead(WORD reg, LPBYTE buf, DWORD length);
AM_EEPROM_STATUS amEepromWrite(WORD reg, LPBYTE buf, DWORD length);

// TODO:
void amEepromSetTimeout(void);
