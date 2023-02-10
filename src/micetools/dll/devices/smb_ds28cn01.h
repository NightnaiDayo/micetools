#include "smbus.h"

smbus_callback_t smbus_ds28cn01_write;
smbus_callback_t smbus_ds28cn01_read;

// Specification

#define DS28CN01_EEPROM_PAGES 4
#define DS28CN01_EEPROM_PAGE_SIZE 32
#define DS28CN01_EEPROM_SIZE (DS28CN01_EEPROM_PAGES * DS28CN01_EEPROM_PAGE_SIZE)

#define DS28CN01_UNIQUE_NUMBER_SIZE 8
#define DS28CN01_MAC_LEN 20

// Registers
#define DS28CN01_REG_NUM 0xA0
#define DS28CN01_REG_NUM_END (DS28CN01_REG_NUM + DS28CN01_UNIQUE_NUMBER_SIZE - 1)
#define DS28CN01_REG_STATUS 0xA8
#define DS28CN01_REG_MAC 0xB0

// Flags
#define DS28CN01_STATUS_FLAG_BUSY 2

// Wiring
#define DS28CN01_GND 0
#define DS28CN01_VCC 1
#define DS28CN01_SCL 2
#define DS28CN01_SDA 3

#define DS28CN01_PIN_AD1 DS28CN01_VCC
#define DS28CN01_PIN_AD0 DS28CN01_VCC

#define DS28CN01_ADDRESS                          \
    (0b101'00'00 |                                \
     (((DS28CN01_PIN_AD1 == DS28CN01_GND)   ? 0b00  \
       : (DS28CN01_PIN_AD1 == DS28CN01_VCC) ? 0b01  \
       : (DS28CN01_PIN_AD1 == DS28CN01_SCL) ? 0b10  \
                                          : 0b11) \
      << 2) |                                     \
     ((DS28CN01_PIN_AD0 == DS28CN01_GND)   ? 0b00   \
      : (DS28CN01_PIN_AD0 == DS28CN01_VCC) ? 0b01   \
      : (DS28CN01_PIN_AD0 == DS28CN01_SCL) ? 0b10   \
                                         : 0b11))
