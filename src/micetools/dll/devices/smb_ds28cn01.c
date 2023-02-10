/**
 * This is one of three uniquely accesible chips on the keychip, and one
 * of two on the SMBus.
 *
 * The DS28CN01 is a 1Kbit EEPROM with hardware SHA-1 and a factory-
 * unique random number.
 *
 * The pins are wired as:
 *
 * pin | name | connection
 * -----------------------
 *  1  | AD0  | VCC3
 *  2  | AD1  | VCC3
 *  3  | NC   |
 *  4  | GND  | GND
 *  5  | SDA  | SMData
 *  6  | SCL  | SMClock
 *  7  | NC   |
 *  8  | Vcc  | VCC3
 */

#include "smb_ds28cn01.h"

#include <openssl/evp.h>

#include "_ds_sha.h"

// Keychip secrets!
static BYTE USER_EEPROM[DS28CN01_EEPROM_PAGES][DS28CN01_EEPROM_PAGE_SIZE] = {
    {
        0x09, 0xb7, 0xcc, 0xe7, 0x74, 0x5f, 0x5c, 0xd9, 0xf2, 0x29, 0x70,
        0x86, 0x70, 0x61, 0xf4, 0xba, 0xdb, 0x4f, 0xde, 0x1e, 0x85, 0x0b,
        0xb6, 0xaf, 0xcb, 0x93, 0x55, 0x6e, 0xf5, 0x1b, 0xed, 0x59,
    },
    {
        0x5d, 0xa4, 0x49, 0x9b, 0xd4, 0xd0, 0x56, 0x00, 0x41, 0xf5, 0xe3,
        0xab, 0x8b, 0x70, 0xa5, 0x0b, 0xb9, 0x3c, 0x7e, 0x0b, 0x58, 0xc3,
        0xfe, 0x89, 0x1d, 0xf3, 0x2b, 0x0a, 0xcc, 0x95, 0x7d, 0xac,
    },
    {
        0x58, 0xda, 0x2d, 0x5c, 0x4c, 0x37, 0xb0, 0x48, 0x69, 0x73, 0x7e,
        0x97, 0x91, 0xff, 0x17, 0x58, 0xe4, 0x85, 0x48, 0xb9, 0x28, 0xc3,
        0xaa, 0x7a, 0x6e, 0x9a, 0x6d, 0xa1, 0xf2, 0x62, 0x53, 0x6e,
    },
    {
        0x44, 0x81, 0xff, 0x91, 0xa5, 0x3c, 0x70, 0x9b, 0xb4, 0xcd, 0xc9,
        0xb3, 0x4a, 0xb5, 0x53, 0x8a, 0xa2, 0x53, 0xc4, 0x91, 0xbd, 0xb6,
        0xc2, 0x69, 0x04, 0xb0, 0xa3, 0x9a, 0x30, 0x2b, 0x77, 0x89,
    },
};

// TODO: Make this actually a valid ID (crc and whatnot)
// Unique per keychip
static const BYTE UNIQUE_NUMBER[DS28CN01_UNIQUE_NUMBER_SIZE] = {
    'M', 'I', 'C', 'E', 'K', 'E', 'Y', '\0',
};

#define DS28CN01_INPUT_BUFFER_SIZE 64
static BYTE MAC_INPUT_BUFFER[DS28CN01_INPUT_BUFFER_SIZE];
static BYTE COMPUTED_MAC[DS28CN01_MAC_LEN];

static BYTE SECRET[8] = { 0 };

void ds28cn01_update_mac(void) {
    puts("ds28cn01: SHA1 input buffer:");
    for (int i = 0; i < DS28CN01_INPUT_BUFFER_SIZE; i++) {
        printf("%02x", MAC_INPUT_BUFFER[i]);
    }
    puts("");

    // EVP_MD_CTX* ctx = EVP_MD_CTX_create();
    // EVP_DigestInit(ctx, EVP_sha1());
    // EVP_DigestUpdate(ctx, MAC_INPUT_BUFFER, DS28CN01_INPUT_BUFFER_SIZE);
    // unsigned int outlen;
    // EVP_DigestFinal_ex(ctx, COMPUTED_MAC, &outlen);
    // EVP_MD_CTX_destroy(ctx);

    ComputeDallasSha(MAC_INPUT_BUFFER, (long*)&COMPUTED_MAC[0], (long*)&COMPUTED_MAC[4],
                     (long*)&COMPUTED_MAC[8], (long*)&COMPUTED_MAC[12],
                     (long*)&COMPUTED_MAC[16]);

    puts("ds28cn01: SHA1 out buffer:");
    for (int i = 0; i < 20; i++) {
        printf("%02x", COMPUTED_MAC[i]);
    }
    puts("");
}

