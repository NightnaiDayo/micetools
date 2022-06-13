#include <Windows.h>
#include <setupapi.h>
#include <stddef.h>

#include "../lib/mice/mice.h"

static HDEVINFO(WINAPI* TrueSetupDiGetClassDevsA)(const GUID* ClassGuid, PCWSTR Enumerator, HWND hwndParent,
                                                  DWORD Flags);
static BOOL(WINAPI* TrueSetupDiEnumDeviceInterfaces)(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData,
                                                     const GUID* InterfaceClassGuid, DWORD MemberIndex,
                                                     PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData);
static BOOL(WINAPI* TrueSetupDiGetDeviceInterfaceDetailA)(HDEVINFO DeviceInfoSet,
                                                          PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
                                                          PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData,
                                                          DWORD DeviceInterfaceDetailDataSize, PDWORD RequiredSize,
                                                          PSP_DEVINFO_DATA DeviceInfoData);
static BOOL(WINAPI* TrueSetupDiDestroyDeviceInfoList)(HDEVINFO DevicesInfoSet);

typedef struct FAKE_DEVICE_ {
    HDEVINFO handle;
    const GUID* guid;
    const WCHAR* path;
    struct FAKE_DEVICE_* next;
} FAKE_DEVICE;
extern FAKE_DEVICE* fake_devices;

BOOL add_fake_device(const GUID* guid, const WCHAR* path);

void hook_setupapi();
