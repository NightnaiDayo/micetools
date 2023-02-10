#include <Windows.h>
#include <stdbool.h>

#include "../lib/am/amInstall.h"
#include "../lib/ami/amiTimer.h"

typedef struct {
    amtime_t m_last;
    unsigned int m_numRecords;
    char m_filename[MAX_PATH];
} mxmEventLogDesination_t;

#pragma pack(push, 1)
typedef struct {
    unsigned int m_crc32;
    char m_gameId[4];
    unsigned char Rsv08[8];
    unsigned int m_nBytes[2];
    unsigned char Rsv18[8];
    unsigned char m_md5[2][16];
    unsigned char Rsv40[448];
} AM_EVENT_LOG_HEADER;
#pragma pack(pop)

#define NUM_LOGS 3

typedef struct {
    bool m_init;
    bool m_logDriveFound;
    bool m_filenamesSet;
    unsigned int m_backupCount;
    unsigned int m_clearCount;
    unsigned int m_interval;
    mxmEventLogDesination_t m_desinations[NUM_LOGS];
} mxmEventLog_t;

extern mxmEventLog_t MxmEventLog;

#define LOG_MAX_SIZE (1024 * 512)
// Offsets relative to [os] partition, in bytes
#define LOG_OFFSET (2047 * 512)
#define LOG_1_OFFSET (2048 * 512)
#define LOG_2_OFFSET (LOG_1_OFFSET + LOG_MAX_SIZE)

bool mxmEventLogGetTempPath(char *name, char *buffer);
bool mxmEventLogGetLogDrive(char *logPath);
int mxmEventLogSetDestination(char *name, unsigned int which);
void mxmEventLogInit(void);
bool mxmEventLogEraseLog(void);
bool mxmEventLogCheckLog(bool *valid);
bool mxmEventLogAccessGetMainStorageDeviceNum(int *devNum);
bool mxmEventLogAccessEraseEventLogData(int deviceNum, unsigned long long address);
bool mxmEventLogAccessCheckEventLogData(int deviceNum, unsigned long long address, bool *valid);
bool mxmEventLogAccessLoadEpbr(AM_INSTALL_BOOT_RECORD *bootRecord);
bool mxmEventLogAccessReadEventLogData(int deviceNum, unsigned long long address,
                                       unsigned char *eventlog1, unsigned int *nBytes1,
                                       unsigned char *eventlog2, unsigned int *nBytes2);
DWORD mxmEventLogAccessWriteDataFile(char *filename, unsigned char *dataFile, unsigned int nBytes);
bool mxmEventLogAccessCopyIntoDataFile(int deviceNum, unsigned long long address, char *filename,
                                       unsigned int type);
bool mxmEventLogAccessBackup(char *filename, int type);
