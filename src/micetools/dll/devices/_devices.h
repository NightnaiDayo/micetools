#pragma once
#include "../comdevice.h"
#include "../common.h"
#include "smbus.h"

void install_led_bd();
void install_touch_bd();
void install_aime_bd();
void install_servo_15069();
void install_gacchu_guts_card();
void install_gacchu_guts_screen();

smbus_callback_t smbus_N2_write;
smbus_callback_t smbus_N2_read;

void start_devices();
void register_device(const char* name, FnComDeviceThread* thread);

void install_devices();
