#include "_devices.h"

BOOL smbus_EXIO_write(ich9_cmd_t cmd, WORD code, BYTE dlen, BYTE* data) {
    switch (cmd) {
        case ICH9_CMD_BYTE_DATA:
            switch (code) {
                case 0x5c:
                    if (data[0] == 0x94) return TRUE;
                default:
                    log_error("smb-exio", "Unknown write command: %02x", code);
                    return FALSE;
            }
        case ICH9_CMD_BLOCK: {
            WORD reg = *(WORD*)(&data[-1]);
            dlen = data[1];

            // char* data_s = malloc(dlen * 3 + 1);
            // for (int i = 0; i < dlen; i++) {
            //     sprintf_s(data_s + i * 3, 3, "%02x ", data[2 + i]);
            // }
            // data_s[dlen * 3 + 1] = '\0';

            log_info("smb-exio", "Block write, %d @ %04x: ", dlen, reg);
            // free(data_s);
            return TRUE;
        }

        default:
            log_error("smb-exio", "Unsupported write mode: %01x (%02x)", cmd, code);
            return FALSE;
    }
}

BOOL smbus_EXIO_read(ich9_cmd_t cmd, WORD code, BYTE dlen, BYTE* data) {
    switch (cmd) {
        case ICH9_CMD_BYTE_DATA:
            if (0x40 <= code < 0x40 + 0x14) {
                // mxkDsExioReadMacOutputBuffer
                // This just has to match N2_I2C_CHALLENGE_RESPONSE!
                data[0] = 0x69;
                return TRUE;
            } else if (code == EXIO_GET_BUSY) {
                data[0] = 0x00;  // Anything non-zero = busy
                return TRUE;
            } else {
                log_error("smx-exio", "Unknown read command: %02x", code);
                return FALSE;
            }
        default:
            log_error("smb-exio", "Unsupported read mode: %01x (%02x)", cmd, code);
            return FALSE;
    }
}
