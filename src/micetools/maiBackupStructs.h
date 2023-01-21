#include <stdint.h>

#pragma pack(push, 1)

/**
 * @brief A sigle byte of EEPROM storage, seemingly unused
 * Base:   1000
 * Backup: 100D
 */
typedef struct {
    uint32_t m_Crc;
    uint32_t m_GameId;
    uint32_t m_Length;  // = 5
    uint8_t Unk04;
} MAI_EEPROM_REGION0;
#define MAI_EEPROM_REGION0_BASE 0x1000
#define MAI_EEPROM_REGION0_DUP 0x100d

/**
 * Base:   4000
 * Backup: 4034
 * Cleared by BACKUP DATA CLEAR
 */
typedef struct {
    uint32_t m_Crc;
    uint32_t m_GameId;
    uint32_t m_Length;  // = 44
    uint32_t m_NumberOfGames;
    uint32_t m_TotalTime;
    uint32_t m_TotalPlayTime;
    uint32_t Unk10[4];
    uint32_t m_LongestPlayTime;
    uint32_t m_ShortestPlayTime;
    uint32_t m_AveragePlayTime;
} MAI_SRAM_REGION0;
#define MAI_SRAM_REGION0_BASE 0x4000
#define MAI_SRAM_REGION0_DUP 0x4034

/**
 * Base:   4068
 * Backup: 4090
 * Cleared by BACKUP DATA CLEAR
 */
typedef struct {
    uint32_t m_Crc;
    uint32_t m_GameId;
    uint32_t m_Version;  // = 190
    uint8_t Rsv04[28];
} MAI_SRAM_REGION1;
#define MAI_SRAM_REGION1_BASE 0x4068
#define MAI_SRAM_REGION1_DUP 0x4090

typedef struct {
    uint8_t m_Reported;  // 0 = 済 (not yet), 1 = 未 (already)
    uint8_t Rsv01[3];
    uint32_t m_Timestamp;  // Unix timestamp
    uint32_t m_BackupClearTimestamp;  // YYMMDDhhmm when displayed as int
    uint32_t m_NumberOfCoinsInserted;
    uint32_t m_CreditByCoinInsertion;
    uint32_t m_CreditByServiceButton;
    uint32_t m_PowerOnTime;  // Excluding test mode
    uint32_t m_TotalPlays;
    uint32_t m_NumberOf1PGames;
    uint32_t m_NumberOfSyncGames;
    uint32_t m_NumberOfVsGames;
    uint32_t m_NumberOfTutorials;
    uint32_t m_NumberOfUploads;
    uint32_t m_NumberOfNewCardFree;
    uint32_t m_ErrorCount[8];
    uint32_t m_TotalPlayTime;
    uint32_t m_ShortestPlayTime;
    uint32_t m_LongestPlayTime;
    uint8_t Rsv64[32];
} MAI_SRAM_Meter;
/**
 * @brief Accounting information
 * Base:   40b8
 * Backup: 44c0
 * Cleared by BACKUP DATA CLEAR
 */
typedef struct {
    uint32_t m_Crc;
    uint32_t m_GameId;
    uint32_t m_Version;  // = 190
    uint32_t m_CreditToStart;
    uint32_t m_CreditToStart2P;
    uint32_t m_NumberOf1PGames;
    uint32_t m_NumberOfSyncGames;
    uint32_t m_NumberOfVsGames;
    uint32_t m_NumberOfTutorials;
    uint32_t m_NumberOfUploads;
    uint32_t m_NumberOfGuestPlays;
    uint32_t m_NumberOfNewUsers;
    uint32_t m_NumberOfNewCardFree;
    uint32_t m_NumberOfNewCardPaid;
    uint32_t m_NumberOfNewWithMultiplePeople;
    uint16_t m_Year;
    uint8_t m_Month;
    uint8_t m_Day;
    uint8_t m_Hour;
    uint8_t m_Minute;
    uint8_t m_Second;
    uint8_t Rsv3B;
    MAI_SRAM_Meter m_Meter[7];
    uint32_t m_NumberOfCoinsInserted;
    uint32_t m_CreditByCoinInsertion;
    uint32_t m_CreditByServiceButton;
    uint32_t m_NumberOfGames;
    uint32_t m_TotalTime;
    uint8_t Rsv3EC[20];
} MAI_SRAM_REGION2;
#define MAI_SRAM_REGION2_BASE 0x40b8
#define MAI_SRAM_REGION2_DUP 0x44c0

