#pragma once

#include <Windows.h>

#include "../_am.h"

AM_LIB_H_HEADER(amSerialId, AM_SERIAL_ID)

typedef struct AM_SERIAL_ID {
    unsigned char Rsv00;
} AM_SERIAL_ID;

BOOL amSerialIdIsValid(char *id_string);
BOOL amSerialIdConvert(char *id_string, char *serial_id);
