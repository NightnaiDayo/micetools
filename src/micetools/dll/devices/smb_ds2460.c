/**
 * This board is located on the mezzanine board in the Ring (IC7).
 *
 * Similar to the DS28CN01, this is a SHA-1 coprocessor with EEPROM
 * storage.
 *
 * The pins are wired as:
 *
 * pin | name | connection
 *  1  | AD0  | GND
 *  2  | AD1  | Floating
 *  3  | AD2  | Floating
 *  4  | GND  | GND
 *  5  | NC   |
 *  6  | SDA  | SMData
 *  7  | SCL  | SMClock
 *  8  | Vcc  | VCC3
 *
 * Pins 2 and 3 are floating, when they should be pulled to Vcc. It is
 * therefore assumed that there's an internal pull-up on all three AD
 * pins, as otherwise this device would be at address 0x40!
 *
 * In code it is referred to as the "ds exio" device, complemeting the
 * "ds" device on the keychip.
 */

#include "smb_ds2460.h"

#include <openssl/evp.h>

#include "_ds_sha.h"

static BYTE USER_EEPROM[DS2460_EEPROM_SIZE];
static BYTE INPUT_BUFFER[DS2460_INPUT_BUFFER_SIZE];

static BYTE COMPUTED_MAC[DS2460_MAC_LEN];

static BYTE SECRETS[4][8] = {
    // S-Secret
    { 0 },
    // E-Secret 1
    { 0 },
    // E-Secret 2  <-- The one that's used
    { 0 },
    // E-Secret 3
    { 0 },
};

void ds2460_update_mac(bool gpSha, byte* secret) {
    memcpy(&INPUT_BUFFER[0], &secret[0], 4);
    memcpy(&INPUT_BUFFER[48], &secret[4], 4);
    INPUT_BUFFER[55] = 0x80;
    for (int i = 56; i < 62; i++) INPUT_BUFFER[i] = 0;
    INPUT_BUFFER[62] = 0x01;
    INPUT_BUFFER[63] = 0xB8;

    puts("ds2460: SHA1 input buffer:");
    for (int i = 0; i < DS2460_INPUT_BUFFER_SIZE; i++) {
        printf("%02x", INPUT_BUFFER[i]);
    }
    puts("");

    if (gpSha) {
        EVP_MD_CTX* ctx = EVP_MD_CTX_create();
        EVP_DigestInit(ctx, EVP_sha1());
        EVP_DigestUpdate(ctx, INPUT_BUFFER, DS2460_INPUT_BUFFER_SIZE);
        unsigned int outlen;
        EVP_DigestFinal_ex(ctx, COMPUTED_MAC, &outlen);
        EVP_MD_CTX_destroy(ctx);
    } else {
        ComputeDallasSha(INPUT_BUFFER, (long*)&COMPUTED_MAC[0], (long*)&COMPUTED_MAC[4],
                         (long*)&COMPUTED_MAC[8], (long*)&COMPUTED_MAC[12],
                         (long*)&COMPUTED_MAC[16]);
    }
}

BOOL smbus_ds2460_write(ich9_cmd_t cmd, WORD code, BYTE dlen, BYTE* data) {
    switch (cmd) {
        case ICH9_CMD_BYTE_DATA:
            switch (code) {
                case DS2460_MEM_CMD:
                    /**
                     * 0x94 =
                     * | 0x80 -> DS2460_CMD_COMPUTE
                     * | 0x10 -> (secret = ESecret2) << 3
                     * | 0x04 -> DS2460_CMD_COMPUTE_TO_MAC_BUFFER
                     */
                    if (!(data[0] & DS2460_CMD_COMPUTE)) {
                        log_error("ds2460", "Unknown command: %02x", data[0]);
                        return FALSE;
                    }
                    BYTE numSecret = (data[0] >> 3) & 3;
                    BOOL toMacBuffer = !!(data[0] & DS2460_CMD_COMPUTE_TO_MAC_BUFFER);
                    BYTE gpSha = !!(data[0] & DS2460_CMD_COMPUTE_GP_SHA);

                    if (data[0] != DS2460_REQUEST_COMPUTE_MAC) {
                    }

                    ds2460_update_mac(gpSha, SECRETS[numSecret]);
                    return TRUE;
                default:
                    log_error("ds2460", "Unknown write command: %02x", code);
                    return FALSE;
            }
        case ICH9_CMD_BLOCK: {
            BYTE offset = code >> 8;

            // One extra byte of data is stuffed into the command code
            if (offset < DS2460_INPUT_BUFFER_SIZE) INPUT_BUFFER[offset] = code & 0xff;
            if (offset + 1 < DS2460_INPUT_BUFFER_SIZE)
                memcpy_s(&INPUT_BUFFER[offset + 1], DS2460_INPUT_BUFFER_SIZE - (offset + 1), data,
                         dlen);

            log_info("ds2460", "Block write, %d @ %04x: ", dlen + 1, offset);
            return TRUE;
        }

        default:
            log_error("ds2460", "Unsupported write mode: %01x (%02x)", cmd, code);
            return FALSE;
    }
}

BOOL smbus_ds2460_read(ich9_cmd_t cmd, WORD code, BYTE dlen, BYTE* data) {
    switch (cmd) {
        case ICH9_CMD_BYTE_DATA:
            if (code == DS2460_MEM_SHA) {
                // if we were busy processing something we'd:
                // return FALSE;
                return TRUE;
            }

            if (DS2460_MEM_MAC <= code < DS2460_MEM_MAC + DS2460_MAC_LEN) {
                data[0] = COMPUTED_MAC[code - DS2460_MEM_MAC];
                return TRUE;
            }

            log_error("smx-exio", "Unknown read command: %02x", code);
            return FALSE;
        default:
            log_error("ds2460", "Unsupported read mode: %01x (%02x)", cmd, code);
            return FALSE;
    }
}
