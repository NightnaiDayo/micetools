#include "_devices.h"

uint16_t pca9535_config = 0xffff;

BOOL smbus_PCA9535_write(ich9_cmd_t cmd, WORD code, BYTE dlen, BYTE* data) {
    switch (cmd) {
        case ICH9_CMD_BYTE_DATA:
            switch (code) {
                case PCA9535_IN0:
                    log_error("pca9535", "Write PCA9535_IN0 unimplemented!");
                    return FALSE;
                case PCA9535_IN1:
                    log_error("pca9535", "Write PCA9535_IN1 unimplemented!");
                    return FALSE;
                case PCA9535_OUT0:
                    log_info("pca9535", "Out 0: %02x", data[0]);
                    return TRUE;
                case PCA9535_OUT1:
                    log_info("pca9535", "Out 1: %02x", data[0]);
                    return TRUE;
                case PCA9535_INV0:
                    log_info("pca9535", "Inv 0: %02x", data[0]);
                    return TRUE;
                case PCA9535_INV1:
                    log_info("pca9535", "Inv 1: %02x", data[0]);
                    return TRUE;
                case PCA9535_CONF0:
                    log_info("pca9535", "Conf 0: %02x", data[0]);
                    pca9535_config = (data[0] << 8) | (pca9535_config & 0xff);
                    return TRUE;
                case PCA9535_CONF1:
                    log_info("pca9535", "Conf 1: %02x", data[0]);
                    pca9535_config = data[0] | (pca9535_config & 0xff00);
                    return TRUE;
                default:
                    log_error("pca9535", "Unknown write command: %02x", code);
                    return FALSE;
            }
        default:
            log_error("pca9535", "Unsupported write mode: %01x (%02x)", cmd, code);
            return FALSE;
    }
}

#define DIPSW_PORTRAIT 0b0000'0'000
#define DIPSW_LANDSCAPE 0b0000'1'000

#define DIPSW_RES_1920x1080 0b0'111'0000
#define DIPSW_RES_1360x768 0b0'110'0000
#define DIPSW_RES_1280x1024 0b0'101'0000
#define DIPSW_RES_1280x720 0b0'100'0000
#define DIPSW_RES_1024x768 0b0'011'0000
#define DIPSW_RES_1024x600 0b0'010'0000
#define DIPSW_RES_640x480 0b0'001'0000
#define DIPSW_RES_DEFAULT 0b0'000'0000

BOOL smbus_PCA9535_read(ich9_cmd_t cmd, WORD code, BYTE dlen, BYTE* data) {
    switch (cmd) {
        case ICH9_CMD_BYTE_DATA:
            switch (code) {
                case PCA9535_IN0:  // DIPSW
                    /*
                    0: ?
                    1: ?
                    2: ?
                    3: Orientation
                    4: /            \
                    5: | Resolution |
                    6: \           /
                    7: game specific

                    0b00001000 = landscape
                    */
                    // data[0] = 0b00001000;
                    // data[0] = 0b0'111'0'000;

                    data[0] = DIPSW_LANDSCAPE | DIPSW_RES_DEFAULT;

                    return TRUE;
                case PCA9535_IN1:  // SW1/2 + extras
                    /*
                        0: unk
                        1: unk
                        2: ¬test
                        3: ¬service
                        4: unk
                        5: unk
                        6: unk
                        7: unk
                    */
                    byte dip = 0x00;
                    if (GetAsyncKeyState('T') >= 0) {
                        dip |= 0x04;
                    }
                    if (GetAsyncKeyState('S') >= 0) {
                        dip |= 0x08;
                    }
                    data[0] = dip;
                    return TRUE;
                case PCA9535_OUT0:
                    log_error("pca9535", "Read PCA9535_OUT0 unimplemented!");
                    return FALSE;
                case PCA9535_OUT1:
                    data[0] = 0x00;
                    return TRUE;
                case PCA9535_INV0:
                    data[0] = 0x00;
                    return TRUE;
                case PCA9535_INV1:
                    data[0] = 0x00;
                    return TRUE;
                case PCA9535_CONF0:
                    data[0] = pca9535_config >> 8;
                    return TRUE;
                case PCA9535_CONF1:
                    data[0] = pca9535_config & 0xff;
                    return TRUE;
                default:
                    log_error("pca9535", "Unknown read command: %02x", code);
                    return FALSE;
            }
        default:
            log_error("pca9535", "Unsupported read mode: %01x (%02x)", cmd, code);
            return FALSE;
    }
}
