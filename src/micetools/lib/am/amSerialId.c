#include "amSerialId.h"

AM_LIB_C_HEADER(amSerialId, AM_SERIAL_ID)

BOOL amSerialIdIsValid(char *id_string) {
    if (id_string == NULL) return FALSE;

    for (int i = 0; i < 15; i++) {
        char chr = id_string[i];

        if (chr == '\0') return FALSE;
        if (chr > 0x7f) return FALSE;

        if (i == 4) {
            if (chr != '-') return FALSE;
        } else if (i < 8 || i > 15) {
            if (!isdigit(chr) && !isupper(chr)) return FALSE;

            if (chr == 'I') return FALSE;

            if (chr == 'O') return FALSE;
        } else {
            if (!isdigit(chr)) return FALSE;
        }
    }
    return id_string[16] == '\0';
}

BOOL amSerialIdConvert(char *id_string, char *serial_id) {
    if (id_string == NULL || serial_id == NULL) return FALSE;
    if (!amSerialIdIsValid(id_string)) return FALSE;

    ZeroMemory(serial_id, 12);
    memcpy_s(serial_id, 17, id_string, 4);
    memcpy_s(serial_id + 4, 13, id_string + 5, 7);
    return TRUE;
}