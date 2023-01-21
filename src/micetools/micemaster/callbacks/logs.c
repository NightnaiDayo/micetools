#include <Windows.h>
#include <winioctl.h>

#include "../../lib/am/amInstall.h"
#include "../mxm.h"
#include "mxmEventLog.h"

bool mxmEventLogAccessGetMainStorageDeviceNum(int* devNum) {
    if (devNum == NULL) {
        amiDebugLog("Error : Invalid Argument");
        return false;
    }

    char windir[256];
    if (!GetSystemWindowsDirectoryA(windir, sizeof windir)) {
        amiDebugLog("Error : GetSystemWindowsDirectoryA error");
        *devNum = -1;
        return false;
    }

    char windirDos[256];
    sprintf_s(windirDos, sizeof windirDos, "\\\\.\\%c:", windir[0]);
    HANDLE hDevice =
        CreateFileA(windirDos, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, FILE_SUPPORTS_REPARSE_POINTS, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) {
        amiDebugLog("Error : CreateFile error ErrorNum = %d", GetLastError());
        *devNum = -1;
        return false;
    }

    STORAGE_DEVICE_NUMBER ioctlBuf;
    DWORD bytesReturned;
    if (!DeviceIoControl(hDevice, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &ioctlBuf,
                         sizeof ioctlBuf, &bytesReturned, NULL)) {
        amiDebugLog("Error : DeviceIoControl error ErrorNum = %d", GetLastError());
        CloseHandle(hDevice);
        *devNum = -1;
        return false;
    }

    CloseHandle(hDevice);
    *devNum = ioctlBuf.DeviceNumber;
    return true;
}

#define LOG_OFFSET (2047 * 512)

bool mxmEventLogAccessEraseEventLogData(int deviceNum, unsigned long long address) {
    char drivePath[MAX_PATH];
    ZeroMemory(drivePath, sizeof drivePath);
    sprintf_s(drivePath, sizeof drivePath, "\\\\.\\PhysicalDrive%d", deviceNum);

    HANDLE hFile =
        CreateFileA(drivePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, FILE_SUPPORTS_REPARSE_POINTS, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        amiDebugLog("Error : CreateFile error");
        return false;
    }

    char logBuffer[512];
    ZeroMemory(logBuffer, sizeof logBuffer);

    LARGE_INTEGER writeAddress;
    writeAddress.QuadPart = address + LOG_OFFSET;
    if (!SetFilePointerEx(hFile, writeAddress, NULL, 0)) {
        CloseHandle(hFile);
        amiDebugLog("SetFilePointerEx error");
        return false;
    }

    DWORD bytesWritten;
    if (!WriteFile(hFile, logBuffer, sizeof logBuffer, &bytesWritten, NULL)) {
        CloseHandle(hFile);
        amiDebugLog("WriteFile error %d", GetLastError());
        return false;
    }
    if (bytesWritten != sizeof logBuffer) {
        CloseHandle(hFile);
        amiDebugLog("Wriete Size Error");
        return false;
    }
    CloseHandle(hFile);
    return true;
}

bool mxmEventLogAccessLoadEpbr(AM_INSTALL_BOOT_RECORD* bootRecord) {
    amtime_t start, now;

    if (amInstallInit() != AM_INSTALL_STATUS_OK) {
        amiDebugLog("Error : amInstallInit.");
        return false;
    }
    amiTimerGet(&start);
    while (1) {
        AM_INSTALL_STATUS err = amInstallGetBr(bootRecord, 0);
        if (!((err == AM_INSTALL_STATUS_ERR_REQUEST) ||
              (err == AM_INSTALL_STATUS_ERR_UPDATE_STATUS) ||
              (err == AM_INSTALL_STATUS_ERR_GET_SEMAPHORE) ||
              (err == AM_INSTALL_STATUS_ERR_FORMAT) || (err == AM_INSTALL_STATUS_OK) ||
              (err == AM_INSTALL_STATUS_BUSY))) {
            amiDebugLog("Error : amInstallGetBr error.");
            amInstallExit();
            return false;
        }

        amiTimerGet(&now);
        if (amiTimerDiffSec(&start, &now) > 30) {
            amiDebugLog("Error : amInstallGetBr time out error.");
            amInstallExit();
            return false;
        }

        if (err == AM_INSTALL_STATUS_OK) {
            if (amInstallExit() == AM_INSTALL_STATUS_OK) return true;

            amiDebugLog("Error : amInstallExit.");
            return false;
        }
    }
}

