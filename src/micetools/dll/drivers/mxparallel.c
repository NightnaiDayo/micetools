#include "mxparallel.h"

BYTE parallel_flags = 0x00;
BYTE parallel_ctrl = 0x00;
BYTE parallel_status = 0x00;
BYTE parallel_data = 0x00;

BYTE KEYCHIP_ID[16] = { 'A', '7', '2', 'E', '-', '0', '2', 'D',
                        '1', '1', '2', '6', '1', '1', '1', '6' };
BYTE MAIN_ID[16] = { 'A', 'A', 'S', 'E', '-', '0', '1', 'A', '6', '5', '6', '4' };

appboot_t APPBOOT = {
    .format = 1,
    .game_id = { 'S', 'D', 'E', 'Y' },
    .region = 8 | 4 | 2 | 1,
    .model_type = 2,
    // Bitfield
    // 1 = use flash for appboot
    .system_flag = 0x24,
    ._ = 0,
    .platform_id = { 'A', 'A', 'S' },
    .dvd_flag = 1,
    .network_addr = (192 << 0) | (168 << 8) | (103 << 16) | (0 << 24),
    .__ = 0,
    .seed = 1,
};
billing_t BILLING = {
    .playlimit = 21046,
    .nearfull = 512,
};
uint32_t BILLING_PLAYCOUNT = 69420;

#define overlappedComplete(len)               \
    do {                                      \
        if (lpOverlapped) {                   \
            SetLastError(ERROR_SUCCESS);      \
            lpOverlapped->Internal = 0;       \
            lpOverlapped->InternalHigh = len; \
            SetEvent(lpOverlapped->hEvent);   \
        }                                     \
    } while (0)
#define outLen(len)                                  \
    do {                                             \
        if (lpBytesReturned) *lpBytesReturned = len; \
        overlappedComplete(len);                     \
    } while (0)

BOOL mxparallel_DeviceIoControl(void* file, DWORD dwIoControlCode, LPVOID lpInBuffer,
                                DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                                LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    log_trace("mxparallel", "DeviceIoControl(<mxparallel>, 0x%08x, 0x%p, 0x%x, -, 0x%x, -, -)",
              dwIoControlCode, lpInBuffer, nInBufferSize, nOutBufferSize);

    switch (dwIoControlCode) {
        case IOCTL_MXPARALLEL_READ_FLAGS:
            ((LPBYTE)lpOutBuffer)[0] = parallel_flags;
            outLen(1);
            return TRUE;
        case IOCTL_MXPARALLEL_WRITE_FLAGS:
            parallel_flags = ((LPBYTE)lpInBuffer)[0];
            outLen(0);
            return TRUE;

        case IOCTL_MXPARALLEL_READ_CTRL_PORT:
            ((LPBYTE)lpOutBuffer)[0] = parallel_ctrl;
            outLen(1);
            return TRUE;
        case IOCTL_MXPARALLEL_WRITE_CTRL_PORT:
            parallel_ctrl = ((LPBYTE)lpInBuffer)[0];
            // log_warning("mxparallel", "Write ctrl %08x", parallel_ctrl);
            outLen(0);
            return TRUE;

        case IOCTL_MXPARALLEL_READ_STATUS:
            ((LPBYTE)lpOutBuffer)[0] = parallel_status;
            outLen(1);
            return TRUE;
        case IOCTL_MXPARALLEL_WRITE_STATUS:
            parallel_status = ((LPBYTE)lpInBuffer)[0];
            outLen(0);
            return TRUE;

        case IOCTL_MXPARALLEL_READ_DATA:
            ((LPBYTE)lpOutBuffer)[0] = parallel_data;
            outLen(1);
            return TRUE;
        case IOCTL_MXPARALLEL_WRITE_DATA:
            parallel_data = ((LPBYTE)lpInBuffer)[0];
            outLen(0);
            return TRUE;
    }
    return FALSE;
}

// * Our implementation needs to use the opposite keys to mxkeychip

