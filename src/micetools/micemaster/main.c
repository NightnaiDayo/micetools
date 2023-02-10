#include <shlwapi.h>

#include "../lib/am/amInstall.h"
#include "../lib/ami/amiMd5.h"
#include "mxm.h"

bool mxMasterInitPcp(MX_MASTER* mxMaster) {
    if (mxMaster->m_pcpaHasInit) return true;

    e_pcpa_t err;
    pcpa_t* stream = &mxMaster->m_pcp;

    if ((err = pcpaInitStream(stream)) != e_pcpa_ok) {
        amiDebugLog("pcpaInitStream Error. Code:%d", err);
        return false;
    }

    err = pcpaSetCallbackFuncBuffer(
        stream, mxMaster->m_pcpCallbacks,
        (sizeof mxMaster->m_pcpCallbacks) / (sizeof mxMaster->m_pcpCallbacks[0]));
    if (err != e_pcpa_ok) {
        amiDebugLog("pcpaSetCallBackFuncBuffer Error. Code:%d", err);
        return false;
    }

    // Misc
    pcpaSetCallbackFunc(stream, MXM_RECONNECT_USB, mxmPcpReconnectUsbDevice, mxMaster);
    pcpaSetCallbackFunc(stream, MXM_DEVELOP, mxmPcpCheckDevelopMode, mxMaster);
    // Logs
    pcpaSetCallbackFunc(stream, MXM_LOG_AVAILALBE, mxmPcpLogAvailable, mxMaster);
    pcpaSetCallbackFunc(stream, MXM_OUTPUT_LOG, mxmPcpOutputLog, mxMaster);
    pcpaSetCallbackFunc(stream, MXM_ERASE_LOG, mxmPcpEraseLog, mxMaster);
    // Foreground control
    pcpaSetCallbackFunc(stream, MXM_FG_CURRENT, mxmPcpCurrentFgprocess, mxMaster);
    pcpaSetCallbackFunc(stream, MXM_FG_NEXT, mxmPcpNextFgprocess, mxMaster);
    pcpaSetCallbackFunc(stream, MXM_FG_ACTIVE, mxmPcpActiveFgprocess, mxMaster);
    pcpaSetCallbackFunc(stream, MXM_FG_FAULT, mxmPcpFaultFgprocess, mxMaster);
    pcpaSetCallbackFunc(stream, MXM_FG_GETCOUNT, mxmPcpGetStartCount, mxMaster);
    pcpaSetCallbackFunc(stream, MXM_FG_SETCOUNT, mxmPcpSetStartCount, mxMaster);

    mxMaster->m_pcpaHasInit = true;
    return true;
}

bool mxMasterInit(MX_MASTER* mxMaster) {
    mxMaster->m_develop = 0;
    mxMaster->m_fault = 0;
    mxMaster->m_current = 0;
    mxMaster->m_next = 0;
    mxMaster->m_changeIsSpawn = false;
    mxMaster->m_processStateChange = false;
    mxMaster->m_backupMetadataChanged = false;
    mxMaster->m_loggingAvailable = false;
    mxMaster->m_backupCount = 0;
    mxMaster->m_clearCount = 0;
    mxMaster->m_interval = 0;
    mxMaster->m_countGameBoot = 0;
    mxMaster->m_countSystemBoot = 0;
    mxMaster->m_countGameTest = 0;
    mxMaster->m_countSystemTest = 0;
    mxMaster->m_binaryMessageLen = 0;
    ZeroMemory(mxMaster->m_binaryMessage, sizeof mxMaster->m_binaryMessage);

    mxmCopySystemFile("mxsegaboot.exe");
    mxmCopySystemFile("mxauthdisc.exe");
    mxmCopySystemFile("mxshellexecute.exe");
    mxmCopySystemFile("ringmaster_pub.pem");
    mxmCopySystemFile("develop_regset.txt");
    mxmCopySystemFile("lockid.txt");
    mxmCopySystemFile("d3dref9.dll");
    mxmCopySystemFile("mxsegaboot_2052.dll");

    return mxMasterInitPcp(mxMaster);
}

