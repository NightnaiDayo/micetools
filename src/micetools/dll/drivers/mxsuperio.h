#pragma once
#include "common.h"

#pragma pack(1)
typedef struct {
    BYTE index;
    BYTE reg;
    BYTE data;
} superio_packet;

#pragma pack(1)
typedef struct  {
    BYTE chip;
    BYTE device;
    BYTE reg;
    BYTE data;
} superio_lpc_packet;
