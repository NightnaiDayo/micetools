#pragma once

#include <stdbool.h>

#define JVS_IO_MAX 31
#define JVS_BUTTON_PAIR_COUNT 15

struct {
    int buttons[JVS_BUTTON_PAIR_COUNT * 2];
    bool invert[JVS_BUTTON_PAIR_COUNT * 2];
    int test;
} jvsKeybindings[JVS_IO_MAX];

int keySystemTest = 0;
