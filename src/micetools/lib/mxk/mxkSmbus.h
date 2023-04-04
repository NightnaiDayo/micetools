#include <stdbool.h>
#include <stdint.h>

#include "../../dll/devices/smb_ds2460.h"
#include "../../dll/devices/smb_ds28cn01.h"

#define SYS_DS_ADDRESS DS2460_ADDRESS   // 54
#define KC_DS_ADDRESS DS28CN01_ADDRESS  // 55

int mxkSmbusI2CWriteCommand(unsigned char command_code, unsigned char *buffer, size_t nbytes);

int mxkSmbusI2CWriteBlock(unsigned char addr, unsigned char command, unsigned char *buffer,
                          unsigned char nbytes);
int mxkSmbusI2CReadBlock(unsigned char smbAddress, unsigned char block, unsigned char *buffer,
                         unsigned char nbytes);

int mxkSmbusWriteByte(unsigned char v_addr, unsigned char command_code, unsigned char data);
int mxkSmbusReadByte(unsigned char addr, unsigned char *readByte, unsigned char cmd_code);

HANDLE mxkSmbusCreateDeviceFile(void);
int mxkSmbusInit(void);
int mxkSmbusRequestMutex(void);
int mxkSmbusReleaseMutex(void);
