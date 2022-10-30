#include "amEeprom.h"
#include "../mice/crc.h"

HANDLE amEepromCreateDeviceFile(const GUID *guid, LPCSTR resource, DWORD member_index) {
    SP_DEVICE_INTERFACE_DATA interface_data;
    SP_DEVICE_INTERFACE_DETAIL_DATA_A interface_detail[204];

    if (!guid) return INVALID_HANDLE_VALUE;

    HDEVINFO DeviceInfoSet =
        SetupDiGetClassDevsA(guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (DeviceInfoSet == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;

    interface_data.cbSize = 0x1c;
    BOOL s;
    s = SetupDiEnumDeviceInterfaces(DeviceInfoSet, NULL, guid, member_index, &interface_data);
    if (!s) goto fail;

    interface_detail[0].cbSize = 5;
    s = SetupDiGetDeviceInterfaceDetailA(DeviceInfoSet, &interface_data, interface_detail,
                                         sizeof interface_detail, NULL, NULL);
    if (!s) goto fail;

    char device_path[260];
    strcpy_s(device_path, sizeof device_path, interface_detail[0].DevicePath);

    if (resource != NULL) {
        strcat_s(device_path, 4, "\\");
        strcat_s(device_path, 4, resource);
    }

    printf("Using device located at %s\n", device_path);
    HANDLE device =
        CreateFileA(device_path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, FILE_SUPPORTS_GHOSTING, NULL);
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    return device;

fail:
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    return INVALID_HANDLE_VALUE;
}

BOOL amEepromReadBlock(HANDLE mxsmbus, BYTE reg, BYTE len, BYTE *data) {
    mxsmbus_request_packet smbus_req = {
        .status = 0,
        .prt = 0x09,
        .addr = SMBUS_EEPROM,
        .reg = reg << 5,
        .dlen = len,
        .data = { 0 },
    };

    DWORD _dummy;
    BOOL s = DeviceIoControl(mxsmbus, (DWORD)IOCTL_MXSMBUS_I2C, &smbus_req, sizeof smbus_req,
                             &smbus_req, sizeof smbus_req, &_dummy, NULL);
    if (!s) return FALSE;
    if (smbus_req.status) return FALSE;
    memcpy(data, smbus_req.data, len);
    return TRUE;
}

BOOL amEepromWriteBlock(HANDLE mxsmbus, BYTE reg, BYTE len, BYTE *data) {
    mxsmbus_request_packet smbus_req = {
        .status = 0,
        .prt = 0x08,
        .addr = SMBUS_EEPROM,
        .reg = reg << 5,
        .dlen = len,
        .data = { 0 },
    };
    memcpy(smbus_req.data, data, sizeof smbus_req.data);

    DWORD _dummy;
    BOOL s = DeviceIoControl(mxsmbus, (DWORD)IOCTL_MXSMBUS_I2C, &smbus_req, sizeof smbus_req,
                             &smbus_req, sizeof smbus_req, &_dummy, NULL);
    if (!s) return FALSE;
    if (smbus_req.status) return FALSE;
    memcpy(data, smbus_req.data, len);
    return TRUE;
}

void amEepromRepairChecksum(BYTE *data) {
    crc32_build_table();

    DWORD check = crc32(28, data + 4, 0);
    ((DWORD*)data)[0] = check;
}
