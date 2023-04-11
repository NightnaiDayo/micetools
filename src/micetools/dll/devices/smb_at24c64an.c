#include "../../amBackupStructs.h"
#include "../../maiBackupStructs.h"
#include "_devices.h"

#define EEPROM_PATH L"dev/eeprom.bin"

#include "../../sysconf.h"

// 8192 x 8 (64kbit) of eeprom
#define EEPROM_SIZE 0x2000

LPBYTE EEPROM_DATA = NULL;
HANDLE EEPROM_FILE = INVALID_HANDLE_VALUE;
HANDLE EEPROM_FILE_MAPPING = INVALID_HANDLE_VALUE;

#define fix_crc(block)                                                              \
    do {                                                                            \
        (block).m_Crc = amiCrc32RCalc(sizeof(block) - 4, (BYTE*)(&(block)) + 4, 0); \
    } while (0)

void set_eeprom_network_config() {
    AM_SYSDATAwH_NETWORK_ETH0 NetEth0 = { 0 };
    NetEth0.m_Eth.m_Flag = 0;  // TODO: Allow DHCP
    NetEth0.m_Eth.m_IpAddress = _byteswap_ulong(MiceConfig.network.ip_address);
    NetEth0.m_Eth.m_SubnetMask = _byteswap_ulong(MiceConfig.network.subnet_mask);
    NetEth0.m_Eth.m_Gateway = _byteswap_ulong(MiceConfig.network.gateway);
    NetEth0.m_Eth.m_PrimaryDns = _byteswap_ulong(MiceConfig.network.primary_dns);
    NetEth0.m_Eth.m_SecondaryDns = _byteswap_ulong(MiceConfig.network.secondary_dns);
    fix_crc(NetEth0);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_NETWORK_ETH0_REG], &NetEth0, sizeof NetEth0);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_NETWORK_ETH0_DUP], &NetEth0, sizeof NetEth0);

    // TODO: Does anything ever use ETH1?
    AM_SYSDATAwH_NETWORK_ETH1 NetEth1 = { 0 };
    fix_crc(NetEth1);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_NETWORK_ETH1_REG], &NetEth1, sizeof NetEth1);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_NETWORK_ETH1_DUP], &NetEth1, sizeof NetEth1);
}

void set_eeprom_static_config() {
    AM_SYSDATAwH_STATIC Static = {
        .m_Region = MiceConfig.sysconf.region & (1 | 2 | 4 | 8),
        .m_Rental = MiceConfig.sysconf.rental,
    };
    strcpy_s(Static.m_strSerialId, sizeof Static.m_strSerialId, MiceConfig.sysconf.serial);
    fix_crc(Static);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_STATIC_REG], &Static, sizeof Static);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_STATIC_DUP], &Static, sizeof Static);
}

void build_eeprom() {    // return;
    log_info(plfEeprom, "Building default EEPROM file");

    memset(EEPROM_DATA, 0xff, EEPROM_SIZE);

    set_eeprom_static_config();

    AM_SYSDATAwH_CREDIT Credit = {
        .m_Config = {
            .ServiceType = 1,
            .CreditRate = 1,
            .CoinRate = { 1, 1 },
            .Cost = { 1, 1, 1, 1, 1, 1, 1, 1 },
        },
     };
    fix_crc(Credit);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_CREDIT_REG], &Credit, sizeof Credit);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_CREDIT_DUP], &Credit, sizeof Credit);

    set_eeprom_network_config();

    AM_SYSDATAwH_ALPB_CARD_ID CardInfo = { 0 };
    fix_crc(CardInfo);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_ALPB_CARD_ID_REG], &CardInfo, sizeof CardInfo);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_ALPB_CARD_ID_DUP], &CardInfo, sizeof CardInfo);

    AM_SYSDATAwH_ALPB_COMPUTER_NAME CompuerName = { 0 };
    fix_crc(CompuerName);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_ALPB_COMPUTER_NAME_REG], &CompuerName, sizeof CompuerName);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_ALPB_COMPUTER_NAME_DUP], &CompuerName, sizeof CompuerName);

    AM_SYSDATAwH_ALPB_DEV_CONFIG DevConfig = { 0 };
    fix_crc(DevConfig);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_ALPB_DEV_CONFIG_REG], &DevConfig, sizeof DevConfig);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_ALPB_DEV_CONFIG_DUP], &DevConfig, sizeof DevConfig);

    // eeprom_dump();
}

