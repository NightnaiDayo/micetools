#include "../../amBackupStructs.h"
#include "../../maiBackupStructs.h"
#include "_devices.h"

#define EEPROM_DUMP L"dev/eeprom.bin"

// 8192 x 8 (64kbit) of eeprom
BYTE EEPROM_DATA[0x2000];

void eeprom_dump() {
    HANDLE dump =
        _CreateFileW(EEPROM_DUMP, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
    if (dump == INVALID_HANDLE_VALUE) {
        log_error("eeprom", "CreateFileA(EEPROM_DUMP) failed: %03x", GetLastError());
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

#define SET_IP(val, a, b, c, d)                                              \
    do {                                                                     \
        *(uint32_t*)&val = (uint32_t)((a << 24) | (b << 16) | (c << 8) | d); \
    } while (0)

#define fix_crc(block)                                                            \
    do {                                                                          \
        (block).m_Crc = amCrc32RGet(sizeof(block) - 4, (BYTE*)(&(block)) + 4, 0); \
    } while (0)

void set_eeprom_network_config() {
    AM_SYSDATAwH_NETWORK_ETH0 NetEth0 = { 0 };
    NetEth0.m_Eth.m_Flag = 0;
    NetEth0.m_Eth.m_IpAddress = MiceConfig.network.ip_address;
    NetEth0.m_Eth.m_SubnetMask = MiceConfig.network.subnet_mask;
    NetEth0.m_Eth.m_Gateway = MiceConfig.network.gateway;
    NetEth0.m_Eth.m_PrimaryDns = MiceConfig.network.primary_dns;
    NetEth0.m_Eth.m_SecondaryDns = MiceConfig.network.secondary_dns;
    fix_crc(NetEth0);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_NETWORK_ETH0_REG], &NetEth0, sizeof NetEth0);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_NETWORK_ETH0_DUP], &NetEth0, sizeof NetEth0);

    AM_SYSDATAwH_NETWORK_ETH1 NetEth1 = { 0 };
    fix_crc(NetEth1);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_NETWORK_ETH1_REG], &NetEth1, sizeof NetEth1);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_NETWORK_ETH1_DUP], &NetEth1, sizeof NetEth1);
}

int build_eeprom() {
    static BOOL built = false;
    if (built) return 0;
    built = true;

    if (FileExists(EEPROM_DUMP)) {
        eeprom_restore();
        // Our network config always gets priority
        set_eeprom_network_config();
        return 0;
    }

    log_info("eeprom", "Building default EEPROM file");

    memset(EEPROM_DATA, 0xff, sizeof EEPROM_DATA);

    AM_SYSDATAwH_STATIC Static = {
        .m_Region = MiceConfig.sysconf.region & (1 | 2 | 4 | 8),
        .m_Rental = MiceConfig.sysconf.rental,
    };
    strcpy_s(Static.m_strSerialId, sizeof Static.m_strSerialId, MiceConfig.sysconf.serial);
    fix_crc(Static);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_STATIC_REG], &Static, sizeof Static);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_STATIC_DUP], &Static, sizeof Static);

    AM_SYSDATAwH_CREDIT Credit = { 0 };
    fix_crc(Credit);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_CREDIT_REG], &Credit, sizeof Credit);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_CREDIT_DUP], &Credit, sizeof Credit);

    set_eeprom_network_config();

    AM_SYSDATAwH_HISTORY History = { 0 };
    // TODO: Game ID here should be configurable.
    History.m_GameId[0] = 'S';
    History.m_GameId[1] = 'D';
    History.m_GameId[2] = 'E';
    History.m_GameId[3] = 'Y';
    History.m_Region = MiceConfig.sysconf.region & (1 | 2 | 4 | 8);
    fix_crc(History);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_HISTORY_REG], &History, sizeof History);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_HISTORY_DUP], &History, sizeof History);
    AM_SYSDATAwH_ALPB_CARD_ID CardInfo = { 0 };
    fix_crc(CardInfo);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_ALPB_CARD_ID_REG] + (sizeof History), &CardInfo,
           sizeof CardInfo);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_ALPB_CARD_ID_DUP] + (sizeof History), &CardInfo,
           sizeof CardInfo);

    AM_SYSDATAwH_ALPB_COMPUTER_NAME CompuerName = { 0 };
    fix_crc(CompuerName);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_ALPB_COMPUTER_NAME_REG], &CompuerName, sizeof CompuerName);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_ALPB_COMPUTER_NAME_DUP], &CompuerName, sizeof CompuerName);

    AM_SYSDATAwH_ALPB_DEV_CONFIG DevConfig = { 0 };
    fix_crc(DevConfig);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_ALPB_DEV_CONFIG_REG], &DevConfig, sizeof DevConfig);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_ALPB_DEV_CONFIG_DUP], &DevConfig, sizeof DevConfig);

    eeprom_dump();
    return 1;
}

void eeprom_read(WORD addr, BYTE* data, BYTE length) {
    if (!build_eeprom()) eeprom_restore();

    if (addr >= sizeof EEPROM_DATA) return;
    if (length + addr > sizeof EEPROM_DATA) length = (sizeof EEPROM_DATA - addr) & 0xff;

    memcpy(data, &EEPROM_DATA[addr], length);
}
void eeprom_write(WORD addr, BYTE* data, BYTE length) {
    if (!build_eeprom()) eeprom_restore();

    if (addr >= sizeof EEPROM_DATA) return;
    if (length + addr > sizeof EEPROM_DATA) length = (sizeof EEPROM_DATA - addr) & 0xff;

    memcpy(&EEPROM_DATA[addr], data, length);
    eeprom_dump();
}

BOOL smbus_AT24C64AN_write(ich9_cmd_t cmd, WORD code, BYTE dlen, BYTE* data) {
    switch (cmd) {
        case ICH9_CMD_BLOCK: {
            log_misc("eeprom", "write %d bytes at 0x%03x", dlen, code);
            eeprom_write(code, data, dlen);
            return TRUE;
        }
        default:
            log_error("eeprom", "Unsupported write mode: %01x, %02x", cmd, code);
            return FALSE;
    }
}

BOOL smbus_AT24C64AN_read(ich9_cmd_t cmd, WORD code, BYTE dlen, BYTE* data) {
    switch (cmd) {
        case ICH9_CMD_BYTE:
            data[0] = 0x00;
            return TRUE;
        case ICH9_CMD_BLOCK: {
            log_misc("eeprom", "read %d bytes at 0x%03x", dlen, code);
            eeprom_read(code, data, dlen);
            return TRUE;
        }
        default:
            log_error("eeprom", "Unsupported read mode: %01x, %02x", cmd, code);
            return FALSE;
    }
}
