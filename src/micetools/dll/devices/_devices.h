#pragma once
#include "../comdevice.h"
#include "../common.h"
#include "smbus.h"

void install_led_bd();
void install_touch_bd();
void install_aime_bd();

smbus_callback_t smbus_PCA9535_write;
smbus_callback_t smbus_PCA9535_read;
smbus_callback_t smbus_DS_write;
smbus_callback_t smbus_DS_read;
smbus_callback_t smbus_N2_write;
smbus_callback_t smbus_N2_read;
smbus_callback_t smbus_AT24C64AN_write;
smbus_callback_t smbus_AT24C64AN_read;
smbus_callback_t smbus_EXIO_write;
smbus_callback_t smbus_EXIO_read;

void install_devices();
