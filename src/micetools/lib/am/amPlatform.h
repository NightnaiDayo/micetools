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
typedef struct {
    char strPlatformId[4];
    int platformId;
} AM_PLATFORM_PLATFORM_ID_EX;

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

typedef struct AM_PLATFORM_ID_GROUP {
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

typedef struct AM_PLATFORM_NV_DEVICE_CONFIG {
    DWORD m_Base;
    DWORD m_Size;
    DWORD m_BlockSize;
    amPlatformRead_t* m_ReadFunc;
    amPlatformWrite_t* m_WriteFunc;
} AM_PLATFORM_NV_DEVICE_CONFIG;

typedef struct AM_PLATFORM_BIOS_INFO {
    CHAR m_BiosVer[64];
    CHAR m_BiosReleaseDate[11];
} AM_PLATFORM_BIOS_INFO;

typedef struct AM_PLATFORM_CMOS_PARAM {
    // [address, bit offset][2]
    BYTE index[2][2];
    BYTE m_MbrCount;
} AM_PLATFORM_CMOS_PARAM;

AM_PLATFORM_NV_DEVICE_CONFIG* amPlatformGetNvDevice(AM_PLATFORM_NV_DEVICE device);

AM_PLATFORM_STATUS amPlatformGetBoardType(AM_PLATFORM_BOARD_TYPE* boardType);
AM_PLATFORM_STATUS amPlatformGetPlatformId(AM_PLATFORM_PLATFORM_ID* platformId);
AM_PLATFORM_STATUS amPlatformGetPlatformIdEx(AM_PLATFORM_PLATFORM_ID_EX* platformId);

int amPlatformGetBiosInfo(AM_PLATFORM_BIOS_INFO* lpInfo);
int amPlatformGetOsVersion(DWORD* lpVersion);  // Reads SystemVersion.txt
int amPlatformGetMemorySize(PLARGE_INTEGER size);

AM_PLATFORM_CMOS_PARAM* amPlatformGetCmosParam(AM_PLATFORM_BOARD_TYPE boardType);

// TODO:
// usbDongle.exe -> see in maimai 1.90 folder
void amPlatformAsyncClose(void);              // usbDongle.exe
void amPlatformAsyncReqUsbDeviceCount(void);  // usbDongle.exe
void amPlatformAsyncResUsbDeviceCount(void);  // usbDongle.exe
void amPlatformChangeDisplayModeAmd(void);
void amPlatformChangeDisplayModeNvidia(void);
void amPlatformChangeDisplayModeNvidiaEx(void);
void amPlatformChangeDisplayResolutionAmd(void);
void amPlatformChangeDisplayResolutionNvidia(void);
void amPlatformChangeDisplayResolutionNvidiaEx(void);
void amPlatformChangeDisplayResolutionWinAPI(void);
void amPlatformDestroyCheckInterfaceHandle(void);
void amPlatformGetAvailableAmdDriver(void);     // usbDongle.exe
void amPlatformGetAvailableNvidiaDriver(void);  // usbDongle.exe
void amPlatformGetCheckInterfaceHandle(void);   // usbDongle.exe
void amPlatformGetComInfo(void);                // usbDongle.exe
void amPlatformGetDevInfo(void);                // usbDongle.exe
void amPlatformGetGpuPstate(void);              // usbDongle.exe
void amPlatformGetGraphicInfo(void);            // usbDongle.exe
void amPlatformGetPartitionInfo(void);          // usbDongle.exe
void amPlatformGetPortName(void);               // usbDongle.exe
void amPlatformGetSearchDevInfo(void);          // usbDongle.exe
void amPlatformGetSoundInfo(void);              // usbDongle.exe
void amPlatformGetStorageInfo(void);            // usbDongle.exe
void amPlatformNvapiInit(void);                 // usbDongle.exe
void amPlatformSetDisplayMode(void);
void amPlatformSetDisplayResolution(void);
void amPlatformSetDisplayResolutionAndMode(void);
