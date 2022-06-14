#include "micesetupapi.h"

FAKE_DEVICE* fake_devices = NULL;

BOOL add_fake_device(const GUID* guid, const WCHAR* path) {
    FAKE_DEVICE* new_device = (FAKE_DEVICE*)malloc(sizeof *new_device);
    if (!new_device) return FALSE;
    new_device->guid = guid;
    new_device->path = path;
    new_device->handle = NULL;
    new_device->next = NULL;
    FAKE_DEVICE** tail = &fake_devices;
    while (*tail) tail = &((*tail)->next);
    *tail = new_device;
    return TRUE;
}

HDEVINFO WINAPI FakeSetupDiGetClassDevsA(const GUID* ClassGuid, PCWSTR Enumerator, HWND hwndParent, DWORD Flags) {
    log_misc("setupapi", "SetupDiGetClassDevsA(%p, %s, %d, %d)", ClassGuid, Enumerator, hwndParent, Flags);
    HDEVINFO res = TrueSetupDiGetClassDevsA(ClassGuid, Enumerator, hwndParent, Flags);

    if (res != INVALID_HANDLE_VALUE && ClassGuid != NULL) {
        FAKE_DEVICE* device = fake_devices;
        while (device != NULL) {
            if (memcmp(device->guid, ClassGuid, sizeof(*ClassGuid)) == 0) {
                log_misc("setupapi", "injecting %ls into class devs list", device->path);
                device->handle = res;
            }
            device = device->next;
        }
    } else {
        log_warning("setupapi", "upstream SetupDiGetClassDevsA failed: %d", GetLastError());
    }

    return res;
}

BOOL WINAPI FakeSetupDiEnumDeviceInterfaces(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData,
                                            const GUID* InterfaceClassGuid, DWORD MemberIndex,
                                            PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData) {
    log_misc("setupapi", "SetupDiEnumDeviceInterfaces");
    FAKE_DEVICE* device = fake_devices;
    if (DeviceInterfaceData) {
        while (device != NULL) {
            if (device->handle == DeviceInfoSet) {
                log_info("setupapi", "hooking fake device: %ls", device->path);
                memcpy(&DeviceInterfaceData->InterfaceClassGuid, device->guid, sizeof *device->guid);
                DeviceInterfaceData->Flags = SPINT_ACTIVE;
                DeviceInterfaceData->Reserved = (ULONG_PTR)device->path;
                SetLastError(ERROR_SUCCESS);
                return TRUE;
            }
            device = device->next;
        }
    }

    // Fallback
    log_misc("setupapi", "device info fallthrough");
    return TrueSetupDiEnumDeviceInterfaces(DeviceInfoSet, DeviceInfoData, InterfaceClassGuid, MemberIndex,
                                           DeviceInterfaceData);
}

BOOL WINAPI FakeSetupDiGetDeviceInterfaceDetailA(HDEVINFO DeviceInfoSet, PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
                                                 PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData,
                                                 DWORD DeviceInterfaceDetailDataSize, PDWORD RequiredSize,
                                                 PSP_DEVINFO_DATA DeviceInfoData) {
    log_misc("setupapi", "SetupDiGetDeviceInterfaceDetailA");
    FAKE_DEVICE* device = fake_devices;
    while (device != NULL) {
        if (device->handle == DeviceInfoSet && (ULONG_PTR)device->path == DeviceInterfaceData->Reserved) {
            log_info("setupapi", "Intercepted SetupDiGetDeviceInterfaceDetailA");

            const WCHAR* res = (WCHAR*)DeviceInterfaceData->Reserved;
            int new_len = (wcslen(res) + 1) * (sizeof *res);
            char* new_res = (char*)malloc(new_len);
            size_t convertedChars = 0;
            wcstombs_s(&convertedChars, new_res, new_len, res, _TRUNCATE);

            size_t len = offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath) + convertedChars;
            if (RequiredSize) *RequiredSize = len;
            if (!DeviceInterfaceDetailData && DeviceInterfaceDetailDataSize < len) {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }
            if (!DeviceInterfaceDetailData || sizeof *DeviceInterfaceDetailData != DeviceInterfaceDetailData->cbSize) {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            memcpy(DeviceInterfaceDetailData->DevicePath, new_res, len);
            free(new_res);
            SetLastError(ERROR_SUCCESS);
            return TRUE;
        }
        device = device->next;
    }

    log_misc("setupapi", "TrueSetupDiGetDeviceInterfaceDetailA fallthrough");
    return TrueSetupDiGetDeviceInterfaceDetailA(DeviceInfoSet, DeviceInterfaceData, DeviceInterfaceDetailData,
                                                DeviceInterfaceDetailDataSize, RequiredSize, DeviceInfoData);
}

BOOL WINAPI FakeSetupDiDestroyDeviceInfoList(HDEVINFO DevicesInfoSet) {
    FAKE_DEVICE* device = fake_devices;
    while (device != NULL) {
        if (device->handle == DevicesInfoSet) device->handle = NULL;
        device = device->next;
    }
    return TrueSetupDiDestroyDeviceInfoList(DevicesInfoSet);
}

void hook_setupapi() {
    hook("Setupapi.DLL", "SetupDiGetClassDevsA", FakeSetupDiGetClassDevsA, (void**)&TrueSetupDiGetClassDevsA, 6);
    hook("Setupapi.DLL", "SetupDiEnumDeviceInterfaces", FakeSetupDiEnumDeviceInterfaces,
         (void**)&TrueSetupDiEnumDeviceInterfaces, 7);
    hook("Setupapi.DLL", "SetupDiGetDeviceInterfaceDetailA", FakeSetupDiGetDeviceInterfaceDetailA,
         (void**)&TrueSetupDiGetDeviceInterfaceDetailA, 6);
    hook("Setupapi.DLL", "SetupDiDestroyDeviceInfoList", FakeSetupDiDestroyDeviceInfoList,
         (void**)&TrueSetupDiDestroyDeviceInfoList, 7);
}
