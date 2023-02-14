#pragma once
#include "common.h"

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

// Keychip
// #define DS_REG_ID 0xA0
// #define DS_REG_ID_END 0xA7
// #define DS_REG_STATUS 0xA8

// #define DS_STATUS_FLAG_BUSY 2
// #define DS_I2C_CHALLENGE_RESPONSE 0xB0

#pragma pack(push, 1)
typedef struct _MXSMBUS_REQUEST_PACKET {
    BYTE status;
    BYTE command;
    BYTE v_addr;
    BYTE command_code;
    BYTE nbytes;
    BYTE data[32];
} MXSMBUS_REQUEST_PACKET, *PMXSMBUS_REQUEST_PACKET;

typedef struct _MXSMBUS_I2C_PACKET {
    BYTE status;
    BYTE command;
    WORD v_addr;
    WORD command_code;
    BYTE nbytes;
    BYTE data[32];
} MXSMBUS_I2C_PACKET, *PMXSMBUS_I2C_PACKET;
#pragma pack(pop)


typedef enum {
    ICH9_CMD_QUICK = 0b000,
    ICH9_CMD_BYTE = 0b001,
    ICH9_CMD_BYTE_DATA = 0b010,
    ICH9_CMD_WORD_DATA = 0b011,
    ICH9_CMD_PROCESS_CALL = 0b100,
    ICH9_CMD_BLOCK = 0b101,
    ICH9_CMD_I2C_READ = 0b110,
    ICH9_CMD_BLOCK_PROCESS = 0b111,
} ich9_cmd_t;

typedef enum {
    MXSMBUS_CMD_WRITE_QUICK = 0,
    MXSMBUS_CMD_READ_QUICK = 1,
    MXSMBUS_CMD_WRITE_BYTE = 2,
    MXSMBUS_CMD_READ_BYTE = 3,
    MXSMBUS_CMD_WRITE_BYTE_DATA = 4,
    MXSMBUS_CMD_READ_BYTE_DATA = 5,
    MXSMBUS_CMD_WRITE_WORD_DATA = 6,
    MXSMBUS_CMD_READ_WORD_DATA = 7,
    MXSMBUS_CMD_WRITE_BLOCK = 8,
    MXSMBUS_CMD_READ_BLOCK = 9,
    MXSMBUS_CMD_PROCESS_CALL = 10,
    MXSMBUS_CMD_I2C = 11,
} mxsmbus_cmd_t;

typedef BOOL smbus_callback_t(ich9_cmd_t cmd, WORD code, BYTE nbytes, BYTE* data);

void smbus_install(BYTE v_addr, smbus_callback_t* write, smbus_callback_t* read);