/**
 * @brief Game Assignments
 * Base:   48c8
 * Backup: 4950
 * Not cleared by BACKUP DATA CLEAR
 */
typedef struct {
    uint32_t m_Crc;
    uint32_t m_GameId;
    uint32_t m_Version;  // = 190
    uint8_t m_NumberOfTracks1P;  // 3 or 4
    uint8_t m_NumberOfTracksMulti;  // 3 or 4
    uint8_t m_NumberOfTracksEvent;  // 1, 2, 3, 4
    uint8_t m_EventMode;  // 0, 1 = OFF, ON
    uint8_t m_OnlineMode;  // 0, 1 = OFF, ON
    uint8_t m_TotalMachine;  // 1, 2
    uint8_t m_LinkId;  // 1, 2
    uint8_t m_AdvertiseSound;  // 0, 1, 2, 3, 4 = 0%, 25%, 50%, 75%, 100%
    uint8_t m_AdvertiseMode;  // 0, 1 = RANKING, BASIC
    uint8_t m_CameraPosition;  // 0, 1, 2 = NO CAMERA, RIGHT, LEFT
    uint8_t m_TouchSensorCalib1P[17];  // all bytes in range 0-99, inclusive
    uint8_t m_TouchSensorCalbi2P[17];  // all bytes in range 0-99, inclusive
    uint8_t m_DressCode;  // 0, 1 = LOW, HIGH
    uint8_t m_CloseScheduleType;  // 0, 1 = DAY, WEEK
    uint8_t m_CloseScheduleHour[8];  // all bytes in range 0-30, inclusive, 0 = ALLTIME, [0] = Day, [1-7] = Week
    uint8_t m_CloseScheduleMinute[8];  // all bytes in range 0-59, inclusive, [0] = Day, [1-7] = Week
    uint8_t Rsv42[62];
} MAI_SRAM_REGION3;
#define MAI_SRAM_REGION3_BASE 0x48c8
#define MAI_SRAM_REGION3_DUP 0x4950

/**
 * Base:   49d8
 * Backup: 4a20
 * Cleared by BACKUP DATA CLEAR
 */
typedef struct {
    uint32_t m_Crc;
    uint32_t m_GameId;
    uint32_t m_Version;  // = 190
    uint32_t m_Timestamp;  // What is this? Hay's is Sat May 28 2022 07:27:40 GMT+0000. Maybe "last backup clear time"
    uint8_t Rsv08[56];
} MAI_SRAM_REGION4;
#define MAI_SRAM_REGION4_BASE 0x49d8
#define MAI_SRAM_REGION4_DUP 0x4a20

/**
 * @brief Something to do with downloads. Not going to get very far here until I have those working!
 * Base:   4a68
 * Backup: 4ad0
 * Not cleared by BACKUP DATA CLEAR
 */
typedef struct {
    uint32_t m_Crc;
    uint32_t m_GameId;
    uint32_t m_Version;  // = 190
    uint32_t m_ErrorCount[8];
    uint8_t Unk24;
    char m_GameVersion[5];
    char Unk2A[32];
    uint16_t Unk4A;
    uint8_t Rsv4C[20];
} MAI_SRAM_REGION5;
#define MAI_SRAM_REGION5_BASE 0x4a68
#define MAI_SRAM_REGION5_DUP 0x4ad0

#pragma pack(pop)