bool mxMasterLoadKeychipInfo(MX_MASTER* mxMaster) {
    AM_DONGLE_STATUS err;

    amiDebugLog("Waiting for dongle setup");
    if ((err = amDongleInit()) != AM_DONGLE_STATUS_OK) {
        amiDebugLog("Error amDongleInit. Code %d", err);
        return false;
    }
    if ((err = amDongleSetAuthConfig("toolmode")) != AM_DONGLE_STATUS_OK) {
        amiDebugLog("Error amDongleSetAuthConfig. Code %d", err);
        amDongleExit();
        return false;
    }

    amtime_t start, now;
    int tDelta;
    amiTimerGet(&start);
    do {
        err = amDongleSetupKeychip();
        amiTimerGet(&now);
        tDelta = amiTimerDiffSec(&start, &now);
    } while (err == AM_DONGLE_STATUS_PENDING && tDelta < 60);
    if (err == AM_DONGLE_STATUS_PENDING) {
        amiDebugLog("Error amDongleSetupKeychip timeout");
        amDongleExit();
        return false;
    }
    if (err != AM_DONGLE_STATUS_OK) {
        amiDebugLog("Error amDongleSetupKeychip. Code %d", err);
        amDongleExit();
        return false;
    }

    if (!amDongleIsAvailable()) {
        amiDebugLog("Error amDongleIsAvailable");
        amDongleExit();
        return false;
    }

    mxMaster->m_develop = amDongleIsDevelop();

    amiTimerGet(&start);
    do {
        err = amDongleGetGameId(mxMaster->m_gameId, AM_DONGLE_BLOCK);
        if (err == AM_DONGLE_STATUS_ERR_KEYCHIP) return false;
        if (err == AM_DONGLE_STATUS_NG) return false;
        if (err == AM_DONGLE_STATUS_ERR_KEYCHIP_DATA) return false;
        if (err == AM_DONGLE_STATUS_ERR_INVALID_PARAM) return false;
        if (err == AM_DONGLE_STATUS_ERR_NO_INIT) return false;
        if (err == AM_DONGLE_STATUS_ERR_NO_SERVER) return false;

        amiTimerGet(&now);
        tDelta = amiTimerDiffSec(&start, &now);
        if (tDelta > 30) return false;
    } while (err != AM_DONGLE_STATUS_OK);

    amiTimerGet(&start);
    do {
        err = amDongleGetPlatformId(mxMaster->m_platformId, AM_DONGLE_BLOCK);
        if (err == AM_DONGLE_STATUS_ERR_KEYCHIP) return false;
        if (err == AM_DONGLE_STATUS_NG) return false;
        if (err == AM_DONGLE_STATUS_ERR_KEYCHIP_DATA) return false;
        if (err == AM_DONGLE_STATUS_ERR_INVALID_PARAM) return false;
        if (err == AM_DONGLE_STATUS_ERR_NO_INIT) return false;
        if (err == AM_DONGLE_STATUS_ERR_NO_SERVER) return false;

        amiTimerGet(&now);
        tDelta = amiTimerDiffSec(&start, &now);
        if (tDelta > 30) return false;
    } while (err != AM_DONGLE_STATUS_OK);

    amiTimerGet(&start);
    do {
        err = amDongleGetNetworkAddress((unsigned int*)&(mxMaster->m_networkAddr.S_un.S_addr),
                                        AM_DONGLE_BLOCK);
        if (err == AM_DONGLE_STATUS_ERR_KEYCHIP) return false;
        if (err == AM_DONGLE_STATUS_NG) return false;
        if (err == AM_DONGLE_STATUS_ERR_KEYCHIP_DATA) return false;
        if (err == AM_DONGLE_STATUS_ERR_INVALID_PARAM) return false;
        if (err == AM_DONGLE_STATUS_ERR_NO_INIT) return false;
        if (err == AM_DONGLE_STATUS_ERR_NO_SERVER) return false;

        amiTimerGet(&now);
        tDelta = amiTimerDiffSec(&start, &now);
        if (tDelta > 30) return false;
    } while (err != AM_DONGLE_STATUS_OK);

    amiTimerGet(&start);
    do {
        err = amDongleBillingGetKeychipId(mxMaster->m_keychipId, AM_DONGLE_BLOCK);
        if (err == AM_DONGLE_STATUS_ERR_KEYCHIP) return false;
        if (err == AM_DONGLE_STATUS_NG) return false;
        if (err == AM_DONGLE_STATUS_ERR_KEYCHIP_DATA) return false;
        if (err == AM_DONGLE_STATUS_ERR_INVALID_PARAM) return false;
        if (err == AM_DONGLE_STATUS_ERR_NO_INIT) return false;
        if (err == AM_DONGLE_STATUS_ERR_NO_SERVER) return false;

        amiTimerGet(&now);
        tDelta = amiTimerDiffSec(&start, &now);
        if (tDelta > 30) return false;
    } while (err != AM_DONGLE_STATUS_OK);

    amDongleExit();
    return true;
}

