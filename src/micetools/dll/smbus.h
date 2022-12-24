#pragma once
#include "common.h"

// PCA9535 (DIPSW)
#define PCA9535_IN0 0x00
#define PCA9535_IN1 0x01
#define PCA9535_OUT0 0x02
#define PCA9535_OUT1 0x03
#define PCA9535_INV0 0x04
#define PCA9535_INV1 0x05
#define PCA9535_CONF0 0x06
#define PCA9535_CONF1 0x07

/*
W83627UHG:
    Configuration is based on HEFRAS
    Configuration 0:
        [Logical index port] 0x2e
        [Logical data port] 0x2f
    Configuration 1:
        [Logical index port] 0x4e
        [Logical data port] 0x4f

    Setup:
        ! Select logical device 0 = FDC (in this case, we don't care)
        index = 0x07, data = 0x00
        index = 0x20, read data  (Chip ID MSB)
        index = 0x21, read data  (Chip ID LSB)
        [merge two bytes to get chip ID]

        ! Select logical device B = HM
        * Read base address of address and data ports
        * Address = xxxh + 5h, Data = xxxh + 6h
        index = 0x07, data = 0x0b
        index = 0x60, read data  (Base MSB)
        index = 0x61, read data  (Base LSB)
        [merge two bytes to get address]

ICH9 Smbus Controller:
    mxsmbus.sys -> io-controller-hub-9-datasheet.pdf, page 759
    PEC is disabled

    Requests are made using a virtual address.
    The physical address is *2 of the virtual, +1 for read.

    Commands:
     0: paddr = vaddr * 2    ; smb_cmd = quick
     1: paddr = vaddr * 2 + 1; smb_cmd = quick
     2: paddr = vaddr * 2    ; smb_cmd = byte
     3: paddr = vaddr * 2 + 1; smb_cmd = byte
     4: paddr = vaddr * 2    ; smb_cmd = byte data
     5: paddr = vaddr * 2 + 1; smb_cmd = byte data
     6: paddr = vaddr * 2    ; smb_cmd = word data
     7: paddr = vaddr * 2 + 1; smb_cmd = word data
     8: paddr = vaddr * 2    ; smb_cmd = block
     9: paddr = vaddr * 2 + 1; smb_cmd = block
    10: paddr = vaddr * 2    ; smb_cmd = process call
    11: paddr = vaddr * 2    ; smb_cmd = i2c read

    IOCTL_MXSMBUS_REQUEST:
        assert command <= 11
        0x25 bytes
    IOCTL_MXSMBUS_I2C:
        assert command <= 10
        0x27 bytes


LPC device 0x00 = FPC
LPC device 0x07 = W83627UHG



smbus:
    0x801:
        PCA9535: 0000 100[Wr] | 8 bit addr/reg | [...
        amHmI2C: 0000 100[Wr] | 8 bit addr/reg | [...

superio:
    0x803:
        amHmLPC:


IOCTL_MXSUPERIO_READ:
    read 1: -> .. 0B 20 00
            <- .. .. .. XX
    read 2: -> .. 0B 21 00
            <- .. .. .. YY
    assert XXYY == 0xA020 (case 1) or 0xA230 (case 2)

! SMBUS is I2C
- ICH9: **host** (schematics, sheet 11)

- EEPROM AT24C64AN: 0AE (schematics, sheet 10)
- DDR2 DIMM A1: 0A0 (schematics, sheet 13)
- DDR2 DIMM B1: 0A4 (schematics, sheet 14)

- ICS9LPRS908: 0x?? (schematics, sheet 15)
- W83627UHG LPC I/O: 0x?? (schematics, sheet 16) ((2E / 4E on a toggle?))
- Mystery chips on the mezanine and keychip (schematics, sheet 17)
- VRMs: 0x?? (schematics, sheet 27)

! ??? is SPI

! System SPI
- SPI Flash ROM: ??? (schematics, sheet 11)
-

! LPC (superio) is its own thing?
- ICH9: **host** (schematics, sheet 11)
- TPM (schematics, sheet 10)
- W83627UHC (schematics, sheet 16)
*/

// SMBUS Devices (+1 is implied for read addresses)
#define SMBUS_PCA9535 0x40
#define SMBUS_DIMM_A1 0xA0
#define SMBUS_DIMM_B1 0xA4
#define SMBUS_EXIO 0xA8
#define SMBUS_N2 0xAA
#define SMBUS_EEPROM 0xAE

// Keychip
#define N2_GET_EEPROM 0x40
#define N2_GET_UNIQUE_NUMBER 0xA0
#define N2_GET_STATUS 0xA8
#define N2_STATUS_FLAG_BUSY 2
#define N2_I2C_CHALLENGE_RESPONSE 0xB0

#define EXIO_GET_BUSY 0x00

#pragma pack(1)
typedef struct mxsmbus_request_packet_ {
    BYTE status;
    BYTE prt;
    BYTE addr;
    BYTE reg;
    BYTE dlen;
    BYTE data[32];
} mxsmbus_request_packet;

#pragma pack(1)
typedef struct mxsmbus_i2c_packet_ {
    BYTE status;
    BYTE prt;
    WORD addr;
    WORD reg;
    BYTE dlen;
    BYTE data[32];
} mxsmbus_i2c_packet;
