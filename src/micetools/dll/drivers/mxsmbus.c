#include "../lib/dmi/dmi.h"
#include "../hooks/setupapi_.h"
#include "mx.h"
#include "smbus.h"

// PCA9535 (DIPSW)
#define PCA9535_WRITE 0x04
#define PCA9535_READ 0x05

#define PCA9535_IN0 0x00
#define PCA9535_IN1 0x01
#define PCA9535_OUT0 0x02
#define PCA9535_OUT1 0x03
#define PCA9535_INV0 0x04
#define PCA9535_INV1 0x05
#define PCA9535_CONF0 0x06
#define PCA9535_CONF1 0x07

#define SMBUS_PCA9535 0x20
#define SMBUS_EEPROM 0x57  // Doesn't line up with manual!

#define SMBUS_DDR2_DIMM_A1 0x000   // what does 0xA0 mean?
#define SMBUS_DDR2_DIMM_B1 0x010   // what does 0xA4 mean?
#define SMBUS_EEPROM_ 0x0AE        // = AT24C64AN
#define SMBUS_ICS9LPRS908 0xfff    // Unknown
#define SMBUS_W83627UHG 0xfff      // Unknown; hwmon. Possibly 0x2e or 0x4e
#define SMBUS_UPI_UP6261BM8 0xfff  // Unknown; vref
#define SMBUS_UPI_ISL6322CR 0xfff  // Unknown; vrm
// SMBUS is send onto the mezzanine board!

#define EEPROM_DUMP L"dev/eeprom.bin"
typedef struct eeprom_reg {
    BYTE data[32];
} eeprom_reg_t;
typedef struct eeprom_bank {
    eeprom_reg_t reg[0x100];
} eeprom_bank_t;

// 256 registers, 32 bytes each
eeprom_bank_t EEPROM_DATA;
/*
 * Known registers:
 *  - Reg 0x00: Stores ??? in [00]
 *  - Reg 0x08: Stores LPC address in [00, 01]
 *  - Reg 0x16: Stores ??? in [00, 01]
 *  - Reg 0x0e: Stores
 */