void mxMasterSysProcessesStart(MX_MASTER* mxMaster) {
    char startup[4][128];

    strcpy_s(startup[0], sizeof startup[0], Config.binary.mxkeychip);
    strcpy_s(startup[1], sizeof startup[1], Config.binary.mxnetwork);
    strcpy_s(startup[2], sizeof startup[2], Config.binary.mxstorage);
    strcpy_s(startup[3], sizeof startup[3], Config.binary.mxjvs);

    STARTUPINFOA startupInfo;
    PROCESS_INFORMATION processInformation;
    DWORD dwCreationFlags;
    BOOL ret;

    for (int i = 0; i < 4; i++) {
        dwCreationFlags = CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS;
        ZeroMemory(&startupInfo, sizeof startupInfo);
        startupInfo.cb = sizeof startupInfo;
        startupInfo.dwFlags = STARTF_USESHOWWINDOW;
        startupInfo.wShowWindow = 0;

        if (strstr(startup[i], "mxjvs.exe") != NULL) {
            WIN32_FIND_DATAA findData;

            HANDLE hFindFile = FindFirstFileA(startup[i], &findData);
            if (hFindFile == INVALID_HANDLE_VALUE) continue;
            FindClose(hFindFile);
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) continue;
            dwCreationFlags = CREATE_NO_WINDOW | HIGH_PRIORITY_CLASS;
        }

        ret = CreateProcessA(NULL, startup[i], NULL, NULL, 0, dwCreationFlags, NULL, NULL,
                             &startupInfo, &processInformation);
        if (!ret) amiDebugLog("Close Process %s", startup[i]);
        CloseHandle(processInformation.hProcess);
        CloseHandle(processInformation.hThread);
    }

    Sleep(2000);

    if (mxMasterLoadKeychipInfo(mxMaster)) {
        mxMaster->m_kcReady = true;
    } else {
        mxMaster->m_current = 1;
        mxMaster->m_kcReady = false;
    }

    char mxinstaller[128];
    sprintf_s(mxinstaller, sizeof mxinstaller, mxMaster->m_develop ? "%s -openmode any" : "%s",
              Config.binary.mxinstaller);

    startupInfo.cb = sizeof startupInfo;
    startupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = 0;
    ret = CreateProcessA(NULL, mxinstaller, NULL, NULL, 0, 0x8000020, NULL, NULL, &startupInfo,
                         &processInformation);
    if (!ret) amiDebugLog("Close Process %s", mxinstaller);

    CloseHandle(processInformation.hProcess);
    CloseHandle(processInformation.hThread);
    mxMaster->m_current = mxMaster->m_develop ? 5 : 1;
}

