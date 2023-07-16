#include "osu.h"

#include <stdio.h>

#include "../lib/am/amCmos.h"
#include "../lib/am/amPlatform.h"

int osuExecProcess(LPSTR command) {
    PROCESS_INFORMATION processInformation = { 0 };
    STARTUPINFOA startupInfo = {
        .cb = 68,
        .dwFlags = STARTF_USESHOWWINDOW,
        .wShowWindow = SW_HIDE,
    };

    if (command == NULL) return -150;

    BOOL spanwed = CreateProcessA(NULL, command, NULL, NULL, 0, 0, NULL, NULL, &startupInfo,
                                  &processInformation);
    DWORD wait = WaitForSingleObject(processInformation.hProcess, INFINITE);

    int ret;
    if (!spanwed) {
        ret = -140;
    } else if (wait == WAIT_OBJECT_0) {
        DWORD exitCode;
        GetExitCodeProcess(processInformation.hProcess, &exitCode);
        ret = exitCode == 0 ? 0 : -141;
    } else {
        ret = -141;
    }

    CloseHandle(processInformation.hProcess);
    CloseHandle(processInformation.hThread);

    return ret;
}

int osuMountPartition(LPCSTR lpTargetPath, LPCSTR lpszVolumeMountPoint) {
    if (lpTargetPath == NULL || lpszVolumeMountPoint == NULL) return -150;

    DefineDosDeviceA(DDD_RAW_TARGET_PATH, "B:", lpTargetPath);
    char szVolumeName[256];
    GetVolumeNameForVolumeMountPointA("B:\\", szVolumeName, sizeof szVolumeName);
    DefineDosDeviceA(DDD_RAW_TARGET_PATH | DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE,
                     "B:", lpTargetPath);

    if (!SetVolumeMountPointA(lpszVolumeMountPoint, szVolumeName)) return -129;

    for (int i = 0; i < 100; i++)
        if (DefineDosDeviceA(DDD_RAW_TARGET_PATH | DDD_REMOVE_DEFINITION, "B:", NULL)) break;

    char szDeviceName[260];
    sprintf_s(szDeviceName, sizeof szDeviceName, "%c:", lpszVolumeMountPoint[4]);

    if (!DefineDosDeviceA(DDD_RAW_TARGET_PATH, szDeviceName, lpTargetPath)) return -130;
    return 0;
}

int osuOpenDevice(DWORD dwDrive, LPHANDLE lphDevice) {
    char szFileName[32];
    sprintf_s(szFileName, sizeof szFileName, "\\\\.\\PhysicalDrive%d", dwDrive);

    HANDLE hDevice = *lphDevice = CreateFileA(szFileName, 0xc0000000, 3, NULL, 3, 0x80, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) return -123;
    return 0;
}

int osuReadSector(HANDLE hDevice, DWORD dwLBA, DWORD dwSectors, LPVOID lpBuffer) {
    LARGE_INTEGER liPointer;
    liPointer.QuadPart = (unsigned long long)dwLBA * 0x200;
    if (!SetFilePointerEx(hDevice, liPointer, NULL, FILE_BEGIN)) return -124;
    if (lpBuffer == NULL) return -150;

    DWORD nNumberOfBytesToRead = dwSectors * 0x200;
    DWORD nRead = 0;
    if (!ReadFile(hDevice, lpBuffer, nNumberOfBytesToRead, &nRead, NULL)) return -125;
    if (nRead != nNumberOfBytesToRead || ((dwSectors * 0x200) >> 32) != 0) return -126;
    return 0;
}

int osuSystemReboot(void) {
    int eCode = 0;

    HANDLE hToken = INVALID_HANDLE_VALUE;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        eCode = -142;
        goto _end;
    }
    TOKEN_PRIVILEGES privileges;
    if (!LookupPrivilegeValueA(NULL, "SeShutdownPrivilege", &privileges.Privileges[0].Luid)) {
        eCode = -143;
        goto _end;
    }
    privileges.PrivilegeCount = 1;
    privileges.Privileges[0].Attributes = 2;
    if (!AdjustTokenPrivileges(hToken, 0, &privileges, 0, NULL, NULL)) {
        eCode = -144;
        goto _end;
    }
    if (!ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0)) {
        eCode = -145;
        goto _end;
    }

_end:
    if (hToken != INVALID_HANDLE_VALUE) CloseHandle(hToken);
    return eCode;
}

int osuSetActivePartition(BYTE partition) {
    AM_PLATFORM_BOARD_TYPE board_type;

    if (amPlatformGetBoardType(&board_type) != AM_PLATFORM_STATUS_OK) return -110;

    AM_PLATFORM_CMOS_PARAM* lpParam = amPlatformGetCmosParam(board_type);
    if (amCmosInit(lpParam) != 0) return -114;

    if (amCmosSetPartition_0(partition) != 0) {
        amCmosExit();
        return -111;
    }

    amCmosExit();
    return 0;
}
