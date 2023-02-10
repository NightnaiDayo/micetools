#include "_devices.h"

#include "smb_pca9535.h"
#include "smb_at24c64an.h"
#include "smb_ds28cn01.h"
#include "smb_ds2460.h"

void install_devices() {
    install_led_bd();
    install_touch_bd();
    install_aime_bd();

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