bool mxmEventLogEraseLog() {
    AM_INSTALL_BOOT_RECORD br;
    ZeroMemory(&br, sizeof br);
    int devNum = 0;
    if (!mxmEventLogAccessGetMainStorageDeviceNum(&devNum)) return false;
    if (!mxmEventLogAccessLoadEpbr(&br)) return false;
    if (!mxmEventLogAccessEraseEventLogData(devNum, br.os)) return false;
    return true;
}

void mxmPcpLogAvailable(pcpa_t* stream, MX_MASTER* mxMaster) {
    pcpaSetSendPacket(stream, MXM_LOG_AVAILALBE, mxMaster->m_loggingAvailable ? "1" : "0");

    char* command = pcpaGetCommand(stream, MXM_LOG_AVAILALBE);
    if (command == NULL) {
        amiDebugLog("Error : Command is not set up.");
        pcpaSetSendPacket(stream, MXM_LOG_AVAILALBE, "?");
        return;
    }
    bool metadataChanged = false;
    if (strcmp(command, "?") != 0) {
        if (strcmp(command, "1") == 0) {
            mxMaster->m_loggingAvailable = true;
            mxmEventLogInit();
        } else if (strcmp(command, "0") == 0) {
            mxMaster->m_loggingAvailable = false;
            if (MxmEventLog.m_filenamesSet) {
                MxmEventLog.m_filenamesSet = false;
                MxmEventLog.m_logDriveFound = false;
            }
        } else {
            pcpaSetSendPacket(stream, MXM_LOG_AVAILALBE, "?");
            pcpaAddSendPacket(stream, "code", "1");
            return;
        }

        command = pcpaGetCommand(stream, "backup_count");
        if (command != NULL) {
            mxMaster->m_backupCount = strtol(command, NULL, 10);
            metadataChanged = true;
        }
        command = pcpaGetCommand(stream, "clear_count");
        if (command != NULL) {
            mxMaster->m_clearCount = strtol(command, NULL, 10);
            metadataChanged = true;
        }
        command = pcpaGetCommand(stream, "interval");
        if (command != NULL) {
            mxMaster->m_interval = strtol(command, NULL, 10);
            metadataChanged = true;
        }
        mxMaster->m_backupMetadataChanged = metadataChanged;
    }

    char buffer[256];
    pcpaSetSendPacket(stream, MXM_LOG_AVAILALBE, mxMaster->m_loggingAvailable ? "1" : 0);

    _itoa_s(mxMaster->m_backupCount, buffer, sizeof buffer, 10);
    pcpaAddSendPacket(stream, "backup_count", buffer);
    _itoa_s(mxMaster->m_clearCount, buffer, sizeof buffer, 10);
    pcpaAddSendPacket(stream, "clear_count", buffer);
    _itoa_s(mxMaster->m_interval, buffer, sizeof buffer, 10);
    pcpaAddSendPacket(stream, "interval", buffer);
}

void mxmPcpOutputLog(pcpa_t* stream, MX_MASTER* mxMaster) {
    // Only allowed: 0, 1, 2
    pcpaSetSendPacket(stream, MXM_OUTPUT_LOG, "0");
    pcpaAddSendPacket(stream, "path", "");
}

void mxmPcpEraseLog(pcpa_t* stream, MX_MASTER* mxMaster) {
    char* command = pcpaGetCommand(stream, MXM_ERASE_LOG);
    if (command == NULL) {
        amiDebugLog("Error : Command error");
        pcpaSetSendPacket(stream, MXM_ERASE_LOG, "1");
        return;
    }

    if (strcmp(command, "0") == 0) {
        if (!mxmEventLogEraseLog()) {
            amiDebugLog("Error : EraseLog error");
            pcpaSetSendPacket(stream, MXM_ERASE_LOG, "1");
        } else {
            pcpaSetSendPacket(stream, MXM_ERASE_LOG, "0");
        }
    } else {
        amiDebugLog("Error : Command error");
        pcpaSetSendPacket(stream, MXM_ERASE_LOG, "1");
    }
}
