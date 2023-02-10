#pragma once
#include "../comdevice.h"
#include "../common.h"
#include "smbus.h"

void install_led_bd();
void install_touch_bd();
void install_aime_bd();

smbus_callback_t smbus_N2_write;
smbus_callback_t smbus_N2_read;

void install_devices();