void micexkTransportSend(unsigned char* send_data, int nbytes) {
    setAck;
    for (int i = 0; i < nbytes; i++) {
        while (_Strobe) YieldProcessor();
        setBusy;
        parallel_data = send_data[i];
        while (!_Strobe) YieldProcessor();
        clearBusy;
        while (_Strobe) YieldProcessor();
    }
    clearAck;
}
void micexkSendPacket(unsigned char* send_data) {
    BYTE encrypted[16];
    memset(encrypted, 0, sizeof encrypted);
    mxkCryptEncryptData(encrypted, send_data);
    micexkTransportSend(encrypted, 16);
}

void micexkTransportRecv(unsigned char* buffer, int nbytes) {
    for (int i = 0; i < nbytes; i++) {
        clearBusy;
        while (!_Strobe) YieldProcessor();
        buffer[i] = parallel_data;
        setBusy;
        while (_Strobe) YieldProcessor();
        clearBusy;
    }
}
void micexkRecvPacket(unsigned char* packet) {
    unsigned char buffer[16];
    micexkTransportRecv(buffer, 16);
    mxkCryptDecryptData(buffer, packet);
}

BYTE nvram[0x4000];
BYTE eeprom[0x1000];
BYTE flash[0x100000];
void init_nv_storage() {
    memset(nvram, 0xff, sizeof nvram);
    memset(eeprom, 0xff, sizeof eeprom);
    memset(flash, 0xff, sizeof flash);

    // NVRAM
    nvram_data_block_t block;
    ZeroMemory(&block, sizeof block);
    block.length = sizeof BILLING_PUBKEY;
    memcpy(block.data, BILLING_PUBKEY, sizeof BILLING_PUBKEY);
    memcpy(&nvram[0x1800], &block, sizeof block);

    block.length = sizeof BILLING_CACERT;
    memcpy(block.data, BILLING_CACERT, sizeof BILLING_CACERT);
    memcpy(&nvram[0x1c00], &block, sizeof block);

    // Flash
    billing_t billing_info;
    ZeroMemory(&billing_info, sizeof billing_info);
    billing_info.playlimit = 21046;
    billing_info.nearfull = 512;
    mxkSignValue(billing_info.nearfull, billing_info.nearfull_sig);
    mxkSignValue(billing_info.playlimit, billing_info.playlimit_sig);
    billing_info.crc = crc32(sizeof billing_info - 4, (unsigned char*)&billing_info + 4, 0);

    memcpy(&flash[0x7a000], &billing_info, sizeof billing_info);
    memcpy(&flash[0x7b000], &billing_info, sizeof billing_info);
}

void dump_nv_storage() {
    FILE* fp;
    fopen_s(&fp, "dev/kc/nvram.bin", "wb");
    fwrite(nvram, 1, sizeof nvram, fp);
    fclose(fp);
    fopen_s(&fp, "dev/kc/eeprom.bin", "wb");
    fwrite(eeprom, 1, sizeof eeprom, fp);
    fclose(fp);
    fopen_s(&fp, "dev/kc/flash.bin", "wb");
    fwrite(flash, 1, sizeof flash, fp);
    fclose(fp);
}

/*
addr = (0x70 + block) * 0x1000 + offset

Eeprom:
    Appears to store entries detailing tracedata metadata
    [4:crc32] [12:?]

    one copy at 0x000, one copy at 0x100
    16 entries
    first dword: CRC
    second dword: ??
    third dword: ??
    fourth dword:
        word 1: ??
        word 2: 0xffff

Nvram:
    2x 0x400 blocks
    [4:length] [length:data]
    first block: billing pubkey
    second block: cacert

Flash (25DF041A):
    nvram0: reads 070000<->070fff
    nvram1: reads 071000<->071fff
    nvram2: reads 072000<->072fff
    nvram3: reads 073000<->073fff
    nvram4: reads 074000<->074fff
    nvram5: reads 075000<->075fff
    nvram6: reads 076000<->076fff
    nvram7: reads 077000<->077fff
    nvram8: reads 078000<->078fff
    nvram9: reads 079000<->079fff

    billing info a: 0x7a000
    billing info b: 0x7b000

    - The first 0x80 bytes of each tracedata sector is a bitfield indicating which blocks have been
        used
    - There are 1022 0x40 blocks per tracedata sector (the last two are always unsed, to account for
        the 0x80 of metadata)
    - There are 7 tracedata sectors, at 0x00000, 0x10000, 0x20000, ..., 0x60000
    - Each slot is DES ECB encrypted, with key 4D77F1748D6D1094

*/

