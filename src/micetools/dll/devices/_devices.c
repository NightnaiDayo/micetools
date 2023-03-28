#include "_devices.h"

#include "smb_at24c64an.h"
#include "smb_ds2460.h"
#include "smb_ds28cn01.h"
#include "smb_pca9535.h"

typedef struct _device_list {
    const char* m_Name;
    FnComDeviceThread* m_Thread;
    struct _device_list* m_Next;
} device_list_t;
device_list_t device_list = { .m_Next = NULL };

#define _start_device_n(n) \
    if (strcmp(MiceConfig.devices.com##n, name) == 0) attach_com_device(n, thread)

inline void start_device(const char* name, FnComDeviceThread* thread) {
    _start_device_n(1);
    _start_device_n(2);
    _start_device_n(3);
    _start_device_n(5);
    _start_device_n(6);
    _start_device_n(7);
    _start_device_n(8);
}

#define _stop_if_unregistered(n) \
    if (MiceConfig.devices.com##n[0] == '\0') detach_com_device(n)

inline void stop_old_devices() {
    _stop_if_unregistered(1);
    _stop_if_unregistered(2);
    _stop_if_unregistered(3);
    _stop_if_unregistered(5);
    _stop_if_unregistered(6);
    _stop_if_unregistered(7);
    _stop_if_unregistered(8);
}

void start_devices() {
    stop_old_devices();

    device_list_t* device = &device_list;
    while (device->m_Next) {
        device = device->m_Next;
        start_device(device->m_Name, device->m_Thread);
    }
}

void register_device(const char* name, FnComDeviceThread* thread) {
    device_list_t* tail = &device_list;
    while (tail->m_Next) tail = tail->m_Next;

    device_list_t* us = tail->m_Next = malloc(sizeof(device_list_t));

    us->m_Name = name;
    us->m_Thread = thread;
    us->m_Next = NULL;
}

void install_devices() {
    install_led_bd();
    install_touch_bd();
    install_aime_bd();
    install_servo_15069();

    start_devices();

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