AM_SYSDATAwH_HISTORY AmHistory = {
    // By making the game ID "____" we ensure games overwrite this, letting us see what they write.
    // This is currently the best way I have found to retrieve the game ID
    .m_GameId = { '_', '_', '_', '_' },
};
/** Clobber EEPROM with our user settings */
void eeprom_fixup() {
    set_eeprom_static_config();
    set_eeprom_network_config();

    AmHistory.m_Region = MiceConfig.sysconf.region & (1 | 2 | 4 | 8);
    fix_crc(AmHistory);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_HISTORY_REG], &AmHistory, sizeof AmHistory);
    memcpy(&EEPROM_DATA[AM_SYSDATAwH_HISTORY_DUP], &AmHistory, sizeof AmHistory);
}

void ensure_valid_eeprom() {
    if (!EEPROM_DATA) {
        BOOL isNew = !FileExists(EEPROM_PATH);

        EEPROM_DATA =
            open_mapped_file(EEPROM_PATH, EEPROM_SIZE, &EEPROM_FILE, &EEPROM_FILE_MAPPING);
        if (EEPROM_DATA == NULL) {
            log_error(plfEeprom, "EEPROM will be memory-backed and not syncronised!");
            EEPROM_DATA = malloc(EEPROM_SIZE);
        }

        if (isNew) build_eeprom();
    }
    eeprom_fixup();
}

void eeprom_read(WORD addr, BYTE* data, BYTE length) {
    ensure_valid_eeprom();

    if (addr >= EEPROM_SIZE) return;
    if (length + addr > EEPROM_SIZE) length = (EEPROM_SIZE - addr) & 0xff;

    memcpy(data, EEPROM_DATA + addr, length);
}
void eeprom_write(WORD addr, BYTE* data, BYTE length) {
    ensure_valid_eeprom();

    if (addr >= EEPROM_SIZE) return;
    if (length + addr > EEPROM_SIZE) length = (EEPROM_SIZE - addr) & 0xff;

    // TODO: Do any games write this as anything other than a complete block?
    if (addr == AM_SYSDATAwH_HISTORY_REG && length == sizeof(AM_SYSDATAwH_HISTORY)) {
        PAM_SYSDATAwH_HISTORY pHistory = (PAM_SYSDATAwH_HISTORY)data;

        mice_got_game_id(pHistory->m_GameId);
    } else if (addr >= 0x1000 && length > 8) {
        char game_id[4];
        game_id[0] = data[7];
        game_id[1] = data[6];
        game_id[2] = data[5];
        game_id[3] = data[4];
        mice_got_game_id(game_id);
    }

    memcpy(EEPROM_DATA + addr, data, length);
}

BOOL smbus_AT24C64AN_write(ich9_cmd_t cmd, WORD code, BYTE dlen, BYTE* data) {
    switch (cmd) {
        case ICH9_CMD_BLOCK: {
            log_misc(plfEeprom, "write %d bytes at 0x%03x", dlen, code);
            eeprom_write(code, data, dlen);
            return TRUE;
        }
        default:
            log_error(plfEeprom, "Unsupported write mode: %01x, %02x", cmd, code);
            return FALSE;
    }
}

BOOL smbus_AT24C64AN_read(ich9_cmd_t cmd, WORD code, BYTE dlen, BYTE* data) {
    switch (cmd) {
        case ICH9_CMD_BYTE:
            data[0] = 0x00;
            return TRUE;
        case ICH9_CMD_BLOCK: {
            log_misc(plfEeprom, "read %d bytes at 0x%03x", dlen, code);
            eeprom_read(code, data, dlen);
            return TRUE;
        }
        default:
            log_error(plfEeprom, "Unsupported read mode: %01x, %02x", cmd, code);
            return FALSE;
    }
}
