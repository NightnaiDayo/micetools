#include "mxm.h"

#include <shlwapi.h>

#define TICK_MS 16

#ifdef MXM_APM2
appLauncherLaunch_t apmsTranslateNext(appLauncherLaunch_t which) {
    switch (which) {
        case appLauncherLaunchSegabootR:
            return appLauncherLaunchSegabootR;
        case appLauncherLaunchGame:
            return appLauncherLaunchGame;
        case appLauncherLaunchSegabootTR:
            return appLauncherLaunchSegabootTR;
        case appLauncherLaunchGameTest:
            return appLauncherLaunchGameTest;
        // TODO: Inversigate this. `segaboot -d` translates to `n:\\game.bat` for APM?
        case appLauncherLaunchSegabootD:
            return appLauncherLaunchAPM2;
        default:
            return 0;
    }
}

#endif

void mxmBeforeBinaryCallback(pcpa_t *stream, MX_MASTER *mxMaster) {
    pcpaSetSendBinaryBuffer(stream, mxMaster->m_binaryMessage, mxMaster->m_binaryMessageLen);
}
void mxmAfterBinaryCallback(pcpa_t *stream, MX_MASTER *mxMaster) {
    mxMaster->m_binaryMessage[0] = '\0';
    mxMaster->m_binaryMessageLen = 0;
}

void mxmCopySystemFile(char *filename) {
    char dest[256];
    char src[256];

    sprintf_s(src, sizeof src, "%s%s", Config.dir.system, filename);
    sprintf_s(dest, sizeof dest, "%s%s", Config.dir.execute, filename);

    if (!PathFileExistsA(src)) {
        amiDebugLog("Error copy %s not exist", src);
        return;
    }
    if (!PathFileExistsA(dest)) CopyFileA(src, dest, 0);
}

void mxmCopySystemFirmwareFile(char *filename) {
    char dest[256];
    char src[256];

    sprintf_s(src, sizeof src, "%s%s", Config.dir.firm, filename);
    sprintf_s(dest, sizeof dest, "%s%s", Config.dir.execute_firm, filename);

    if (!PathFileExistsA(src)) {
        amiDebugLog("Error copy %s not exist", src);
        return;
    }
    if (!PathFileExistsA(dest)) CopyFileA(src, dest, 0);
}

int mxmBackupEventlog(int destination) {
    if (!MxmEventLog.m_logDriveFound) return 0;

    char *logs[3] = {
        "Application",
        "Security",
        "System",
    };
    HANDLE hEventLog = OpenEventLogA(NULL, logs[destination]);
    if (hEventLog == INVALID_HANDLE_VALUE || hEventLog == NULL) {
        amiDebugLog("Error OpenEventLog(%d). Code %d", destination, GetLastError());
        return -2;
    }

    DWORD numberOfRecords = 0;
    if (!GetNumberOfEventLogRecords(hEventLog, &numberOfRecords)) {
        amiDebugLog("Error GetNumberOf-EventLogRecords(%d). Code %d", destination, GetLastError());
        CloseEventLog(hEventLog);
        return -3;
    }

    bool clearLog = false;
    if (MxmEventLog.m_clearCount < numberOfRecords) {
        clearLog = true;
    } else {
        amtime_t now;
        amiTimerGet(&now);
        unsigned int deltaT =
            amiTimerDiffSec(&(MxmEventLog.m_desinations[destination].m_last), &now);
        if (deltaT < MxmEventLog.m_interval) {
            printf("Waiting: %d/%d\n", deltaT, MxmEventLog.m_interval);
            CloseEventLog(hEventLog);
            return 0;
        }
    }

    if (MxmEventLog.m_backupCount <=
        (numberOfRecords - MxmEventLog.m_desinations[destination].m_numRecords)) {
        char *lpFileName = MxmEventLog.m_desinations[destination].m_filename;

        HANDLE hFile = CreateFileA(lpFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                                   FILE_SUPPORTS_REPARSE_POINTS, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(hFile);
            if (!DeleteFileA(lpFileName)) {
                amiDebugLog("Error DeleteFileA() : %d", GetLastError());
                CloseEventLog(hEventLog);
                return -4;
            }
        }

        if (!clearLog) {
            if (!BackupEventLogA(hEventLog, lpFileName)) {
                amiDebugLog("Error BackupEventLogA() : %d", GetLastError());
                CloseEventLog(hEventLog);
                return -3;
            }
            MxmEventLog.m_desinations[destination].m_numRecords = numberOfRecords;
        } else {
            if (!ClearEventLogA(hEventLog, lpFileName)) {
                amiDebugLog("Error ClearEventLog() : %d", GetLastError());
                CloseEventLog(hEventLog);
                return -3;
            }
            MxmEventLog.m_desinations[destination].m_numRecords = 0;
            char name[8];
            strncpy_s(name, sizeof name, lpFileName, 3);
            mxmEventLogSetDestination(name, destination);
        }

        amiTimerGet(&(MxmEventLog.m_desinations[destination].m_last));
    }

    CloseEventLog(hEventLog);
    return 0;
}