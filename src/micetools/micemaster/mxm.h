#include <stdio.h>

#include "appLauncher.h"
#include "callbacks/callbacks.h"
#include "config.h"
#include "lib/am/amDongle.h"
#include "lib/libpcp/libpcp.h"
#include "mxmEventLog.h"

typedef enum {
    mxMasterStartType_0 = 0,
    mxMasterStartType_1 = 1,  // Stores system message
    mxMasterStartType_2 = 2,  // Stores "path" into m_nextPath
    mxMasterStartType_3 = 3,
    mxMasterStartType_4 = 4,
    mxMasterStartType_5 = 5,

    // Segaboot: 1, 5, 6
    // Gameboot: 2
    // Systemtest: 3

} mxMasterStartType_t;

typedef struct MX_MASTER_ {
    ushort m_controlPort;
    ushort m_dataPort;
    int m_openMode;

    bool m_processStateChange;
    bool m_changeIsSpawn;
    bool m_backupMetadataChanged;

    unsigned int m_current;
    unsigned int m_next;
    bool m_develop;
    bool m_fault;

    bool m_loggingAvailable;
    unsigned int m_backupCount;
    unsigned int m_clearCount;
    unsigned int m_interval;
    char m_nextPath[128];
    char m_gameId[8];
    char m_platformId[8];
    char m_message[256];
    char m_keychipId[32];
    IN_ADDR m_networkAddr;
    int m_countSystemBoot;
    int m_countGameBoot;
    int m_countSystemTest;
    int m_countGameTest;
    unsigned char m_binaryMessage[512];
    size_t m_binaryMessageLen;
    bool m_serverOpen;
    bool m_kcReady;
    bool m_pcpaHasInit;
    appLauncher_t* m_appLauncher;
    pcpa_cb_table_t m_pcpCallbacks[11];
    pcpa_t m_pcp;

} MX_MASTER;

void mxmBeforeBinaryCallback(pcpa_t* stream, MX_MASTER* mxMaster);
void mxmAfterBinaryCallback(pcpa_t* stream, MX_MASTER* mxMaster);
void mxmCopySystemFile(char* filename);
int mxmBackupEventlog(int destination);