void eeprom_dump() {
    HANDLE dump = _CreateFileW(EEPROM_DUMP, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
    if (dump == INVALID_HANDLE_VALUE) {
        log_error("eeprom", "CreateFileA(EEPROM_DUMP) failed");
        return;
    }
    _WriteFile(dump, &EEPROM_DATA, sizeof EEPROM_DATA, NULL, NULL);
    FlushFileBuffers(dump);
    _CloseHandle(dump);
}
void eeprom_restore() {
    HANDLE dump =
        _CreateFileW(EEPROM_DUMP, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (dump == INVALID_HANDLE_VALUE) return;
    DWORD read;
    if (!_ReadFile(dump, &EEPROM_DATA, sizeof EEPROM_DATA, &read, NULL))
        log_error("eeprom", "failed to restore (%d)", GetLastError());
    _CloseHandle(dump);
}

DWORD eeprom_crc(BYTE reg) {
    if (reg == 0x04 || reg == 0x14 || reg == 0x80 || reg == 0x280) {
        // Some registers are only treated as 16 byte values
        crc32_build_table();
        return crc32(12, EEPROM_DATA.reg[reg].data + 4, 0);
    }

    crc32_build_table();
    return crc32(28, EEPROM_DATA.reg[reg].data + 4, 0);
}
void eeprom_read(BYTE reg, BYTE index, BYTE* data, BYTE length) {
    eeprom_restore();
    for (BYTE i = index; i < index + length; i++) {
        if (i > 0x1f) break;

        BYTE byte = EEPROM_DATA.reg[reg].data[i];

        // If register has a CRC
        // if (reg == 0x00 || reg == 0x01 || reg == 0x02 || reg == 0x10 || reg
        // == 0x11 || reg == 0x12 || reg == 0x200) {
        if (true) {
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
    for (BYTE i = index; i < index + length; i++) {
        if (i > 0x1f) break;
        EEPROM_DATA.reg[reg].data[i] = data[i - index];
    }
    eeprom_dump();
}

BYTE eeprom_read_one(BYTE reg, BYTE index) {
    BYTE data;
    eeprom_read(reg, index, &data, 1);
    return data;
}
void eeprom_write_one(BYTE reg, BYTE index, BYTE data) { eeprom_write(reg, index, &data, 1); }

BOOL mxsmbus_DeviceIoControl(void* file, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize,
                             LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned,
                             LPOVERLAPPED lpOverlapped) {
    mxsmbus_i2c_packet* i2c_packet = (mxsmbus_i2c_packet*)lpInBuffer;
    mxsmbus_i2c_packet* i2c_out = (mxsmbus_i2c_packet*)lpOutBuffer;

    mxsmbus_request_packet* request_packet = (mxsmbus_request_packet*)lpInBuffer;
    mxsmbus_request_packet* request_out = (mxsmbus_request_packet*)lpOutBuffer;

    BYTE dlen;

    switch (dwIoControlCode) {
        case IOCTL_MXSMBUS_GET_VERSION:
            log_misc("mxsmbus",
                     "DeviceIoControl(<mxsmbus>, <get version>, 0x%p, 0x%x, "
                     "-, 0x%x, -, -)",
                     lpInBuffer, nInBufferSize, nOutBufferSize);

            ((LPDWORD)lpOutBuffer)[0] = 0x01020001;
            if (lpBytesReturned) *lpBytesReturned = 4;
            break;
        case IOCTL_MXSMBUS_REQUEST:  // dip r/w
            log_misc("mxsmbus",
                     "DeviceIoControl(<mxsmbus>, <request>, 0x%p, 0x%x, -, "
                     "0x%x, -, -)",
                     lpInBuffer, nInBufferSize, nOutBufferSize);

            // Game trace:
            // 05 for 20
            // ^[x3]
            // 04 for 20
            // ^[x3]
            // 05 for 20
            // 03 for 57
            // ^[after every eeprom read/write]

            // Address 0x20 = PCA9535 = DIP switches
            // Address 0x30 = Keychip? "N2"
            // Address 0x55 = Keychip?
            // Address 0x57 = EEPROM

            // dyn = 0x57
            // 03: addr dyn: amEepromWait
            // 04: addr dyn: amHmI2CWriteByte (via IOCTL_MXSMBUS_I2C instead)
            // 05: addr dyn: amHmI2CWriteByte (via IOCTL_MXSMBUS_I2C instead)
            //
            // dyn2 = 0x20
            // 04: addr dyn2: amDipswWriteByteInternal
            // 05: addr dyn2: amDipswReadByteInternal[Ex]

            // 0B: addr 0x20: amHmGetLPCChipId
            // 0B: addr 0x21: amHmGetLPCChipId
            // 48: addr 0x00: amHmGetLPCChipId

            switch (i2c_packet->addr) {
                case SMBUS_PCA9535:
                    switch (i2c_packet->prt) {
                        case PCA9535_WRITE:
                            switch (i2c_packet->reg) {
                                case PCA9535_OUT1:
                                    log_info("mxsmbus", "pca9535 OUT1: %02x %02x", i2c_packet->data[0],
                                             i2c_packet->data[1]);
                                    break;
                                case PCA9535_INV0:
                                    log_info("mxsmbus", "pca9535 INV0: %02x %02x", i2c_packet->data[0],
                                             i2c_packet->data[1]);
                                    break;
                                case PCA9535_INV1:
                                    log_info("mxsmbus", "pca9535 INV1: %02x %02x", i2c_packet->data[0],
                                             i2c_packet->data[1]);
                                    break;
                                default:
                                    log_error("mxsmbux",
                                              "(write) Undefined pca9535 "
                                              "register: 0x%02x",
                                              i2c_packet->reg);
                                    exit(1);
                            }
                            break;
                        case PCA9535_READ:
                            switch (i2c_packet->reg) {
                                case PCA9535_IN0:  // DIPSW
                                    i2c_packet->data[0] = 0x00;
                                    break;
                                case PCA9535_IN1:  // SW1/2 + extras
                                                   /*
                                                       0: uk
                                                       1: uk
                                                       2: ¬test
                                                       3: ¬service
                                                       4: uk
                                                       5: uk
                                                       6: uk
                                                       7: uk
                                                   */
                                    i2c_packet->data[0] = 0x0c;
                                    break;
                                case PCA9535_INV0:
                                case PCA9535_INV1:
                                case PCA9535_OUT1:  // LEDs, probably
                                    i2c_packet->data[0] = 0x00;
                                    break;
                                default:
                                    log_error("mxsmbux",
                                              "(read) Undefined pca9535 "
                                              "register: 0x%02x",
                                              i2c_packet->reg);
                                    exit(1);
                            }
                            i2c_packet->status = 0x00;
                            break;
                        default:
                            log_error("mxsmbux", "Unknown pca9535 command: 0x%02x", i2c_packet->prt);
                            exit(1);
                    }
                    break;
                case SMBUS_EEPROM:
                    switch (i2c_packet->prt) {
                        case 3:  // Wait
                            // 0x18 = wait, 0x00 = done
                            i2c_packet->status = 0;
                            break;
                        default:
                            log_error("mxsmbux", "Unknown eeprom command: 0x%02x", i2c_packet->prt);
                            exit(1);
                    }
                    break;
                default:
                    log_error("mxsmbus", "Unknown smbus device: 0x%02x", i2c_packet->addr);
                    exit(1);
            }

            i2c_out->status = MXSBUS_OKAY;
            if (lpBytesReturned) *lpBytesReturned = sizeof(mxsmbus_i2c_packet);
            break;
        case IOCTL_MXSMBUS_I2C:  // i2c r/w
            log_misc("mxsmbus",
                     "DeviceIoControl(<mxsmbus>, <i2c>, 0x%p, 0x%x, -, 0x%x, "
                     "-, -)",
                     lpInBuffer, nInBufferSize, nOutBufferSize);

            log_misc("mxsmbus", "SMBus I2C request for %02x: 0x%02x (%02x bytes @ %02x)", request_packet->addr,
                     request_packet->prt, request_packet->dlen, request_packet->reg);
            // log_warning("eeprom", "%08x %08x %08x %08x", dwordInBuffer[0],
            // dwordInBuffer[1], dwordInBuffer[2], dwordInBuffer[3]);
            // log_warning("eeprom", "%08x %08x %08x %08x", dwordInBuffer[4],
            // dwordInBuffer[5], dwordInBuffer[6], dwordInBuffer[7]);
            // log_warning("eeprom", "%08x %08x", dwordInBuffer[8],
            // dwordInBuffer[9]);

            // for (int i = 0; i < nInBufferSize; i++) {
            //	printf("%02x ", ((LPBYTE)lpInBuffer)[i]);
            // }
            // puts("");

            // prt = byteInBuffer[1];
            // addr = wordInBuffer[1];
            // reg = wordInBuffer[2] & 0xFF;
            dlen = request_packet->dlen;
            if (dlen > 0x20) dlen = 0x20;

            /*
             * Known addresses:
             *  - 0x57: EEPROM
             **/

            request_packet->status = MXSBUS_OKAY;

            if (request_packet->status != 0) {
                log_error("mxsmbus", "invalid i2c packet");
                return FALSE;
            }

            if (request_packet->addr != SMBUS_EEPROM) {
                log_error("mxsmbus", "Unexpected I2C device: 0x%02x", request_packet->addr);
                exit(1);
            }
            // 04 = Write byte
            // 05 = Read byte
            //
            // 08 = Write block
            // 09 = Read block

            if (request_packet->prt == 0x08) {
                // Write
                log_misc("mxsmbus", "eeprom write %02x (0x%04x)", dlen, request_packet->reg);
                // for (int i = 0; i < dlen; i++) printf("%02x ",
                // request_packet->data[i]); puts("");

                eeprom_write(request_packet->reg >> 5, request_packet->reg & 0x1f, request_packet->data,
                             request_packet->dlen);
                request_out->status = 0;
            } else if (i2c_packet->prt == 0x09) {
                // Read
                log_misc("mxsmbus", "eeprom read %02x (0x%04x)", dlen, request_packet->reg);
                eeprom_read(request_packet->reg >> 5, request_packet->reg & 0x1f, request_out->data, dlen);
                // for (int i = 0; i < dlen; i++) printf("%02x ",
                // request_out->data[i]); puts("");
                request_out->status = 0;
            } else {
                log_warning("mxsmbus", "UNHANDLED MXSMBUS I2C %02x", request_packet->prt);
                exit(1);
            }

            if (lpBytesReturned) *lpBytesReturned = sizeof(mxsmbus_request_packet);
            break;
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
