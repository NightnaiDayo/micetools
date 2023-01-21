#include "_devices.h"

/**
 * TODO: This whole device, properly
 */

BOOL smbus_DS_write(ich9_cmd_t cmd, WORD code, BYTE dlen, BYTE* data) {
    static unsigned char challenge[7];

    switch (cmd) {
        case ICH9_CMD_BLOCK: {
            switch (code) {
                case 0xa9d0:
                case 0xa9d1:
                case 0xa9d2:
                case 0xa9d3: {
                    if (dlen != 7) {
                        log_error("smb-ds", "Expected challenge length of 7 (saw %d)!", dlen);
                        return FALSE;
                    }

                    memcpy(challenge, &(data[2]), 7);

                    char* challenge_s = malloc(dlen * 3 + 1);
                    if (challenge_s == NULL) {
                        log_info("smb-ds", "Challenge: (buffer failed)");
                        return TRUE;
                    }

                    for (int i = 0; i < dlen; i++) {
                        sprintf_s(challenge_s + i * 3, 4, "%02x ", data[2 + i]);
                    }
                    challenge_s[dlen * 3 + 1] = '\0';
                    log_info("smb-ds", "Challenge: %s", challenge_s);
                    free(challenge_s);
                    return TRUE;
                }
                default:
                    log_error("smb-ds", "Unknown write command: %04x", code);
            }
        }
        case ICH9_CMD_I2C_READ: {
            switch (code) {
                case DS_I2C_CHALLENGE_RESPONSE:
                    // This just has to match EXIO!
                    for (int i = 0; i < dlen; i++) data[i] = 0x69;
                    return TRUE;
                default:
                    log_error("smb-ds", "Unknown I2C read command: %04x", code);
            }
        }
        default:
            log_error("smb-ds", "Unsupported write mode: %01x (%04x)", cmd, code);
            return FALSE;
    }
}

BOOL smbus_DS_read(ich9_cmd_t cmd, WORD code, BYTE dlen, BYTE* data) {
    static unsigned char DS_eeprom[3][0x20];

    switch (cmd) {
        case ICH9_CMD_BYTE_DATA:
            if (code < 0x80) {
                BYTE page = (code >> 5) & 0b11;
                BYTE offset = code & 0x1f;

                data[0] = DS_eeprom[page][offset];

                return TRUE;
            } else if (code >= DS_GET_UNIQUE_NUMBER && code < DS_GET_UNIQUE_NUMBER + 0xf) {
                data[0] = 0x04;  // chosen by fair dice roll.
                return TRUE;
            } else if (code == DS_GET_STATUS) {
                // Polled until DS_STATUS_FLAG_BUSY low
                data[0] = 0x00;
                return TRUE;
            } else {
                log_error("smb-ds", "Unknown read command: %04x", code);
                return FALSE;
            }
        default:
            log_error("smb-ds", "Unsupported read mode: %01x (%04x)", cmd, code);
            return FALSE;
    }
}
