#include <stdint.h>

#pragma pack(push, 1)

typedef struct {
    uint32_t m_Flag;
    uint32_t m_IpAddress;
    uint32_t m_SubnetMask;
    uint32_t m_Gateway;
    uint32_t m_PrimaryDns;
    uint32_t m_SecondaryDns;
} AM_SYSDATA_NETWORK_IF;

typedef struct {
    uint64_t m_TimeStamp;
    char GameId[4];
    uint16_t m_Error;
    uint8_t m_AppStartCount;
    uint8_t Rsv0f[17];
} ERROR_LOG_BODY;

typedef struct {
    uint8_t ChuteType;
    uint8_t ServiceType;
    uint8_t Operation;
    uint8_t CoinRate[2];
    uint8_t BonusAdder;
    uint8_t CreditRate;
    uint8_t Cost[8];
    uint8_t Rsv0F;
} AM_CREDIT_CONFIG;

typedef struct {
    uint8_t Credit;
    uint8_t Remain;
} AM_CREDIT_PDATA;

typedef struct {
    AM_CREDIT_PDATA Player[4];
    uint8_t Rsv08[8];
} AM_CREDIT_DATA;

typedef struct {
    uint32_t CoinChute[4];
    uint32_t TotalCoin;
    uint32_t CoinCredit;
    uint32_t ServiceCredit;
    uint32_t TotalCredit;
} AM_CREDIT_BOOKKEEPING;

typedef struct {
    uint8_t m_month;
    uint8_t m_week;
    uint8_t m_dayOfWeek;
    uint8_t m_hour;
    uint8_t m_minute;
    uint8_t m_rsv5[3];
} alpbSysdataDst_T;

typedef struct {
    uint32_t m_Crc;
    uint8_t Rsv04[4];
    AM_SYSDATA_NETWORK_IF m_Eth;
} AM_SYSDATAwH_NETWORK;

typedef struct {
    uint32_t m_Crc;
    uint8_t Rsv04[8];
    uint8_t m_Region;
    uint8_t m_Rental;
    uint8_t Rsv0F;
    char m_strSerialId[17];
} AM_SYSDATAwH_STATIC;

typedef struct {
    uint32_t m_Crc;
    uint8_t Rsv04[4];
    AM_CREDIT_CONFIG m_Config;
    uint8_t Rsv18[8];
} AM_SYSDATAwH_CREDIT;

typedef AM_SYSDATAwH_NETWORK AM_SYSDATAwH_NETWORK_ETH0;
typedef AM_SYSDATAwH_NETWORK AM_SYSDATAwH_NETWORK_ETH1;

typedef struct {
    uint32_t m_Crc;
    uint8_t Rsv04[4];
    char m_GameId[4];
    uint8_t m_Region;
    uint8_t Rsv0D[3];
} AM_SYSDATAwH_HISTORY;

typedef struct {
    uint32_t m_Crc;
    uint8_t Rsv04[4];
    uint8_t m_WritePointer;
    uint8_t m_LogNum;
    uint8_t Rsv0a[22];
    ERROR_LOG_BODY m_Body[15];
} AM_SYSDATAwH_ERROR_LOG;

typedef struct {
    uint32_t m_Crc;
    uint8_t Rsv04[4];
    AM_CREDIT_DATA m_CreditData;
    AM_CREDIT_BOOKKEEPING m_Bookkeeping;
    uint8_t Rsv38[456];
} AM_SYSDATAwH_BACKUP;

typedef struct {
    uint32_t m_Crc;
    uint8_t Rsv04[4];
    alpbSysdataDst_T m_Daylight;
    alpbSysdataDst_T m_Standard;
    uint32_t m_Bias;
    uint32_t m_ServerBias;
    uint8_t Rsv20[480];
} AM_SYSDATAwH_TIMEZONE;

typedef struct {
    uint32_t m_Crc;
    uint8_t Rsv04[4];
    uint32_t m_Caution;
    uint32_t m_Peak;
    uint8_t Rsv10[496];
} AM_SYSDATAwH_HM_PEAK;

typedef struct {
    uint32_t m_Crc;
    uint8_t Rsv04[4];
    uint8_t m_MountSleepS;
    uint8_t Rsv09[55];
} AM_SYSDATAwH_ALPB_DEV_CONFIG;

typedef struct {
    uint32_t m_Crc;
    uint32_t m_Uk1;
    uint32_t m_Uk2;
    uint32_t m_Uk3;
} AM_SYSDATAwH_ALPB_CARD_ID;

