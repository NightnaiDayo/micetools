#include "_devices.h"

#include "smb_at24c64an.h"
#include "smb_ds2460.h"
#include "smb_ds28cn01.h"
#include "smb_pca9535.h"

typedef struct _device_list {
    VIRTUAL_SERIAL_DEVICE m_Device;
    struct _device_list* m_Next;
} device_list_t;
device_list_t device_list = { .m_Next = NULL };

static DWORD WINAPI dummy_comdev_thread(com_device_t* dev) {
    BYTE buffer[1];
    while (1) {
        comdev_read_blocking(dev, buffer, 1);
        log_warning(plfComm, "Idle data on port %ls: %02x", dev->com->wName, buffer[0]);
    }
}

static VIRTUAL_SERIAL_DEVICE dummySerialDevice = {
    .m_Name = "Port idler",
    .m_Entrypoint = dummy_comdev_thread,
};

inline void start_virtual_device(BYTE bPort, LPCSTR szName) {
    if (szName[0] != '\0') {
        device_list_t* device = &device_list;
        while (device->m_Next) {
            device = device->m_Next;
            if (strcmp(szName, device->m_Device.m_Name) == 0) {
                attach_com_device(bPort, &device->m_Device);
                return;
            }
        }

        log_error(plfComm, "Device %s requested by unknown! (COM%hhu)", szName, bPort);
    }

    if (MiceConfig.devices.add_idlers)
        attach_com_device(bPort, &dummySerialDevice);
    else
        detach_com_device(bPort);
}

void start_devices() {
    // start_virtual_device(1, MiceConfig.devices.com1);
    start_virtual_device(2, MiceConfig.devices.com2);
    start_virtual_device(3, MiceConfig.devices.com3);
    start_virtual_device(5, MiceConfig.devices.com5);
    start_virtual_device(6, MiceConfig.devices.com6);
    start_virtual_device(7, MiceConfig.devices.com7);
    start_virtual_device(8, MiceConfig.devices.com8);
}

void register_device(const char* name, FnComDeviceThread* thread) {
    device_list_t* tail = &device_list;
    while (tail->m_Next) tail = tail->m_Next;

    device_list_t* us = tail->m_Next = malloc(sizeof(device_list_t));

    us->m_Device.m_Name = name;
    us->m_Device.m_Entrypoint = thread;
    us->m_Next = NULL;
}

void install_devices() {
    install_led_bd();
    install_touch_bd();
    install_aime_bd();
    install_servo_15069();
    install_gacchu_guts_card();
    install_gacchu_guts_screen();

    smbus_install(/* 0x20 */ PCA9535_ADDRESS, &smbus_PCA9535_write, &smbus_PCA9535_read);

    // mxkN2CmdWriteData(byte addr,byte *data,ushort nbytes,ushort *nbytesOut)
    // -> smbusWriteByte(addr, command:0xcc, byte:data[i])
    // -> smbusWriteI2C(addr, command:0xcc, data+i, n)
    smbus_install(0x30, &smbus_N2_write, &smbus_N2_read);

    // mxkDsExioRequestComputeMac() -> smbusWriteByte(addr:0x54, command:0x5c, byte:0x94)
    smbus_install(/* 0x54 */ DS2460_ADDRESS, &smbus_ds2460_write, &smbus_ds2460_read);

    // mxkDsKeychipComputeMac
    smbus_install(/* 0x55 */ DS28CN01_ADDRESS, &smbus_ds28cn01_write, &smbus_ds28cn01_read);
    smbus_install(/* 0x57 */ AT24C64AN_ADDRESS, &smbus_AT24C64AN_write, &smbus_AT24C64AN_read);
}
