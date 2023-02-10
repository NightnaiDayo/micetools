#include "mxmEventLog.h"

#include <Windows.h>
#include <stdio.h>
#include <winioctl.h>

#include "../lib/ami/amiMd5.h"

mxmEventLog_t MxmEventLog;

#define LOG_DRIVE "L:\\"
#define LOG_VOLUME "\\\\.\\L:\\"
#define LOG_DRIVE_NAME "SEGA_AM_LOG"
#define TEMP_PATH "C:\\WINDOWS\\TEMP\\"

bool mxmEventLogGetTempPath(char *name, char *buffer) {
    SYSTEMTIME now;

    GetLocalTime(&now);
    _snprintf_s(buffer, 0x104, 0xffffffff, "%s%s%04d%02d%02d%02d%02d%02d.evt", TEMP_PATH, name,
                now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond);
    return true;
}

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
    char *type_names[NUM_LOGS];
    SYSTEMTIME now;

    type_names[0] = "application";
    type_names[1] = "security";
    type_names[2] = "system";

    if (name == NULL || which >= NUM_LOGS) return -1;

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
        for (int i = 0; i < NUM_LOGS; i++) mxmEventLogSetDestination(name, i);
        MxmEventLog.m_filenamesSet = true;
    }
}

bool mxmEventLogEraseLog(void) {
    AM_INSTALL_BOOT_RECORD br;
    ZeroMemory(&br, sizeof br);
    int devNum = 0;
    if (!mxmEventLogAccessGetMainStorageDeviceNum(&devNum)) return false;
    if (!mxmEventLogAccessLoadEpbr(&br)) return false;
    if (!mxmEventLogAccessEraseEventLogData(devNum, br.os)) return false;
    return true;
}

bool mxmEventLogCheckLog(bool *valid) {
    if (valid == NULL) return false;

    AM_INSTALL_BOOT_RECORD br;
    ZeroMemory(&br, sizeof br);
    int devNum = 0;

    if (!mxmEventLogAccessGetMainStorageDeviceNum(&devNum)) return false;
    if (!mxmEventLogAccessLoadEpbr(&br)) return false;
    if (!mxmEventLogAccessCheckEventLogData(devNum, br.os, valid)) return false;
    return true;
}

