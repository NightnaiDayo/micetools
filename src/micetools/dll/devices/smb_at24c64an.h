#include "smbus.h"

smbus_callback_t smbus_AT24C64AN_write;
smbus_callback_t smbus_AT24C64AN_read;

#define AT24C64AN_GND 0
#define AT24C64AN_VCC 1

// Ref: MS-9667 rev 0A sheet 10
#define AT24C64AN_PIN_A0 AT24C64AN_VCC
#define AT24C64AN_PIN_A1 AT24C64AN_VCC
#define AT24C64AN_PIN_A2 AT24C64AN_VCC

#define AT24C64AN_ADDRESS \
    (0b1010'000 | (AT24C64AN_PIN_A2 << 2) | (AT24C64AN_PIN_A1 << 1) | AT24C64AN_PIN_A0)

void build_eeprom(void);
