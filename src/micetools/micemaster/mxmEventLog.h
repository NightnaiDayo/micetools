#include <Windows.h>
#include <stdbool.h>

typedef struct {
    unsigned int Rsv00;
    unsigned int Rsv04;
    unsigned int m_numRecords;
    char m_filename[MAX_PATH];
} mxmEventLogDesination_t;

typedef struct {
    bool m_init;
    bool m_logDriveFound;
    bool m_filenamesSet;
    unsigned int m_backupCount;
    unsigned int m_clearCount;
    unsigned int m_interval;
    mxmEventLogDesination_t m_desinations[3];
} mxmEventLog_t;

extern mxmEventLog_t MxmEventLog;

bool mxmEventLogGetLogDrive(char *logPath);
int mxmEventLogSetDestination(char *name, unsigned int which);
void mxmEventLogInit(void);