BOOL smbus_ds28cn01_write(ich9_cmd_t cmd, WORD code, BYTE dlen, BYTE* data) {
    static unsigned char challenge[8];

    switch (cmd) {
        case ICH9_CMD_BLOCK: {
            if (code >> 8 == 0xa9) {
                BYTE command = code & 0xff;

                BYTE page = command & 3;
                BYTE upper = command & 0xf0;

                if (upper == 0xD0) {
                    log_warning("ds28cn01", "Computing for: authentication (flag = 1)");
                } else if (upper == 0xE0) {
                    log_warning("ds28cn01", "Computing for: ds.compute (flag = 0)");
                } else {
                    log_error("ds28cn01", "Unknown A9");
                    return FALSE;
                }

                if (dlen != 7) {
                    log_error("ds28cn01", "Expected challenge length of 7 (saw %d)!", dlen);
                    return FALSE;
                }

                memcpy(&MAC_INPUT_BUFFER[0], &SECRET[0], 4);
                memcpy(&MAC_INPUT_BUFFER[4], USER_EEPROM[page], DS28CN01_EEPROM_PAGE_SIZE);
                memcpy(&MAC_INPUT_BUFFER[36], &data[0], 4);
                MAC_INPUT_BUFFER[40] = 0x40 | page;
                memcpy(&MAC_INPUT_BUFFER[41], UNIQUE_NUMBER, 7);
                memcpy(&MAC_INPUT_BUFFER[48], &SECRET[4], 4);
                memcpy(&MAC_INPUT_BUFFER[52], &data[4], 3);
                MAC_INPUT_BUFFER[55] = 0x80;
                for (int i = 56; i < 62; i++) MAC_INPUT_BUFFER[i] = 0;
                MAC_INPUT_BUFFER[62] = 0x01;
                MAC_INPUT_BUFFER[63] = 0xB8;

                //

                // Stuffing into the command code, as ever
                // challenge[0] = command & 0xff;
                // memcpy(&(challenge[1]), data, dlen);

                // dlen++;
                // char* challenge_s = malloc(dlen * 3 + 1);
                // if (challenge_s == NULL) {
                //     log_info("ds28cn01", "Challenge: (buffer failed)");
                //     return TRUE;
                // }

                // for (int i = 0; i < dlen; i++) {
                //     sprintf_s(challenge_s + i * 3, 4, "%02x ", data[i]);
                // }
                // challenge_s[dlen * 3 + 1] = '\0';
                // log_info("ds28cn01", "Challenge: %s", challenge_s);
                // free(challenge_s);

                return TRUE;
            }

            log_error("ds28cn01", "Unknown write command: %04x", code);
        }
        case ICH9_CMD_I2C_READ: {
            switch (code) {
                case DS28CN01_REG_MAC:
                    // This just has to match EXIO!
                    if (dlen > DS28CN01_MAC_LEN) return FALSE;

                    ds28cn01_update_mac();

                    for (int i = 0; i < dlen; i++) data[i] = COMPUTED_MAC[i];
                    return TRUE;
                default:
                    log_error("ds28cn01", "Unknown I2C read command: %04x", code);
            }
        }
        default:
            log_error("ds28cn01", "Unsupported write mode: %01x (%04x)", cmd, code);
            return FALSE;
    }
}

BOOL smbus_ds28cn01_read(ich9_cmd_t cmd, WORD code, BYTE dlen, BYTE* data) {
    switch (cmd) {
        case ICH9_CMD_BYTE_DATA:
            if (code < DS28CN01_EEPROM_SIZE) {
                BYTE page = (code >> 5) & 0b11;
                BYTE offset = code & 0x1f;

                data[0] = USER_EEPROM[page][offset];

                return TRUE;
            }

            if (code >= DS28CN01_REG_NUM && code <= DS28CN01_REG_NUM_END) {
                data[0] = UNIQUE_NUMBER[code - DS28CN01_REG_NUM];
                return TRUE;
            }

            if (code == DS28CN01_REG_STATUS) {
                // Polled until DS_STATUS_FLAG_BUSY low
                data[0] = 0x00;
                return TRUE;
            }

            log_error("ds28cn01", "Unknown read command: %04x", code);
            return FALSE;
        default:
            log_error("ds28cn01", "Unsupported read mode: %01x (%04x)", cmd, code);
            return FALSE;
    }
}
