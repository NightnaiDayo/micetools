#include <Windows.h>
//
#include <SetupAPI.h>
#include <stdio.h>
#include <stdlib.h>
#pragma comment(lib, "Setupapi.lib")

#include "../lib/am/amEeprom.h"

HANDLE mxkSmbusCreateDeviceFile() {
    SP_DEVICE_INTERFACE_DATA interfaceData;
    SP_DEVICE_INTERFACE_DETAIL_DATA_A interfaceDetail[204];

    HDEVINFO DeviceInfoSet =
        SetupDiGetClassDevsA(&MXSMBUS_GUID, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (DeviceInfoSet == INVALID_HANDLE_VALUE) {
        return INVALID_HANDLE_VALUE;
    }

    interfaceData.cbSize = 28;
    BOOL s;
    s = SetupDiEnumDeviceInterfaces(DeviceInfoSet, NULL, &MXSMBUS_GUID, 0, &interfaceData);
    if (!s) {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        return INVALID_HANDLE_VALUE;
    }

    interfaceDetail[0].cbSize = 5;
    s = SetupDiGetDeviceInterfaceDetailA(DeviceInfoSet, &interfaceData, interfaceDetail,
                                         sizeof interfaceDetail, NULL, NULL);
    if (!s) {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
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

typedef unsigned char byte;
typedef unsigned int uint;

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

HANDLE MXSMBUS;

int smbusWriteI2C(byte addr, byte command, byte* buffer, byte nbytes) {
    i2c_packet io_buf;
    io_buf.addr = addr;
    io_buf.command_code = (command << 8) | *buffer;
    io_buf.nbytes = (char)nbytes - 1;
    io_buf.status = 0;
    io_buf.command = 8;
    memcpy(io_buf.data, buffer + 1, (uint)io_buf.nbytes);

    byte bVar3;
    for (bVar3 = 0; bVar3 < 5; bVar3++) {
        DWORD bytesReturned;
        BOOL success = DeviceIoControl(MXSMBUS, IOCTL_MXSMBUS_I2C, &io_buf, sizeof io_buf, &io_buf,
                                       sizeof io_buf, &bytesReturned, NULL);
        if (!success || bytesReturned != sizeof io_buf) {
            return -9;
        }
        if (io_buf.status == 0) break;
        if (io_buf.status != 24) return -9;

        Sleep(16);
    }
    return bVar3 == 5 ? -9 : 0;
}

int dsReadByte(byte addr, byte* readByte, byte cmd_code) {
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

bool mxkDsExioWaitNotBusy(void) {
    while (true) {
        byte val = 0;
        if (dsReadByte(0x54, &val, 0) == 0) return true;

        Sleep(10);
    }
    return false;
}

int mxkDsExioWriteInputBuffer(byte* buffer) {
    uint offset = 0;
    uint uVar1 = 0;
    while (uVar1 < 64) {
        uint nbytes;
        if (64 - uVar1 < 9) {
            nbytes = 64 - offset & 0xffff;
        } else {
            nbytes = 8;
        }

        if (smbusWriteI2C(0x54, offset & 0xff, buffer, nbytes & 0xff) != 0) return -9;
        if (!mxkDsExioWaitNotBusy()) return -10;

        offset += nbytes;
        uVar1 = offset & 0xffff;
        buffer += nbytes;
    }
    return 0;
}

int smbusWriteByte(byte v_addr, byte command_code, byte data) {
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

    if (!success) {
        printf("DIO failed %03x\n", GetLastError());
        return -9;
    }
    if (packet.status != 0) {
        printf("Packet status: %d\n", packet.status);
        return -9;
    }
    if (bytesReturned != sizeof packet) return -9;
    return 0;
}

int mxkDsExioRequestComputeMac(void) {
    if (smbusWriteByte(0x54, 0x5c, 0x94) != 0) return -9;
    if (!mxkDsExioWaitNotBusy()) return -10;

    return 0;
}

int mxkDsExioReadMacOutputBuffer(byte* macBuffer) {
    byte cmd_code = 0x40;
    for (int i = 0; i < 20; i++, cmd_code++) {
        if (dsReadByte(0x54, &(macBuffer[i]), cmd_code) != 0) return -8;
    }
    return 0;
}

int mxkDsKeychipReadEeprom(byte* pageData, byte page) {
    if (page > 3) return -2;

    byte addr = page << 5;
    for (int i = 0; i < 32; i++, addr++) {
        if (dsReadByte(0x55, &(pageData[i]), addr) != 0) return -8;
    }
    return 0;
}

byte INPUT_BUFFER[64] = {
    // Random (0:4 bytes --> [takes the first 4 bytes of the secret])
    0,
    0,
    0,
    0,
    // Eeprom (4:32 bytes)
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    // Challenge low (36:4 bytes)
    0x76,
    0x76,
    0x76,
    0x76,
    // Page (40:1 byte)
    0x43,
    // ID (41:7 bytes)
    0x41,
    0x42,
    0x43,
    0x44,
    0x45,
    0x46,
    0x47,
    // Random (48:4 bytes --> takes the last 4 bytes of the secret)
    0,
    0,
    0,
    0,
    // Challenge high (52:3 bytes)
    0x77,
    0x77,
    0x77,
    // Random (55:9 bytes)
    0,  // --> takes value 0x80
    0,  // --> takes 0
    0,  //   > ...
    0,  //   > ...
    0,  //   > ...
    0,  //   > ...
    0,  //   > ...
    0,  // --> takes value 0x01
    0,  // --> takes value 0xB8
};

/**
 * Expected output:
 *
 * 1a e7 2d 24 8c b4 e3 40 fb 62 7d be 63 65 eb 25 70 5f d4 e3
 */

int exio_mac_test(byte* buffer) {
    // puts("mxkDsExioWriteInputBuffer...");
    if (mxkDsExioWriteInputBuffer(buffer)) {
        puts("mxkDsExioWriteInputBuffer failed");
        return -1;
    }
    // puts("mxkDsExioRequestComputeMac...");
    if (mxkDsExioRequestComputeMac()) {
        puts("mxkDsExioRequestComputeMac failed");
        return -1;
    }
    byte mac[20];
    // puts("mxkDsExioReadMacOutputBuffer...");
    if (mxkDsExioReadMacOutputBuffer(mac)) {
        puts("mxkDsExioReadMacOutputBuffer failed");
        return -1;
    }
    // puts("Output mac:");
    for (int i = 0; i < 20; i++) {
        printf("%02x", mac[i]);
    }
    puts("");
    return 0;
}

int keychip_ds_eeprom_dump(void) {
    byte pageData[32];
    for (byte page = 0; page < 4; page++) {
        printf("Page %d\n", page);
        if (mxkDsKeychipReadEeprom(pageData, page)) {
            puts("Read failed");
        }
        for (int i = 0; i < 32; i++) {
            printf("%02x", pageData[i]);
        }
        puts("");
    }
    return 0;
}

int main(int argc, char** argv) {
    MXSMBUS = mxkSmbusCreateDeviceFile();

    if (MXSMBUS == INVALID_HANDLE_VALUE) {
        puts("Unable to open smbus");
        return -1;
    }

    byte buffer[sizeof INPUT_BUFFER];
    for (int i = 0; i < sizeof INPUT_BUFFER; i++) {
        printf("%02d: ", i);

        memcpy(buffer, INPUT_BUFFER, sizeof INPUT_BUFFER);
        buffer[i] = 0x69;
        exio_mac_test(buffer);
    }

    // keychip_ds_eeprom_dump();

    CloseHandle(MXSMBUS);

    return 0;
}