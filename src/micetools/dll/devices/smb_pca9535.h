#include "smbus.h"

smbus_callback_t smbus_PCA9535_write;
smbus_callback_t smbus_PCA9535_read;

#define PCA9535_GND 0
#define PCA9535_VCC 1

#define PCA9535_PIN_A0 PCA9535_GND
#define PCA9535_PIN_A1 PCA9535_GND
#define PCA9535_PIN_A2 PCA9535_GND

#define PCA9535_ADDRESS \
    (0b0100'000 | (PCA9535_PIN_A2 << 2) | (PCA9535_PIN_A1 << 1) | PCA9535_PIN_A0)