void mxMasterFdcProcessesStart(MX_MASTER* mxMaster) {
    char startup[3][128];

    strcpy_s(startup[0], sizeof startup[0], Config.binary.mxgcatcher);
    strcpy_s(startup[1], sizeof startup[1], Config.binary.mxgfetcher);
    strcpy_s(startup[2], sizeof startup[2], Config.binary.mxgdeliver);

    if (mxMaster->m_kcReady) {
        char* format;
        char* networkAddr = inet_ntoa(mxMaster->m_networkAddr);

        if (mxMaster->m_develop == 0) {
            format = " %s %s %s %s";
        } else {
            format = " -d %s %s %s %s";
        }

        char workBuf[128];
        sprintf_s(workBuf, sizeof workBuf, format, mxMaster->m_platformId, mxMaster->m_gameId,
                  networkAddr, mxMaster->m_keychipId);

        for (int i = 0; i < 3; i++) {
            strcat_s(startup[i], sizeof startup[i], workBuf);
        }
    }

    STARTUPINFOA startupInfo;
    PROCESS_INFORMATION processInformation;
    DWORD dwCreationFlags;
    BOOL ret;

    for (int i = 0; i < 3; i++) {
        dwCreationFlags = CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS;
        ZeroMemory(&startupInfo, sizeof startupInfo);
        startupInfo.cb = sizeof startupInfo;
        startupInfo.dwFlags = STARTF_USESHOWWINDOW;
        startupInfo.wShowWindow = 0;

        ret = CreateProcessA(NULL, startup[i], NULL, NULL, 0, dwCreationFlags, NULL, NULL,
                             &startupInfo, &processInformation);
        if (!ret) amiDebugLog("Close Process %s", startup[i]);
        CloseHandle(processInformation.hProcess);
        CloseHandle(processInformation.hThread);
    }
}

void mxMasterFirstFgProcessesStart(MX_MASTER* mxMaster) {
    appLauncherAppInfo_t appInfo;

    appInfo.m_mode = mxMaster->m_current;
    if (appInfo.m_mode == 1) {
        appInfo.m_mode = 6;
    } else if (appInfo.m_mode != 2 && appInfo.m_mode != 3 && appInfo.m_mode != 4 &&
               appInfo.m_mode != 5) {
        amiDebugLog("Error : Not Process Mode");
        mxMaster->m_current = 1;
        appInfo.m_mode = 6;
    }

    switch (appInfo.m_mode) {
        case 1:
        case 5:
        case 6:
            mxMaster->m_countSystemBoot = mxMaster->m_countSystemBoot + 1;
            break;
        case 2:
            mxMaster->m_countGameBoot = mxMaster->m_countGameBoot + 1;
            break;
        case 3:
            mxMaster->m_countSystemTest = mxMaster->m_countSystemTest + 1;
            break;
        case 4:
            mxMaster->m_countGameTest = mxMaster->m_countGameTest + 1;
            break;
    }

    appLauncherCreateThread(&appInfo, mxMaster->m_appLauncher);
}

