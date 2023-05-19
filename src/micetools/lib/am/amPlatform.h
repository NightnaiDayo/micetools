#pragma once

#include <Windows.h>

#include "../_am.h"

AM_LIB_H_HEADER(amPlatform, AM_PLATFORM)

#define PLATFORM_RINGWIDE "AAM"
#define PLATFORM_RINGEDGE "AAL"
#define PLATFORM_RINGEDGE2 "AAL2"  // Also AAS in places

#define MANUFACTURER_RE "NEC"
#define MANUFACTURER_RW_1 "Supermicro"
#define MANUFACTURER_RW_2 "Advantech"

typedef struct {
    char strPlatformId[4];
} AM_PLATFORM_PLATFORM_ID;

typedef enum {
    AM_PLATFORM_BOARD_TYPE_RINGEDGE = 0,
    AM_PLATFORM_BOARD_TYPE_RW_SPEC_1 = 1,  // Supermicro
    AM_PLATFORM_BOARD_TYPE_RW_SPEC_2 = 2,  // Advantech
    AM_PLATFORM_BOARD_TYPE_RINGEDGE2 = 3,
    AM_PLATFORM_BOARD_TYPE_UNKNOWN = 4,
} AM_PLATFORM_BOARD_TYPE;

typedef struct AM_PLATFORM {
    BOOL m_boardTypeCached;
    AM_PLATFORM_BOARD_TYPE m_boardType;
    BOOL m_platformIdCached;
    AM_PLATFORM_PLATFORM_ID m_platformId;
} AM_PLATFORM;

typedef struct {
    char strVendorId[5];
    char strDeviceId[5];
    char strSubId[9];
} AM_PLATFORM_ID_GROUP;

typedef enum {
    AM_PLATFORM_STATUS_OK = 0,
    AM_PLATFORM_STATUS_NO_INFO = 1,
    AM_PLATFORM_STATUS_BUSY = 2,
    AM_PLATFORM_STATUS_NG = -1,
    AM_PLATFORM_STATUS_ERR_INVALID_PARAM = -2,
    AM_PLATFORM_STATUS_ERR_SYS = -3,
    AM_PLATFORM_STATUS_ERR_NO_OPEN = -4,
    AM_PLATFORM_STATUS_ERR_ALREADY_OPEN = -5,
    AM_PLATFORM_STATUS_ERR_NO_REQUEST = -6,
    AM_PLATFORM_STATUS_ERR_ALREADY_REQUEST = -7,
    AM_PLATFORM_STATUS_ERR_BUFF_SIZE = -8,
    AM_PLATFORM_STATUS_ERR_NO_INIT = -9,
    AM_PLATFORM_STATUS_ERR_ALREADY_INIT = -10,
    AM_PLATFORM_STATUS_NOT_SUPPORT = -11,
} AM_PLATFORM_STATUS;

typedef enum {
    AM_PLATFORM_NV_DEVICE_EEPROM = 0,
    AM_PLATFORM_NV_DEVICE_EEPROM_DUP = 1,
    AM_PLATFORM_NV_DEVICE_SRAM = 2,
    AM_PLATFORM_NV_DEVICE_SRAM_DUP = 3,
    _NUM_AM_PLATFORM_NV_DEVICE,
} AM_PLATFORM_NV_DEVICE;

typedef int(amPlatformRead_t)(WORD addr, LPBYTE buf, DWORD length);
typedef int(amPlatformWrite_t)(WORD addr, LPBYTE buf, DWORD length);

typedef struct {
    DWORD m_base;
    DWORD m_size;
    DWORD m_blockSize;
    amPlatformRead_t* m_read;
    amPlatformWrite_t* m_write;
} AM_PLATFORM_NV_DEVICE_CONFIG;

AM_PLATFORM_NV_DEVICE_CONFIG* amPlatformGetNvDevice(AM_PLATFORM_NV_DEVICE device);

AM_PLATFORM_STATUS amPlatformGetBoardType(AM_PLATFORM_BOARD_TYPE* boardType);
AM_PLATFORM_STATUS amPlatformGetPlatformId(AM_PLATFORM_PLATFORM_ID* platformId);

// TODO:

void amPlatformAsyncClose(void);
void amPlatformAsyncReqUsbDeviceCount(void);
void amPlatformAsyncResUsbDeviceCount(void);
void amPlatformChangeDisplayModeAmd(void);
void amPlatformChangeDisplayModeNvidia(void);
void amPlatformChangeDisplayModeNvidiaEx(void);
void amPlatformChangeDisplayResolutionAmd(void);
void amPlatformChangeDisplayResolutionNvidia(void);
void amPlatformChangeDisplayResolutionNvidiaEx(void);
void amPlatformChangeDisplayResolutionWinAPI(void);
void amPlatformDestroyCheckInterfaceHandle(void);
void amPlatformGetAvailableAmdDriver(void);
void amPlatformGetAvailableNvidiaDriver(void);
void amPlatformGetBiosInfo(void);
void amPlatformGetCheckInterfaceHandle(void);
void amPlatformGetComInfo(void);
void amPlatformGetDevInfo(void);
void amPlatformGetGpuPstate(void);
void amPlatformGetGraphicInfo(void);
void amPlatformGetMemorySize(void);
void amPlatformGetOsVersion(void);
void amPlatformGetPartitionInfo(void);
void amPlatformGetPlatformIdEx(void);
void amPlatformGetPortName(void);
void amPlatformGetSearchDevInfo(void);
void amPlatformGetSoundInfo(void);
void amPlatformGetStorageInfo(void);
void amPlatformNvapiInit(void);
void amPlatformSetDisplayMode(void);
void amPlatformSetDisplayResolution(void);
void amPlatformSetDisplayResolutionAndMode(void);
