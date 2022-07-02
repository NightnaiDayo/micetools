#pragma once
#include "common.h"

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
