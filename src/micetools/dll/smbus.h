#pragma once
#include "common.h"

// PCA9535 (DIPSW)
#define PCA9535_WRITE 0x04
#define PCA9535_READ 0x05

#define PCA9535_IN0 0x00
#define PCA9535_IN1 0x01
#define PCA9535_OUT0 0x02
#define PCA9535_OUT1 0x03
#define PCA9535_INV0 0x04
#define PCA9535_INV1 0x05
#define PCA9535_CONF0 0x06
#define PCA9535_CONF1 0x07

#define SMBUS_PCA9535 0x20
#define SMBUS_EEPROM 0x57  // Doesn't line up with manual!

#define SMBUS_DDR2_DIMM_A1 0x000   // what does 0xA0 mean?
#define SMBUS_DDR2_DIMM_B1 0x010   // what does 0xA4 mean?
#define SMBUS_EEPROM_ 0x0AE        // = AT24C64AN
#define SMBUS_ICS9LPRS908 0xfff    // Unknown
#define SMBUS_W83627UHG 0xfff      // Unknown; hwmon. Possibly 0x2e or 0x4e
#define SMBUS_UPI_UP6261BM8 0xfff  // Unknown; vref
#define SMBUS_UPI_ISL6322CR 0xfff  // Unknown; vrm
// SMBUS is send onto the mezzanine board!

#pragma pack(1)
typedef struct mxsmbus_request_packet_ {
    BYTE status;
    BYTE prt;
    WORD addr;
    WORD reg;
    BYTE dlen;
    BYTE data[32];
} mxsmbus_request_packet;

#pragma pack(1)
typedef struct mxsmbus_i2c_packet_ {
    BYTE status;
    BYTE prt;
    BYTE addr;
    BYTE reg;
    BYTE dlen;
    BYTE data[32];
} mxsmbus_i2c_packet;

#pragma pack(1)
typedef struct mxsuperio_lpc_packet_ {
    BYTE index;
    BYTE reg;
    BYTE data;
} mxsuperio_lpc_packet;
enum mxsbus_status { MXSBUS_OKAY = 0 };
