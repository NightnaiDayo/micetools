#include "mxmEventLog.h"

#include <stdio.h>

mxmEventLog_t MxmEventLog;

#define LOG_DRIVE "L:\\"
#define LOG_VOLUME "\\\\.\\L:\\"
#define LOG_DRIVE_NAME "SEGA_AM_LOG"

bool mxmEventLogGetLogDrive(char *logPath) {
    char volumeName[264];
    if (GetVolumeInformationA(LOG_VOLUME, volumeName, 264, NULL, NULL, NULL, NULL, 0)) {
        if (strcmp(volumeName, LOG_DRIVE_NAME) == 0) {
            strncpy_s(logPath, MAX_PATH, LOG_DRIVE, _TRUNCATE);
            return true;
        }
    }
    return false;
}

int mxmEventLogSetDestination(char *name, unsigned int which) {
    char *type_names[3];
    SYSTEMTIME now;

    type_names[0] = "application";
    type_names[1] = "security";
    type_names[2] = "system";

    if (name == NULL || which >= 3) return -1;

    GetLocalTime(&now);
    _snprintf_s(MxmEventLog.m_desinations[which].m_filename, MAX_PATH, _TRUNCATE,
                "%s%s%04d%02d%02d%02d%02d%02d.evt", name, type_names[which], now.wYear, now.wMonth,
                now.wDay, now.wHour, now.wMinute, now.wSecond);
    return 0;
}

void mxmEventLogInit(void) {
    if (MxmEventLog.m_filenamesSet) return;

    char name[268];
    name[0] = '\0';
    MxmEventLog.m_logDriveFound = mxmEventLogGetLogDrive(name);
    if (MxmEventLog.m_logDriveFound) {
        for (int i = 0; i < 3; i++) mxmEventLogSetDestination(name, i);
        MxmEventLog.m_filenamesSet = true;
    }
}