BYTE kc_aes_iv[16];

void mxparallel_process_packet(BYTE* request) {
    BYTE response[16];
    memset(response, 10, sizeof response);

    response[0] = request[0];
    switch (request[0]) {
        // We're pretending to be the keychip, so S and R are swapped for us!
        case SetKeyS:
            log_info("mxparallel", "SetKeyS");
            micexkRecvPacket(request);
            mxkSetKeyR(request);
            micexkSendPacket(response);
            break;
        case SetKeyR:
            log_info("mxparallel", "SetKeyR");
            micexkRecvPacket(request);
            mxkSetKeyS(request);
            micexkSendPacket(response);
            break;

        case Encrypt:
            log_info("mxparallel", "Encrypt");
            micexkRecvPacket(request);
            micexkSendPacket(request);
            break;
        case Decrypt:
            log_info("mxparallel", "Decrypt");
            micexkRecvPacket(request);
            micexkSendPacket(request);
            break;
        case SetIV:
            log_info("mxparallel", "SetIV");
            ZeroMemory(kc_aes_iv, sizeof kc_aes_iv);
            micexkSendPacket(response);
            break;
        case GetAppBootInfo:
            log_info("mxparallel", "GetAppBootInfo");
            if (request[1] != 0x00) {
                log_warning("mxparallel", "GetAppBootInfo[%d] unexpected!", request[1]);
            }

            APPBOOT.crc = crc32(sizeof APPBOOT - 4, (unsigned char*)&APPBOOT + 4, 0);
            for (int i = 0; i < sizeof APPBOOT; i += 16) {
                micexkSendPacket((unsigned char*)(&APPBOOT) + i);
            }
            break;

        case KcGetVersion:
            log_info("mxparallel", "GetVersion");
            response[0] = 0x01;
            response[1] = 0x04;
            micexkSendPacket(response);
            break;

        case FlashRead: {
            uint32_t addr = request[1] | (request[2] << 8) | (request[3] << 16);
            uint32_t nbytes = request[4] | (request[5] << 8) | (request[6] << 16);
            log_info("mxparallel", "FlashRead: %06x/%06x", addr, nbytes);

            if (addr + nbytes <= sizeof flash)
                micexkTransportSend(&flash[addr], nbytes);
            else
                log_error("mxparallel", "Flash read would exceed storage!");

            break;
        }
        case FlashErase: {
            uint32_t addr = request[1] | (request[2] << 8) | (request[3] << 16);
            log_info("mxparallel", "FlashErase: %06x", addr);

            micexkSendPacket(response);
            break;
        }
        case FlashWrite: {
            uint32_t addr = request[1] | (request[2] << 8) | (request[3] << 16);
            uint32_t nbytes = request[4] | (request[5] << 8) | (request[6] << 16);
            log_info("mxparallel", "FlashWrite: %06x/%06x", addr, nbytes);

            if (addr + nbytes <= sizeof flash)
                micexkTransportRecv(&flash[addr], nbytes);
            else
                log_error("mxparallel", "Flash write would exceed storage!");

            micexkSendPacket(response);
            dump_nv_storage();
            break;
        }

        // EEPROM?
        case EepromWrite: {
            // TODO: What is this? Appears to be some sort of EEPROM write
            uint8_t offset = request[1];
            log_info("mxparallel", "EepromWrite: %02x", offset);

            if (offset * 16 + 16 <= sizeof eeprom) {
                micexkRecvPacket(&eeprom[offset * 16]);
            } else {
                log_error("mxparallel", "EEPROM write would exceed storage!");
            }
            micexkSendPacket(response);
            dump_nv_storage();
            break;
        }
        case EepromRead: {
            // TODO: What is this? Appears to be some sort of EEPROM read
            uint8_t offset = request[1];
            log_info("mxparallel", "EepromRead: %02x", offset);

            if (offset * 16 + 16 <= sizeof eeprom) {
                micexkSendPacket(&eeprom[offset * 16]);
            } else {
                log_error("mxparallel", "EEPROM read would exceed storage!");
            }
            break;
        }

        // NVRAM?
        case NvramWrite: {
            // TODO: What is this? Appears to be some sort of memory write
            uint16_t addr = request[1] | (request[2] << 8);
            uint8_t blocks = request[3];
            log_info("mxparallel", "NvramWrite: %04x (%02x)", addr, blocks);

            if (addr + blocks * 16 <= sizeof nvram) {
                for (byte i = 0; i < blocks; i++) {
                    micexkRecvPacket(&(nvram[addr + (i * 16)]));
                }
                micexkSendPacket(response);
            } else {
                log_error("mxparallel", "NVRAM write would exceed storage!");
            }
            dump_nv_storage();
            break;
        }
        case NvramRead: {
            // TODO: What is this? Appears to be some sort of memory read
            uint16_t addr = request[1] | (request[2] << 8);
            uint8_t blocks = request[3];
            log_info("mxparallel", "NvramRead: %04x (%02x)", addr, blocks);

            if (addr + blocks * 16 <= sizeof nvram) {
                for (byte i = 0; i < blocks; i++) {
                    micexkSendPacket(&(nvram[addr + (i * 16)]));
                }
            } else {
                log_error("mxparallel", "NVRAM read would exceed storage!");
            }
            break;
        }

        case AddPlayCount:
            log_info("mxparallel", "AddPlayCount");
            BILLING_PLAYCOUNT++;
            micexkSendPacket(response);
            break;

        case SetMainId:
            log_info("mxparallel", "SetMainId");

            // micexkRecvPacket(MAIN_ID);
            micexkRecvPacket(request);
            response[0] = 0xff;
            micexkSendPacket(response);
            break;
        case GetMainId:
            log_info("mxparallel", "GetMainId");
            micexkSendPacket(MAIN_ID);
            break;
        case SetKeyId:
            log_info("mxparallel", "SetKeyId");
            micexkRecvPacket(KEYCHIP_ID);
            micexkSendPacket(response);
            break;
        case GetKeyId:
            log_info("mxparallel", "GetKeyId");
            micexkSendPacket(KEYCHIP_ID);
            break;

        case GetPlayCounter:
            log_info("mxparallel", "GetPlayCounter");
            ((uint32_t*)response)[0] = BILLING_PLAYCOUNT;
            micexkSendPacket(response);
            break;

        default:
            log_error("mxparallel", "Unhandled opcode: %d", request[0]);
            for (byte i = 0; i < 16; i++) {
                printf("%02x ", request[i]);
            }
            puts("");

            break;
    }
}

DWORD WINAPI mxparallel_thread(LPVOID _) {
    log_info("mxparallel", "Parallel device thread spawned");
    clearBusy;
    clearAck;

    BYTE last_strobe = 0x00;
    while (1) {
        unsigned char packet[16];
        micexkRecvPacket(packet);
        mxparallel_process_packet(packet);
    }
}

void setup_mxparallel() {
    // We're pretending to be a keychip, so need to swap the keys!
    mxkSwapKeys();

    init_nv_storage();

    file_hook_t* mxparallel = new_file_hook(L"\\\\.\\mxparallel");
    mxparallel->DeviceIoControl = &mxparallel_DeviceIoControl;
    hook_file(mxparallel);

    CreateThread(NULL, 0, mxparallel_thread, NULL, 0, NULL);
}
