#include <Windows.h>
//
#include <SetupAPI.h>

#include "../am/amEeprom.h"
#include "../ami/ami.h"
#include "../mice/ioctl.h"
#include "mxkSmbus.h"

#pragma pack(push, 1)
typedef struct {
    unsigned char status;
    unsigned char command;
    unsigned short addr;
    unsigned short command_code;
    unsigned char nbytes;
    unsigned char data[32];
} i2c_packet;
typedef struct {
    unsigned char status;
    unsigned char command;
    unsigned char addr;
    unsigned char command_code;
    unsigned char nbytes;
    unsigned char data[32];
} smb_packet;
#pragma pack(pop)

HANDLE N2_MUTEX;
HANDLE MXSMBUS = INVALID_HANDLE_VALUE;

int mxkSmbusInit(void) {
    static BOOL hasInit;

    if (hasInit) return -4;

    N2_MUTEX = CreateMutexA(NULL, 0, NULL);
    if (N2_MUTEX == NULL) {
        amiDebugLog("CreateMutex Error(%ld).", GetLastError());
        return -5;
    }

    int error;

    MXSMBUS = mxkSmbusCreateDeviceFile();
    if (MXSMBUS == INVALID_HANDLE_VALUE) {
        amiDebugLog("mxkSmbusCreateDeviceFile Error.");
        error = -5;
    } else {
        DWORD nBytesReturned;
        unsigned int version;
        BOOL succ = DeviceIoControl(MXSMBUS, IOCTL_MXSRAM_GET_VERSION, NULL, 0, &version,
                                    sizeof version, &nBytesReturned, NULL);
        if (!succ || nBytesReturned != sizeof version) {
            error = -5;
            amiDebugLog("mxkSmbusGetDriverVerision Error.");
        } else {
            if ((version & 0xffff) == 1) {
                hasInit = 1;
                return 0;
            }
            amiDebugLog(
                "Unknown SMBUS Driver Protocol(0x%08x). Please Update SMBUS "
                "Driver or User Program.",
                version);
            error = -6;
        }
    }

    if (MXSMBUS != INVALID_HANDLE_VALUE) {
        CloseHandle(MXSMBUS);
        MXSMBUS = INVALID_HANDLE_VALUE;
    }

    if (N2_MUTEX == NULL) return error;
    CloseHandle(N2_MUTEX);
    N2_MUTEX = NULL;
    return error;
}

int mxkSmbusI2CWriteBlock(unsigned char addr, unsigned char command, unsigned char *buffer,
                     unsigned char nbytes) {
    i2c_packet packet;
    packet.addr = addr;
    packet.command_code = (command << 8) | buffer[0];
    packet.nbytes = (nbytes - 1) & 0xff;
    packet.status = 0;
    packet.command = 8;
    memcpy(packet.data, buffer + 1, packet.nbytes);

    for (int tries = 0; tries < 5; tries++) {
        DWORD bytesReturned;
        BOOL success = DeviceIoControl(MXSMBUS, IOCTL_MXSMBUS_I2C, &packet, sizeof packet, &packet,
                                       sizeof packet, &bytesReturned, NULL);
        if (!success || bytesReturned != sizeof packet) {
            return -9;
        }
        if (packet.status == 0) return 0;
        if (packet.status != 24) return -9;

        Sleep(16);
    }
    return -9;
}

