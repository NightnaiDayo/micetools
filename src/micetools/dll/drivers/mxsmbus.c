#include "../hooks/setupapi_.h"
#include "../lib/dmi/dmi.h"
#include "mx.h"
#include "smbus.h"

#define EEPROM_DUMP L"dev/eeprom.bin"
typedef struct eeprom_reg {
    BYTE data[32];
} eeprom_reg_t;
typedef struct eeprom_bank {
    eeprom_reg_t reg[0x100];
} eeprom_bank_t;

// 256 registers, 32 bytes each
eeprom_bank_t EEPROM_DATA;

void eeprom_dump() {
    HANDLE dump =
        _CreateFileW(EEPROM_DUMP, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
    if (dump == INVALID_HANDLE_VALUE) {
        log_error("eeprom", "CreateFileA(EEPROM_DUMP) failed");
        return;
    } else {
        log_info("eeprom", "Wrote eeprom to %ls", EEPROM_DUMP);
    }
    _WriteFile(dump, &EEPROM_DATA, sizeof EEPROM_DATA, NULL, NULL);
    FlushFileBuffers(dump);
    _CloseHandle(dump);
}
void eeprom_restore() {
    HANDLE dump = _CreateFileW(EEPROM_DUMP, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, NULL);
    if (dump == INVALID_HANDLE_VALUE) {
        // Make the file, even though it'll probably be empty
        eeprom_dump();
        return;
    }
    DWORD read;
    if (!_ReadFile(dump, &EEPROM_DATA, sizeof EEPROM_DATA, &read, NULL))
        log_error("eeprom", "failed to restore (%d)", GetLastError());
    _CloseHandle(dump);
}

DWORD eeprom_crc(BYTE reg) {
    if (reg == 0x04 || reg == 0x14 || reg == 0x80 || reg == 0x280) {
        // Some registers are only treated as 16 byte values
        return crc32(12, EEPROM_DATA.reg[reg].data + 4, 0);
    }

    return crc32(28, EEPROM_DATA.reg[reg].data + 4, 0);
}

#pragma pack(1)
typedef struct {
    uint32_t checksum;
    uint8_t pad1[8];
    uint8_t region;
    uint8_t pad2[2];
    char mainId[17];
} eeprom_block_1;
#pragma pack(1)
typedef struct {
    uint32_t checksum;
    uint32_t unk;
    uint32_t dhcp_enabled;
    uint32_t machine_ip;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t primary_dns;
    uint32_t secondary_dns;
} eeprom_block_3;
#pragma pack(1)
typedef struct {
    uint32_t checksum;
    byte pad[4];
    char game_id[4];
    uint32_t unk;
} eeprom_block_5_1;
#pragma pack(1)
typedef struct {
    uint32_t checksum;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t unk3;
} eeprom_block_5_2;

#define SET_IP(val, a, b, c, d)                                  \
    do {                                                         \
        *(uint32_t*)&val = (a << 24) | (b << 16) | (c << 8) | d; \
    } while (0)

void eeprom_read(BYTE reg, BYTE index, BYTE* data, BYTE length) {
    eeprom_restore();
    for (BYTE i = index; i < index + length; i++) {
        if (i > 0x1f) break;

        BYTE byte = EEPROM_DATA.reg[reg].data[i];

        // TODO: Reg 1 and 17 in the EEPROM are system info. We should fake these!

        if (reg == 0x00 || reg == 0x10) {
            eeprom_block_1 block;
            memset(&block, 0, sizeof block);

            block.region = 0b111;
            strcpy(block.mainId, "AASE-01A65646203");

            block.checksum = crc32(28, (BYTE*)(&block) + 4, 0);

            byte = ((BYTE*)(&block))[i];
        }
        if (reg == 0x02) {
            eeprom_block_3 block;
            memset(&block, 0, sizeof block);

            // TODO: Load network config!
            SET_IP(block.machine_ip, 192, 168, 103, 101);
            SET_IP(block.netmask, 255, 255, 255, 0);
            SET_IP(block.gateway, 192, 168, 103, 254);
            SET_IP(block.primary_dns, 192, 168, 103, 254);
            SET_IP(block.secondary_dns, 0, 0, 0, 0);
            block.dhcp_enabled = 0;

            block.unk = 4835744;

            block.checksum = crc32(28, (BYTE*)(&block) + 4, 0);

            byte = ((BYTE*)(&block))[i];
        }
        if (reg == 0x04) {
            eeprom_block_5_1 block1;
            eeprom_block_5_2 block2;

            memset(&block1, 0, sizeof block1);
            memset(&block2, 0, sizeof block2);
            block1.game_id[0] = 'S';
            block1.game_id[1] = 'D';
            block1.game_id[2] = 'E';
            block1.game_id[3] = 'Y';

            block2.unk2 = 0xffffffff;
            block2.unk3 = 0xffffffff;

            block1.checksum = crc32(12, (BYTE*)(&block1) + 4, 0);
            block2.checksum = crc32(12, (BYTE*)(&block2) + 4, 0);

            if (i < 16)
                byte = ((BYTE*)(&block1))[i];
            else
                byte = ((BYTE*)(&block2))[i - 16];
        }

        // If register has a CRC
        // if (reg == 0x00 || reg == 0x01 || reg == 0x02 || reg == 0x10 || reg
        // == 0x11 || reg == 0x12 || reg == 0x200) {
        if (false) {
            // Intercept the read and inject a CRC instead
            if (i == 0x00 || i == 0x01 || i == 0x02 || i == 0x03) {
                DWORD crc = eeprom_crc(reg);
                byte = crc >> 8 * i & 0xff;
            }
        }

        data[i - index] = byte;
    }
}
void eeprom_write(BYTE reg, BYTE index, BYTE* data, BYTE length) {
    log_misc("eeprom", "write reg=%d idx=%d len=%d", reg, index, length);

    for (BYTE i = index; i < index + length; i++) {
        if (i > 0x1f) break;
        EEPROM_DATA.reg[reg].data[i] = data[i - index];
    }
    eeprom_dump();
}

typedef enum {
    SMB_CMD_QUICK = 0b000,
    SMB_CMD_BYTE = 0b001,
    SMB_CMD_BYTE_DATA = 0b010,
    SMB_CMD_WORD_DATA = 0b011,
    SMB_CMD_PROCESS_CALL = 0b100,
    SMB_CMD_BLOCK = 0b101,
    SMB_CMD_I2C_READ = 0b110,
    SMB_CMD_BLOCK_PROCESS = 0b111,
} smb_cmd_t;

// EEPROM
BOOL smbus_AT24C64AN(BYTE addr, smb_cmd_t cmd, BYTE code, BYTE dlen, BYTE* data) {
    BOOL read = addr & 1 == 1;
    if (read) {
        switch (cmd) {
            case SMB_CMD_BYTE:
                data[0] = 0x00;
                break;
            case SMB_CMD_BLOCK: {
                WORD reg = *(WORD*)(&data[-1]);
                dlen = data[1];
                log_info("eeprom", "Block read, %d @ %02x", dlen, reg);

                eeprom_read(reg >> 5, reg & 0x1f, &data[2], dlen);
                return TRUE;
            }
            default:
                log_error("eeprom", "Unsupported read mode: %01x, %02x", cmd, code);
                return FALSE;
        }
        return TRUE;
    }

    switch (cmd) {
        case SMB_CMD_BLOCK: {
            WORD reg = *(WORD*)(&data[-1]);
            dlen = data[1];
            log_info("eeprom", "Block write, %d @ %02x", dlen, reg);

            eeprom_write(reg >> 5, reg & 0x1f, &data[2], dlen);
            return TRUE;
        }
        default:
            log_error("eeprom", "Unsupported write mode: %01x, %02x", cmd, code);
            return FALSE;
    }
}

// dipsw
BOOL smbus_PCA9535(BYTE addr, smb_cmd_t cmd, BYTE code, BYTE dlen, BYTE* data) {
    static uint16_t pca9535_config = 0xffff;

    BOOL read = addr & 1 == 1;
    if (read) {
        switch (cmd) {
            case SMB_CMD_BYTE_DATA:
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
                        data[0] = 0b00001000;
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

    switch (cmd) {
        case SMB_CMD_BYTE_DATA:
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

// Very incomplete keychip
BOOL smbus_N2(BYTE addr, smb_cmd_t cmd, BYTE code, BYTE dlen, BYTE* data) {
    static unsigned char challenge[7];
    static unsigned char n2_eeprom[3][0x20];

    BOOL read = addr & 1 == 1;
    if (read) {
        switch (cmd) {
            case SMB_CMD_BYTE_DATA:
                if (code < 0x80) {
                    BYTE page = (code >> 5) & 0b11;
                    BYTE offset = code & 0x1f;

                    data[0] = n2_eeprom[page][offset];

                    return TRUE;
                } else if (code >= N2_GET_UNIQUE_NUMBER && code < N2_GET_UNIQUE_NUMBER + 0xf) {
                    data[0] = 0x04;  // chosen by fair dice roll.
                    return TRUE;
                } else if (code == N2_GET_STATUS) {
                    // Polled until N2_STATUS_FLAG_BUSY low
                    data[0] = 0x00;
                    return TRUE;
                } else {
                    log_error("smb-keychip", "Unknown read command: %02x", code);
                    return FALSE;
                }
            default:
                log_error("smb-keychip", "Unsupported read mode: %01x (%02x)", cmd, code);
                return FALSE;
        }
    }

    switch (cmd) {
        case SMB_CMD_BLOCK: {
            WORD reg = *(WORD*)(&data[-1]);
            dlen = data[1];

            if (dlen != 7) {
                log_error("smb-keychip", "Expected challenge length of 7 (saw %d)!", dlen);
                return FALSE;
            }

            memcpy(challenge, &(data[2]), 7);

            char* challenge_s = malloc(dlen * 3 + 1);
            for (int i = 0; i < dlen; i++) {
                sprintf(challenge_s + i * 3, "%02x ", data[2 + i]);
            }
            challenge_s[dlen * 3 + 1] = '\0';
            log_info("smb-keychip", "Challenge: %s", challenge_s);
            free(challenge_s);
            return TRUE;
        }
        case SMB_CMD_I2C_READ: {
            switch (code) {
                case N2_I2C_CHALLENGE_RESPONSE:
                    // This just has to match EXIO!
                    for (int i = 0; i < dlen; i++) data[i] = 0x69;
                    return TRUE;
                default:
                    log_error("smb-keychip", "Unknown I2C command: %02x", code);
            }
        }
        default:
            log_error("smb-keychip", "Unsupported write mode: %01x (%02x)", cmd, code);
            return FALSE;
    }
}
BOOL smbus_EXIO(BYTE addr, smb_cmd_t cmd, BYTE code, BYTE dlen, BYTE* data) {
    BOOL read = addr & 1 == 1;
    if (read) {
        switch (cmd) {
            case SMB_CMD_BYTE_DATA:
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

    switch (cmd) {
        case SMB_CMD_BYTE_DATA:
            switch (code) {
                case 0x5c:
                    if (data[0] == 0x94) return TRUE;
                default:
                    log_error("smb-exio", "Unknown write command: %02x", code);
                    return FALSE;
            }
        case SMB_CMD_BLOCK: {
            WORD reg = *(WORD*)(&data[-1]);
            dlen = data[1];

            char* data_s = malloc(dlen * 3 + 1);
            for (int i = 0; i < dlen; i++) {
                sprintf(data_s + i * 3, "%02x ", data[2 + i]);
            }
            data_s[dlen * 3 + 1] = '\0';

            log_info("smb-exio", "Block write, %d @ %04x: %s", dlen, reg, data_s);
            free(data_s);
            return TRUE;
        }

        default:
            log_error("smb-exio", "Unsupported write mode: %01x (%02x)", cmd, code);
            return FALSE;
    }
}

BOOL handle_smbus(BYTE* request) {
    BYTE command = request[1];
    BYTE v_addr = request[2] & 0x7f;
    BYTE command_code = request[3];

    BYTE p_addr = request[2] << 1;
    smb_cmd_t smb_cmd = SMB_CMD_QUICK;
    switch (command) {
        case 0:
            break;
        case 1:
            p_addr++;
            break;
        case 2:
            smb_cmd = SMB_CMD_BYTE;
            break;
        case 3:
            p_addr++;
            smb_cmd = SMB_CMD_BYTE;
            break;
        case 4:
            smb_cmd = SMB_CMD_BYTE_DATA;
            break;
        case 5:
            p_addr++;
            smb_cmd = SMB_CMD_BYTE_DATA;
            break;
        case 6:
            smb_cmd = SMB_CMD_WORD_DATA;
            break;
        case 7:
            p_addr++;
            smb_cmd = SMB_CMD_WORD_DATA;
            break;
        case 8:
            smb_cmd = SMB_CMD_BLOCK;
            break;
        case 9:
            p_addr++;
            smb_cmd = SMB_CMD_BLOCK;
            break;
        case 10:
            smb_cmd = SMB_CMD_PROCESS_CALL;
            break;
        case 11:
            smb_cmd = SMB_CMD_I2C_READ;
            break;
    }

    log_trace("smbus", "Making request to %02X (virtual: %02X/%02x, cmd: %01X, code: %02X)", p_addr,
              v_addr, command, smb_cmd, command_code);

    switch (p_addr) {
        case SMBUS_EEPROM:
        case SMBUS_EEPROM + 1:
            return smbus_AT24C64AN(p_addr, smb_cmd, command_code, request[4], &request[5]);
        case SMBUS_PCA9535:
        case SMBUS_PCA9535 + 1:
            return smbus_PCA9535(p_addr, smb_cmd, command_code, request[4], &request[5]);
        case SMBUS_N2:
        case SMBUS_N2 + 1:
            return smbus_N2(p_addr, smb_cmd, command_code, request[4], &request[5]);
        case SMBUS_EXIO:
        case SMBUS_EXIO + 1:
            return smbus_EXIO(p_addr, smb_cmd, command_code, request[4], &request[5]);
        default:
            log_error("smbus", "Request to unregistered address: %02x", p_addr);
            return FALSE;
    }
}

BOOL mxsmbus_DeviceIoControl(void* file, DWORD dwIoControlCode, LPVOID lpInBuffer,
                             DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                             LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    mxsmbus_i2c_packet* i2c_packet = (mxsmbus_i2c_packet*)lpInBuffer;
    mxsmbus_i2c_packet* i2c_out = (mxsmbus_i2c_packet*)lpOutBuffer;

    // Default value
    if (lpBytesReturned) *lpBytesReturned = nOutBufferSize;

    switch (dwIoControlCode) {
        case IOCTL_MXSMBUS_GET_VERSION:
            log_misc("mxsmbus",
                     "DeviceIoControl(<mxsmbus>, <get version>, 0x%p, 0x%x, "
                     "-, 0x%x, -, -)",
                     lpInBuffer, nInBufferSize, nOutBufferSize);

            ((LPDWORD)lpOutBuffer)[0] = 0x01020001;
            if (lpBytesReturned) *lpBytesReturned = 4;
            break;
        case IOCTL_MXSMBUS_I2C: {
            BYTE command = ((BYTE*)lpInBuffer)[1];
            if (command > 10) {
                ((BYTE*)lpOutBuffer)[0] = 0x19;
                return FALSE;
            }
            if (handle_smbus(lpInBuffer)) {
                ((BYTE*)lpOutBuffer)[0] = 0x00;
                return TRUE;
            } else {
                ((BYTE*)lpOutBuffer)[0] = 0x01;
                return FALSE;
            };
        }
        case IOCTL_MXSMBUS_REQUEST: {
            BYTE command = ((BYTE*)lpInBuffer)[1];
            if (command > 11) {
                ((BYTE*)lpOutBuffer)[0] = 0x19;
                return FALSE;
            }
            if (handle_smbus(lpInBuffer)) {
                ((BYTE*)lpOutBuffer)[0] = 0x00;
                return TRUE;
            } else {
                ((BYTE*)lpOutBuffer)[0] = 0x01;
                return FALSE;
            };
        }
        default:
            log_warning("mxsmbus", "unhandled 0x%08x", dwIoControlCode);
            return FALSE;
    }
    return TRUE;
}

void setup_mxsmbus() {
    file_hook_t* mxsmbus = new_file_hook(L"\\\\.\\mxsmbus");
    mxsmbus->DeviceIoControl = &mxsmbus_DeviceIoControl;

    hook_file(mxsmbus);

    if (!add_fake_device(&MXSMBUS_GUID, L"\\\\.\\mxsmbus")) {
        log_error("mxsmbus", "failed to install mxsmbus device");
    }
}
