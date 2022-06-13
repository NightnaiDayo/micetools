#include <Windows.h>

#include "../lib/dmi/dmi.h"
#include "../lib/mice/mice.h"
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

#define PCA9535 0x20
#define EEPROM 0x57

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

typedef uint_fast32_t crc_t;

static const crc_t crc_table[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832,
    0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856, 0x646ba8c0, 0xfd62f97a,
    0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3,
    0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab,
    0xb6662d3d, 0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01, 0x6b6b51f4,
    0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074,
    0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525,
    0x206f85b3, 0xb966d409, 0xce61e49f, 0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615,
    0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7, 0xfed41b76,
    0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c, 0x36034af6,
    0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7,
    0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7,
    0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278,
    0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9, 0xbdbdf21c, 0xcabac28a, 0x53b39330,
    0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d};
#define CRC_ALGO_TABLE_DRIVEN 1
static inline crc_t crc_init(void) { return 0xffffffff; }
crc_t crc_update(crc_t crc, const void* data, size_t data_len) {
    const unsigned char* d = (const unsigned char*)data;
    unsigned int tbl_idx;

    while (data_len--) {
        tbl_idx = (crc ^ *d) & 0xff;
        crc = (crc_table[tbl_idx] ^ (crc >> 8)) & 0xffffffff;
        d++;
    }
    return crc & 0xffffffff;
}
static inline crc_t crc_finalize(crc_t crc) { return crc ^ 0xffffffff; }

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
    if (reg == 0x80 || reg == 0x280) {
        // Some registers are only treated as 16 byte values
        return crc_finalize(crc_update(crc_init(), EEPROM_DATA.reg[reg].data + 4, 12));
    }
    return crc_finalize(crc_update(crc_init(), EEPROM_DATA.reg[reg].data + 4, 28));
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

BOOL mxsmbus_DeviceIoControl(DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer,
                             DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
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
                case PCA9535:
                    switch (i2c_packet->prt) {
                        case PCA9535_WRITE:
                            switch (i2c_packet->reg) {
                                case PCA9535_OUT1:
                                    log_error("mxsmbus", "pca9535 OUT1: %02x %02x", i2c_packet->data[0],
                                              i2c_packet->data[1]);
                                    break;
                                case PCA9535_INV0:
                                    log_error("mxsmbus", "pca9535 INV0: %02x %02x", i2c_packet->data[0],
                                              i2c_packet->data[1]);
                                    break;
                                case PCA9535_INV1:
                                    log_error("mxsmbus", "pca9535 INV1: %02x %02x", i2c_packet->data[0],
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
                                case PCA9535_IN1:  // What's this one?
                                    i2c_packet->data[0] = 0x00;
                                    break;
                                case PCA9535_INV0:
                                case PCA9535_INV1:
                                case PCA9535_OUT1:
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
                case EEPROM:
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

            if (request_packet->addr != EEPROM) {
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