int mxkSmbusRequestMutex(void) {
    if (WaitForSingleObject(N2_MUTEX, 256) != WAIT_OBJECT_0) return -7;
    return 0;
}
int mxkSmbusReleaseMutex(void) {
    if (!ReleaseMutex(N2_MUTEX)) {
        amiDebugLog("ReleaseMutex Error(%d).", GetLastError());
        return -5;
    }
    return 0;
}
HANDLE mxkSmbusCreateDeviceFile(void) {
    SP_DEVICE_INTERFACE_DATA interfaceData;
    SP_DEVICE_INTERFACE_DETAIL_DATA_A interfaceDetail[204];

    HDEVINFO DeviceInfoSet =
        SetupDiGetClassDevsA(&MXSMBUS_GUID, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (DeviceInfoSet == INVALID_HANDLE_VALUE) {
        amiDebugLog("SetupDiGetClassDevs Error(%ld).", GetLastError());
        return INVALID_HANDLE_VALUE;
    }

    interfaceData.cbSize = 28;
    BOOL s;
    s = SetupDiEnumDeviceInterfaces(DeviceInfoSet, NULL, &MXSMBUS_GUID, 0, &interfaceData);
    if (!s) {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        amiDebugLog("SetupDiEnumDeviceInterfaces Error(%ld).", GetLastError());
        return INVALID_HANDLE_VALUE;
    }

    interfaceDetail[0].cbSize = 5;
    s = SetupDiGetDeviceInterfaceDetailA(DeviceInfoSet, &interfaceData, interfaceDetail,
                                         sizeof interfaceDetail, NULL, NULL);
    if (!s) {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        amiDebugLog("SetupDiGetDeviceInterfaceDetailA Error(%ld).", GetLastError());
        return INVALID_HANDLE_VALUE;
    }

    char fileName[260];
    strcpy_s(fileName, sizeof fileName, interfaceDetail[0].DevicePath);
    HANDLE device =
        CreateFileA(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, FILE_SUPPORTS_GHOSTING, NULL);
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    return device;
}
int mxkSmbusReadByte(unsigned char addr, unsigned char *readByte, unsigned char cmd_code) {
    if (readByte == NULL) return -2;
    if (MXSMBUS == INVALID_HANDLE_VALUE) return -5;
    smb_packet buffer;

    buffer.command_code = cmd_code;
    buffer.status = 0;
    buffer.command = 5;
    buffer.nbytes = 0;
    buffer.data[0] = 0;
    buffer.data[1] = 0;
    buffer.addr = addr;

    DWORD bytesReturned;
    BOOL success = DeviceIoControl(MXSMBUS, IOCTL_MXSMBUS_REQUEST, &buffer, sizeof buffer, &buffer,
                                   sizeof buffer, &bytesReturned, NULL);
    if (success && buffer.status == 0 && bytesReturned == sizeof buffer) {
        *readByte = buffer.data[0];
        return 0;
    }
    return -8;
}
int mxkSmbusWriteByte(unsigned char v_addr, unsigned char command_code, unsigned char data) {
    if (MXSMBUS == INVALID_HANDLE_VALUE) return -5;

    smb_packet packet;

    packet.command_code = command_code;
    packet.data[0] = data;
    packet.status = 0;
    packet.command = 4;
    packet.nbytes = 0;
    packet.data[1] = 0xff;
    packet.addr = v_addr;

    DWORD bytesReturned;
    BOOL success = DeviceIoControl(MXSMBUS, IOCTL_MXSMBUS_REQUEST, &packet, sizeof packet, &packet,
                                   sizeof packet, &bytesReturned, NULL);

    if (!success || bytesReturned != sizeof packet) {
        amiDebugLog("DIO failed %03x\n", GetLastError());
        return -9;
    }
    if (packet.status != 0) {
        amiDebugLog("Packet status: %d\n", packet.status);
        return -9;
    }
    return 0;
}
int mxkSmbusI2CWriteCommand(unsigned char command_code, unsigned char *buffer, size_t nbytes) {
    if (buffer == NULL || nbytes - 1 > 31) return -2;

    i2c_packet packet;
    packet.addr = KC_DS_ADDRESS;
    packet.nbytes = nbytes & 0xff;
    packet.command_code = command_code | 0xa900;
    packet.status = 0;
    packet.command = 8;
    memcpy(packet.data, buffer, nbytes & 0xff);

    int tries = 0;
    do {
        DWORD nBytesReturned;
        BOOL succ = DeviceIoControl(MXSMBUS, IOCTL_MXSMBUS_I2C, &packet, sizeof packet, &packet,
                                    sizeof packet, &nBytesReturned, NULL);
        if (!succ || nBytesReturned != sizeof packet) {
            amiDebugLog("Failed!! %d", GetLastError());
            return -9;
        }
        if (packet.status == 0) break;
        if (packet.status != 0x18) return -9;
        tries += 1;
        Sleep(0x10);
    } while (tries < 5);
    return tries == 5 ? -9 : 0;
}

bool mxkSmbusI2CReadBlock(unsigned char smbAddress, unsigned char block, unsigned char *buffer,
                          unsigned char nbytes) {
    DWORD nBytesReturned;
    smb_packet packet;
    packet.status = 0;
    packet.command = 11;
    packet.addr = smbAddress;
    packet.command_code = block;
    packet.nbytes = nbytes;

    for (int tries = 0; tries < 5; tries++) {
        BOOL succ = DeviceIoControl(MXSMBUS, IOCTL_MXSMBUS_REQUEST, &packet, sizeof packet, &packet,
                                    sizeof packet, &nBytesReturned, NULL);
        if (!succ || nBytesReturned != sizeof packet) return false;
        if (packet.status == 0) {
            memcpy(buffer, packet.data, nbytes);
            return true;
        }
        if (packet.status != 24) return false;
        Sleep(16);
    }
    return false;
}