void mxMasterMainLoop(MX_MASTER* mxMaster) {
    mxMaster->m_loggingAvailable = false;
    mxMaster->m_backupMetadataChanged = false;
    mxMaster->m_backupCount = 1;
    mxMaster->m_clearCount = 100;
    mxMaster->m_interval = 60;

    if (!MxmEventLog.m_init) {
        MxmEventLog.m_backupCount = 1;
        MxmEventLog.m_clearCount = 100;
        MxmEventLog.m_interval = 60;
        MxmEventLog.m_logDriveFound = false;
        MxmEventLog.m_filenamesSet = false;
        ZeroMemory(MxmEventLog.m_desinations, sizeof MxmEventLog.m_desinations);
        MxmEventLog.m_init = true;
    }

    e_pcpa_t err;
    do {
        err = pcpaOpenServerWithBinary(&mxMaster->m_pcp, mxMaster->m_openMode,
                                       mxMaster->m_controlPort, mxMaster->m_dataPort, 60000);
        if (err != e_pcpa_ok) {
            if (mxMaster->m_pcpaHasInit) {
                pcpaClose(&mxMaster->m_pcp);
                mxMaster->m_pcpaHasInit = false;
            }
            mxMasterInitPcp(mxMaster);
            amiDebugLog("Error : Not Open Server. Code %d", err);
        }
    } while (err != e_pcpa_ok);
    mxMaster->m_serverOpen = true;

    appLauncherAppInfo_t appInfo[3];

    while (1) {
        err = pcpaServer(&mxMaster->m_pcp, 16);
        if (err != e_pcpa_ok && err != e_pcpa_to && err != e_pcpa_closed) {
            if (mxMaster->m_pcpaHasInit) {
                pcpaClose(&mxMaster->m_pcp);
                mxMaster->m_pcpaHasInit = false;
            }
            mxMasterInitPcp(mxMaster);
            do {
                err =
                    pcpaOpenServerWithBinary(&mxMaster->m_pcp, mxMaster->m_openMode,
                                             mxMaster->m_controlPort, mxMaster->m_dataPort, 60000);
                if (err != e_pcpa_ok) {
                    if (mxMaster->m_pcpaHasInit) {
                        pcpaClose(&mxMaster->m_pcp);
                        mxMaster->m_pcpaHasInit = false;
                    }
                    mxMasterInitPcp(mxMaster);
                    amiDebugLog("Error : Not Open Server. Code %d", err);
                }
            } while (err != e_pcpa_ok);
        }

        appLauncher_t* appLauncher;
        if (mxMaster->m_processStateChange) {
            appLauncher = mxMaster->m_appLauncher;
            if (appLauncher->m_createdThread) {
                if (WaitForSingleObject(appLauncher->m_hThread, 0) != WAIT_TIMEOUT) {
                    CloseHandle(appLauncher->m_hThread);
                    appLauncher->m_hThread = NULL;
                    appLauncher->m_createdThread = false;
                }
            }

            if (!mxMaster->m_develop || mxMaster->m_changeIsSpawn) {
                appInfo[0].m_mode = mxMaster->m_current;
                mxMaster->m_changeIsSpawn = 0;
                if (appInfo[0].m_mode == 2) {
                    sprintf_s(appInfo[0].m_path, sizeof appInfo[0].m_path, "%s",
                              mxMaster->m_nextPath);
                }
                switch (appInfo[0].m_mode) {
                    case 1:
                    case 5:
                    case 6:
                        mxMaster->m_countSystemBoot++;
                        break;
                    case 2:
                        mxMaster->m_countGameBoot++;
                        break;
                    case 3:
                        mxMaster->m_countSystemTest++;
                        break;
                    case 4:
                        mxMaster->m_countGameTest++;
                }
                appLauncherCreateThread(appInfo, mxMaster->m_appLauncher);
            }
            mxMaster->m_processStateChange = false;
        }

        if (mxMaster->m_backupMetadataChanged) {
            if (MxmEventLog.m_init) {
                MxmEventLog.m_backupCount = mxMaster->m_backupCount;
                MxmEventLog.m_clearCount = mxMaster->m_clearCount;
                MxmEventLog.m_interval = mxMaster->m_interval;
            }
            mxMaster->m_backupMetadataChanged = false;
        }

        if (MxmEventLog.m_init && MxmEventLog.m_filenamesSet) {
            for (int i = 0; i < NUM_LOGS; i++) {
                if (mxmBackupEventlog(i) != 0) break;
            }
        }
    }
}

int main() {
    load_micemaster_config();

    // Enable colour
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hConsole, &dwMode))
        SetConsoleMode(hConsole, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    // Start WSA
    WSADATA wsaData;
    int err = WSAStartup(2, &wsaData);
    if (err) return -1;

    // Initialise mxm structures
    MX_MASTER* mxMaster = malloc(sizeof *mxMaster);
    if (mxMaster == NULL) {
        amiDebugLog("Unable to allocate MX_MASTER");
        return -1;
    }
    ZeroMemory(mxMaster, sizeof *mxMaster);
    appLauncher_t appLauncher;
    ZeroMemory(&appLauncher, sizeof appLauncher);
    appLauncher.m_appInfo.m_appLauncher = &appLauncher;
    mxMaster->m_appLauncher = &appLauncher;

    // Startup
    if (!mxMasterInit(mxMaster)) {
        amiDebugLog("Error mxMasterInit");
        return -1;
    }
    mxMaster->m_controlPort = 40100;
    mxMaster->m_dataPort = 40101;
    mxMaster->m_openMode = OPEN_MODE_LOCAL;

    mxMasterSysProcessesStart(mxMaster);
    mxMasterFdcProcessesStart(mxMaster);
    mxMasterFirstFgProcessesStart(mxMaster);
    mxMasterMainLoop(mxMaster);
}
