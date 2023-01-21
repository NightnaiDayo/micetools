#pragma once
#include "common.h"

#pragma pack(push, 1)
typedef struct {
    BYTE index;
    BYTE reg;
    BYTE data;
} superio_packet;

typedef struct  {
    BYTE chip;
    BYTE device;
    BYTE reg;
    BYTE data;
} superio_lpc_packet;
#pragma pack(pop)
