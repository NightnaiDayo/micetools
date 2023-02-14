#include "smb_pca9535.h"

#include "../key_config.h"

static uint16_t pca9535_config = 0xffff;

static uint8_t out[2] = { 0xFF, 0xFF };
static uint8_t inv[2] = { 0x00, 0x00 };
/**
 * RingEdge config (will be set by amlib):
 *   config = { 0xFF, 0xFC }
 *
 * LEDs are on IO1_0 and IO1_1
 */
static uint8_t config[2] = { 0xFF, 0xFF };

BOOL smbus_PCA9535_write(ich9_cmd_t cmd, WORD code, BYTE dlen, BYTE* data) {
    switch (cmd) {
        case ICH9_CMD_BYTE_DATA:
            switch (code) {
                case PCA9535_REG_IN0:
                    return FALSE;
                case PCA9535_REG_IN1:
                    return FALSE;

                case PCA9535_REG_OUT0:
                    out[0] = data[0];
                    return TRUE;
                case PCA9535_REG_OUT1:
                    out[1] = data[0];
                    return TRUE;

                case PCA9535_REG_INV0:
                    inv[0] = data[0];
                    return TRUE;
                case PCA9535_REG_INV1:
                    inv[1] = data[0];
                    return TRUE;

                case PCA9535_REG_CONF0:
                    config[0] = data[0];
                    return TRUE;
                case PCA9535_REG_CONF1:
                    config[1] = data[0];
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

/**
 * Supported resolutions by amPlatform:
 *  640x480: VGA
 *  1024x600: WSVGA
 *  1024x768: XGA
 *  1280x720: 720P
 *  1280x1024: WUXGA
 *  1360x768: WVGA
 *  1920x1080: 1080P
 *  1440x900: WXGAPlus
 *  1680x1050: WSXGAPlus
 *  1600x1200: UXGA
 *  1920x1200: WYXGA
 *  1280x768: WXGA1280
 *  800x600: SVGA
 *  800x480: WVGA
 *
 * DIPSW Resolutions:
 *  [0]: 640x480
 *  [1]: 640x480
 *  [2]: 1024x600
 *  [3]: 1024x768
 *  [4]: 1280x720
 *  [5]: 1280x1024
 *  [6]: 1360x768
 *  [7]: 1920x1080
 */

#define DIPSW_LANDSCAPE 0b0000'0'000
#define DIPSW_PORTRAIT 0b0000'1'000

#define DIPSW_RES_DEFAULT 0b0'000'0000  // = 640x480
#define DIPSW_RES_640x480 0b0'001'0000
#define DIPSW_RES_1024x600 0b0'010'0000
#define DIPSW_RES_1024x768 0b0'011'0000
#define DIPSW_RES_1280x720 0b0'100'0000
#define DIPSW_RES_1280x1024 0b0'101'0000
#define DIPSW_RES_1360x768 0b0'110'0000
#define DIPSW_RES_1920x1080 0b0'111'0000

BOOL smbus_PCA9535_read(ich9_cmd_t cmd, WORD code, BYTE dlen, BYTE* data) {
    switch (cmd) {
        case ICH9_CMD_BYTE_DATA:
            switch (code) {
                case PCA9535_REG_IN0:  // DIPSW
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

                    data[0] = ((~MiceConfig.sysconf.dipsw) ^ inv[0]) & config[0];
                    return TRUE;
                case PCA9535_REG_IN1:  // SW1/2 + extras
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
                    byte dip = 0x04 | 0x08;
                    static bool lastTest = false;
                    static bool lastSystem = false;

                    if (keySystemTest && GetAsyncKeyState(keySystemTest) < 0) {
                        // if (!lastSystem)
                         dip &= ~0x04;
                        // lastSystem = true;
                    } else
                        lastSystem = false;
                    if (keySystemService && GetAsyncKeyState(keySystemService) < 0) {
                        // if (!lastTest)
                        dip &= ~0x08;
                        // lastTest = true;
                    } else
                        lastTest = false;

                    data[0] = (dip ^ inv[1]) & config[1];
                    return TRUE;

                case PCA9535_REG_OUT0:
                    data[0] = out[0] & ~config[0];
                    return FALSE;
                case PCA9535_REG_OUT1:
                    data[0] = out[1] & ~config[1];
                    return TRUE;
                case PCA9535_REG_INV0:
                    data[0] = inv[0];
                    return TRUE;
                case PCA9535_REG_INV1:
                    data[0] = inv[1];
                    return TRUE;

                case PCA9535_REG_CONF0:
                    data[0] = config[0];
                    return TRUE;
                case PCA9535_REG_CONF1:
                    data[0] = config[1];
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