bool mxmEventLogAccessGetMainStorageDeviceNum(int *devNum) {
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
    if (!SetFilePointerEx(hFile, writeAddress, NULL, FILE_BEGIN)) {
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

bool mxmEventLogAccessCheckEventLogData(int deviceNum, unsigned long long address, bool *valid) {
    AM_EVENT_LOG_HEADER logHeader;
    char drivePath[MAX_PATH];
    ZeroMemory(&logHeader, sizeof logHeader);
    ZeroMemory(drivePath, sizeof drivePath);

    sprintf_s(drivePath, sizeof drivePath, "\\\\.\\PhysicalDrive%d", deviceNum);

    HANDLE hFile =
        CreateFileA(drivePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, FILE_SUPPORTS_REPARSE_POINTS, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        amiDebugLog("Error : CreateFile error");
        return false;
    }

    LARGE_INTEGER writeAddress;
    writeAddress.QuadPart = address + LOG_OFFSET;
    if (!SetFilePointerEx(hFile, writeAddress, NULL, FILE_BEGIN)) {
        CloseHandle(hFile);
        amiDebugLog("SetFilePointerEx error");
        return false;
    }

    DWORD bytesRead;
    if (!ReadFile(hFile, &logHeader, sizeof logHeader, &bytesRead, NULL)) {
        CloseHandle(hFile);
        amiDebugLog("ReadFile Header error");
        return false;
    }
    if (bytesRead != sizeof logHeader) {
        CloseHandle(hFile);
        amiDebugLog("Read Size Error");
        return false;
    }

    amiCrc32RInit();
    unsigned int calcCrc =
        amiCrc32RCalc(sizeof logHeader - 4, (unsigned char *)(&logHeader) + 4, 0);
    *valid = logHeader.m_crc32 == calcCrc;
    CloseHandle(hFile);
    return true;
}

bool mxmEventLogAccessLoadEpbr(AM_INSTALL_BOOT_RECORD *bootRecord) {
    amtime_t start, now;

    AM_INSTALL_STATUS err = amInstallInit();
    if (err != AM_INSTALL_STATUS_OK && err != AM_INSTALL_STATUS_ERR_ALREADY_INIT) {
        amiDebugLog("Error : amInstallInit. Code %d", err);
        return false;
    }
    amiTimerGet(&start);
    while (1) {
        err = amInstallGetBr(bootRecord, 0);
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

bool mxmEventLogAccessReadEventLogData(int deviceNum, unsigned long long address,
                                       unsigned char *eventlog1, unsigned int *nBytes1,
                                       unsigned char *eventlog2, unsigned int *nBytes2) {
    if (nBytes1 == 0 || eventlog2 == NULL || nBytes2 == NULL) return false;

    AM_EVENT_LOG_HEADER eventLogHeader;
    ZeroMemory(&eventLogHeader, sizeof eventLogHeader);

    char drivePath[256];
    ZeroMemory(&drivePath, sizeof drivePath);
    sprintf_s(drivePath, sizeof drivePath, "\\\\.\\PhysicalDrive%d", deviceNum);

    HANDLE hFile =
        CreateFileA(drivePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, FILE_SUPPORTS_REPARSE_POINTS, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        amiDebugLog("Error : CreateFile error");
        return false;
    }

    LARGE_INTEGER readAddress;
    readAddress.QuadPart = address + LOG_OFFSET;
    if (!SetFilePointerEx(hFile, readAddress, NULL, 0)) {
        CloseHandle(hFile);
        amiDebugLog("SetFilePointerEx error");
        return false;
    }

    DWORD bytesRead;
    if (!ReadFile(hFile, &eventLogHeader, sizeof eventLogHeader, &bytesRead, NULL)) {
        CloseHandle(hFile);
        amiDebugLog("ReadFile Header error");
        return false;
    }
    if (bytesRead != sizeof eventLogHeader) {
        CloseHandle(hFile);
        amiDebugLog("Read Size error");
        return false;
    }

    amiCrc32RInit();
    unsigned int calcCrc =
        amiCrc32RCalc(sizeof eventLogHeader - 4, (unsigned char *)(&eventLogHeader) + 4, 0);

    if (calcCrc != eventLogHeader.m_crc32) {
        CloseHandle(hFile);
        return false;
    }

    if (eventLogHeader.m_nBytes[0] > LOG_MAX_SIZE || eventLogHeader.m_nBytes[1] > LOG_MAX_SIZE) {
        CloseHandle(hFile);
        amiDebugLog("Buffer Size error");
        return false;
    }

    unsigned char *readBuffer = malloc(LOG_MAX_SIZE);
    if (readBuffer == NULL) {
        CloseHandle(hFile);
        return false;
    }
    ZeroMemory(readBuffer, LOG_MAX_SIZE);

    struct {
        unsigned long long offset;
        unsigned int srcSize;
        unsigned int dstSize;
        unsigned char *dst;
    } readSetup[2] = {
        {
            .offset = LOG_1_OFFSET,
            .dstSize = LOG_MAX_SIZE,
            .dst = eventlog1,
        },
        {
            .offset = LOG_2_OFFSET,
            .dstSize = LOG_MAX_SIZE,
            .dst = eventlog2,
        },
    };

    for (int i = 0; i < 2; i++) {
        readAddress.QuadPart = address + readSetup[i].offset;
        if (!SetFilePointerEx(hFile, readAddress, NULL, 0)) {
            free(readBuffer);
            CloseHandle(hFile);
            amiDebugLog("SetFilePointerEx error");
            return false;
        }

        DWORD nRead = eventLogHeader.m_nBytes[i];
        if ((nRead & 0x1FF) != 0) nRead += 512 - (nRead & 0x1FF);

        if (!ReadFile(hFile, readBuffer, nRead, &bytesRead, NULL)) {
            free(readBuffer);
            CloseHandle(hFile);
            amiDebugLog("ReadFile Evt error");
            return false;
        }
        if (nRead != bytesRead) {
            free(readBuffer);
            CloseHandle(hFile);
            amiDebugLog("Read Size error");
            return false;
        }

        AmiMd5 md5;
        amiMd5Init(&md5);
        amiMd5Update(&md5, nRead, readBuffer);
        amiMd5Finalise(&md5);

        if (memcmp(md5.m_digest, eventLogHeader.m_md5[i], sizeof md5.m_digest) != 0) {
            free(readBuffer);
            CloseHandle(hFile);
            amiDebugLog("Event Log is broken");
            return false;
        }

        memcpy(readSetup[i].dst, readBuffer, readSetup[i].dstSize);
    }

    free(readBuffer);
    CloseHandle(hFile);
    return true;
}

DWORD mxmEventLogAccessWriteDataFile(char *filename, unsigned char *dataFile, unsigned int nBytes) {
    HANDLE hFile = CreateFileA(filename, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                               CREATE_ALWAYS, FILE_SUPPORTS_REPARSE_POINTS, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        amiDebugLog("Error : CreateFile error");
        return (DWORD)-1;
    }

    DWORD ret = (DWORD)-1;
    DWORD bytesRead;
    if (WriteFile(hFile, dataFile, nBytes, &bytesRead, NULL)) {
        ret = bytesRead;
    }
    CloseHandle(hFile);
    return ret;
}

bool mxmEventLogAccessCopyIntoDataFile(int deviceNum, unsigned long long address, char *filename,
                                       unsigned int type) {
    unsigned int nBytes1;
    unsigned char *eventLog1 = malloc(LOG_MAX_SIZE);
    if (eventLog1 == NULL) return false;
    unsigned int nBytes2;
    unsigned char *eventLog2 = malloc(LOG_MAX_SIZE);
    if (eventLog2 == NULL) {
        free(eventLog1);
        return false;
    }

    if (!mxmEventLogAccessReadEventLogData(deviceNum, address, eventLog1, &nBytes1, eventLog2,
                                           &nBytes2)) {
        free(eventLog1);
        free(eventLog2);
        return false;
    }

    bool success;
    if (type == 0) {
        success = mxmEventLogAccessWriteDataFile(filename, eventLog2, nBytes1);
    } else if (type == 1) {
        success = mxmEventLogAccessWriteDataFile(filename, eventLog2, nBytes2);
    } else {
        free(eventLog1);
        free(eventLog2);
        return false;
    }

    free(eventLog1);
    free(eventLog2);
    if (success == (DWORD)-1) return false;
    return true;
}

bool mxmEventLogAccessBackup(char *filename, int type) {
    AM_INSTALL_BOOT_RECORD br;
    ZeroMemory(&br, sizeof br);
    int devNum = 0;
    if (!mxmEventLogAccessGetMainStorageDeviceNum(&devNum)) return false;
    if (!mxmEventLogAccessLoadEpbr(&br)) return false;
    if (!mxmEventLogAccessCopyIntoDataFile(devNum, br.os, filename, type)) return false;
    return true;
}
