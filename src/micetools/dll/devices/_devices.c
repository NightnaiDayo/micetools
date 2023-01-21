#include "_devices.h"

void install_devices() {
    install_led_bd();
    install_touch_bd();
    install_aime_bd();

    smbus_install(0x20, &smbus_PCA9535_write, &smbus_PCA9535_read);

    // mxkN2CmdWriteData(byte addr,byte *data,ushort nbytes,ushort *nbytesOut)
    // -> smbusWriteByte(addr, command:0xcc, byte:data[i])
    // -> smbusWriteI2C(addr, command:0xcc, data+i, n)
    smbus_install(0x30, &smbus_N2_write, &smbus_N2_read);

    // mxkDsExioRequestComputeMac() -> smbusWriteByte(addr:0x54, command:0x5c, byte:0x94)
    smbus_install(0x54, &smbus_EXIO_write, &smbus_EXIO_read);

    // mxkDsKeychipComputeMac
    smbus_install(0x55, &smbus_DS_write, &smbus_DS_read);

    smbus_install(0x57, &smbus_AT24C64AN_write, &smbus_AT24C64AN_read);
}