typedef struct {
    uint32_t m_Crc;
    uint32_t m_Uk1;
    uint32_t m_Uk2;
    uint32_t m_Uk3;
    uint32_t m_Uk4;
    uint32_t m_Uk5;
    uint32_t m_Uk6;
    uint32_t m_Uk7;
} AM_SYSDATAwH_ALPB_COMPUTER_NAME;

#pragma pack(pop)

#define AM_SYSDATAwH_STATIC_REG 0x000
#define AM_SYSDATAwH_STATIC_DUP 0x200
#define AM_SYSDATAwH_CREDIT_REG 0x020
#define AM_SYSDATAwH_CREDIT_DUP 0x220
#define AM_SYSDATAwH_NETWORK_ETH0_REG 0x040
#define AM_SYSDATAwH_NETWORK_ETH0_DUP 0x240
#define AM_SYSDATAwH_NETWORK_ETH1_REG 0x060
#define AM_SYSDATAwH_NETWORK_ETH1_DUP 0x260
#define AM_SYSDATAwH_HISTORY_REG 0x080
#define AM_SYSDATAwH_HISTORY_DUP 0x280
#define AM_SYSDATAwH_ALPB_CARD_ID_REG 0x090
#define AM_SYSDATAwH_ALPB_CARD_ID_DUP 0x290
#define AM_SYSDATAwH_ALPB_COMPUTER_NAME_REG 0x0a0
#define AM_SYSDATAwH_ALPB_COMPUTER_NAME_DUP 0x2a0
#define AM_SYSDATAwH_ALPB_DEV_CONFIG_REG 0x0c0
#define AM_SYSDATAwH_ALPB_DEV_CONFIG_DUP 0x2c0

/**
 * Storage locations:
 *
 * Static: (AM_SYSDATAwH_STATIC)
 *  0x000 - 0x01f, eeprom
 *  0x200 - 0x21f, eeprom
 * Credit: (AM_SYSDATAwH_CREDIT)
 *  0x020 - 0x03f, eeprom
 *  0x220 - 0x23f, eeprom
 * Network Eth0: (AM_SYSDATAwH_NETWORK_ETH0)
 *  0x040 - 0x05f, eeprom
 *  0x240 - 0x25f, eeprom
 * Network Eth1: (AM_SYSDATAwH_NETWORK_ETH1)
 *  0x060 - 0x07f, eeprom
 *  0x260 - 0x27f, eeprom
 * History: (AM_SYSDATAwH_HISTORY)
 *  0x080 - 0x08f, eeprom
 *  0x280 - 0x28f, eeprom
 * Alpb Card Id: (AM_SYSDATAwH_ALPB_CARD_ID)
 *  0x090 - 0x09f, eeprom
 *  0x290 - 0x29f, eeprom
 * Computer Name: (AM_SYSDATAwH_ALPB_COMPUTER_NAME)
 *  0x0a0 - 0x0bf, eeprom
 *  0x2a0 - 0x2bf, eeprom
 * Dev Config (AM_SYSDATAwH_ALPB_DEV_CONFIG)
 *  0x0c0 - 0x0ff, eeprom
 *  0x2c0 - 0x2ff, eeprom
 *
 * Backup: (AM_SYSDATAwH_BACKUP)
 *  0x0000 - 0x01ff, sram
 *  0x1000 - 0x11ff, sram
 * HM Peak: (AM_SYSDATAwH_HM_PEAK)
 *  0x0200 - 0x03ff, sram
 *  0x1200 - 0x13ff, sram
 * Timezone: (AM_SYSDATAwH_TIMEZONE)
 *  0x0400 - 0x05ff, sram
 *  0x1400 - 0x15ff, sram
 * Error Log: (AM_SYSDATAwH_ERROR_LOG)
 *  0x0600 - 0x07ff, sram
 *  0x1600 - 0x17ff, sram
 */
#define AM_SYSDATAwH_BACKUP_REG 0x0000
#define AM_SYSDATAwH_BACKUP_DUP 0x1000
#define AM_SYSDATAwH_HM_PEAK_REG 0x0200
#define AM_SYSDATAwH_HM_PEAK_DUP 0x1200
#define AM_SYSDATAwH_TIMEZONE_REG 0x0400
#define AM_SYSDATAwH_TIMEZONE_DUP 0x1400
#define AM_SYSDATAwH_ERROR_LOG_REG 0x0600
#define AM_SYSDATAwH_ERROR_LOG_DUP 0x1600
