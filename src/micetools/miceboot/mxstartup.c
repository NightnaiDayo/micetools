#include <Windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../amBackupStructs.h"
#include "../lib/am/amBackup.h"
#include "../lib/am/amEeprom.h"
#include "../lib/am/amLog.h"
#include "../lib/am/amPlatform.h"
#include "../lib/ami/amiCrc.h"
#include "../lib/ami/amiDebug.h"
#include "ewfapi.h"
#include "osg.h"
#include "osu.h"

typedef enum {
    MxError_Unknown = -1,
    MxError_None = 0,

    MxError_SecurityCheck = 500,
    MxError_Ewf = 501,
    MxError_Version = 502,
    MxError_NoKeyFile = 503,
    MxError_DiskAccessPrivilege = 504,
    MxError_SystemMount = 505,
    MxError_MountUpdate = 506,
    MxError_MxMaster = 507,
    MxError_OSUpdateKeyFile = 508,
    MxError_ExecuteOSUpdate = 509,
    MxError_EWFEnable = 510,
    MxError_SBR = 511,
    MxError_SysKeyDelete = 512,
    MxError_MountDriver = 513,
    MxError_RingRegistry = 514,

    MxError_UnMountVolume = 540,
    MxError_MountRecovery = 541,
    MxError_ChangeActivePartition = 543,
    MxError_Reboot = 545,
    MxError_GetCD = 546,
} MxError_t;
typedef enum {
    Init = 0,
    CheckEwf = 1,
    EwfDisabled = 2,
    GetSBR = 3,
    NormalBoot = 4,
    SystemUpdate2 = 5,
    SystemUpdate3 = 6,
    Done = 7,
} StartupStep_t;

#define VERSION_BUFFER 256

#define VERSION_FILE "C:\\System\\SystemVersion.txt"
#define DEFAULT_VERSION_FILE "C:\\System\\DefaultSystemVersion.txt"

#define SYSTEM_KEY_FILE "Z:\\SystemKeyFile"
#define OS_UPDATE_KEY_FILE "Z:\\UpdateKeyFile"
#define SYSTEM_VOLUME "C:\\System\\Execute\\System"
#define DEFAULT_DRIVERS "Z:\\Minint\\System32\\DEFAULT_DRIVERS"
#define SYSTEM_MOUNT "S:"
#define UPDATE_MOUNT "W:"
#define UPDATE_MOUNT_SLASH "W:\\"
#define DRIVERS_MOUNT "V:"
#define DRIVERS_MOUNT_SLASH "V:\\"
#define OS_UPDATE_CONF "W:\\OSupdate2.conf"
#define DRIVERS_UPDATE_EXE "V:\\MXOSUpdate.exe"
#define UPDATE_EXE "W:\\MXOSUpdate.exe"

#define NEXT_HOP_CWD SYSTEM_MOUNT "\\"
#define NEXT_HOP_MXMASTER NEXT_HOP_CWD "mxmaster.exe"

#define CACLS_REMPERM_EXECUTE "cacls c:\\system\\execute /t /e /p AppUser:N"
#define CACLS_REMPERM_Z "cacls z:\\ /t /e /p appuser:N"
#define CACLS_REMPERM_S "cacls s:\\ /t /e /p appuser:N"

#define TRUECRYPT "TrueCrypt"

// Rather than implement these sums in code, we're just going to use the
// presummed versions, because we have nothing to gain by obfuscating this.
// These two sum to TC_PASSWORD
#define TC_PASSWORD_1 "3d550a335d2557055d2b3d1444153348"
#define TC_PASSWORD_2 "36105d2e0b3c1b5f1336365f335a3f1c"
#define TC_PASSWORD "segahardpassword"
// These two sum to SYSTEM_KEYFILE_ADS
#define SYSTEM_KEYFILE_1 "041443255b204a3f054d1b4c352a22292f211b29440e1237591f2e49375d093967614d"
#define SYSTEM_KEYFILE_2 "3f26192e1e532a26680f2a2c3039534b363b2923082c41421a5537241408700d020b18"
#define SYSTEM_KEYFILE_ADS "C:\\System\\Execute\\DLL:SystemKeyFile"
// These two sum to OS_UPDATE_ADS
#define OS_UPDATE_KEYFILE_1 "4019582c56283006300b40042110143d603b3d3b1d012b5c0d5423152235163f624a43"
#define OS_UPDATE_KEYFILE_2 "03210427234b445f3d51057444536137052107112f392a14570d515029306307072222"
#define OS_UPDATE_KEYFILE_ADS "C:\\System\\Execute\\DLL:UpdateKeyFile"

int LOG_EN_PLATFORM = 0;

void log(char* function, unsigned int line_number, char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("%s: Line%d  ", function, line_number);
    vprintf(fmt, args);
    va_end(args);
}

BOOL isDebugEnabled(void) {
    WIN32_FIND_DATAA find;
    HANDLE hFind;

    hFind = FindFirstFileA(".\\DebugEnable", &find);
    if (hFind != INVALID_HANDLE_VALUE) {
        FindClose(hFind);
        return TRUE;
    }
    FindClose(hFind);
    return FALSE;
}

colour GetBGColour(void) {
    HKEY hkResult;
    LSTATUS status;
    status =
        RegOpenKeyExA(HKEY_USERS, ".Default\\Control Panel\\Colors", 0, KEY_ALL_ACCESS, &hkResult);
    if (status != ERROR_SUCCESS) return White;

    BYTE data[16];
    DWORD cbData = sizeof data;
    status = RegQueryValueExA(hkResult, "BackGround", NULL, NULL, data, &cbData);
    if (status == ERROR_SUCCESS) {
        RegCloseKey(hkResult);
        return (memcmp(data, "100 100 100", 13) == 0) ? Grey : White;
    } else {
        RegCloseKey(hkResult);
        return White;
    }
}

OSU_TOP g_Top;
OSU_BOTTOM g_Bottom;
OSU g_Osu;

#define LINE5_LEN 32
bool InitGraphics(HINSTANCE hInstance, int nShowCmd) {
    g_Top.show_top = 0;
    g_Top.debugEnable = isDebugEnabled();
    g_Top.top_text_colour = White;
    g_Top.top_text_font = 2;
    g_Top.str1 = "SYSTEM  UPDATE";
    g_Bottom.backgroundColour = GetBGColour();
    colour bg_invert = g_Bottom.backgroundColour == White ? Black : White;
    g_Bottom.line5_colour = bg_invert;
    g_Bottom.power_colour = bg_invert;
    g_Bottom.status_colour = bg_invert;
    g_Bottom.status_font = 2;
    g_Bottom.status_str = "NOW LOADING";
    g_Bottom.status_use_counter = 1;
    g_Bottom.str2 = "";
    static char line5[LINE5_LEN];
    line5[0] = '\0';
    g_Bottom.line5 = line5;
    g_Bottom.hide_power = 0;
    g_Bottom.power_font = 0;
    g_Osu.hInstance = hInstance;
    g_Osu.nShowCmd = nShowCmd;
    g_Osu.s1 = &g_Top;
    g_Osu.s2 = &g_Bottom;

    BOOL ret = osgInitDraw(&g_Osu);
    if (!ret) amiDebugLog("Error : osgInitDraw error");
    return ret != 0;
}

void AppendAsterisk(void) {
    EnterCriticalSection(&g_Top.criticalSection);
    strcat_s(g_Bottom.line5, LINE5_LEN, "*");
    LeaveCriticalSection(&g_Top.criticalSection);
}
void setStrForStep2(void) {
    EnterCriticalSection(&g_Top.criticalSection);
    // TODO: Wtf was this line?
    //   FUN_00402c40((basic_string<char,struct_std::char_traits<char>,class_std::allocator<char>_>
    //   *)
    //                &g_Bottom.line5);
    g_Top.show_top = 1;
    g_Top.str2 = "STEP  2";
    g_Bottom.status_font = 0;
    g_Bottom.status_str = "";
    g_Bottom.status_use_counter = 0;
    g_Bottom.str2 = "DO NOT TURN OFF THE POWER.";
    g_Bottom.hide_power = 1;
    g_Bottom.power_font = 0;
    LeaveCriticalSection(&g_Top.criticalSection);
}
void setStrForStep3(void) {
    EnterCriticalSection(&g_Top.criticalSection);
    // TODO: Wtf was this line?
    //   FUN_00402c40((basic_string<char,struct_std::char_traits<char>,class_std::allocator<char>_>
    //   *)
    //                &g_Bottom.line5);
    g_Top.show_top = 1;
    g_Top.str2 = "STEP  3";
    g_Bottom.status_font = 0;
    g_Bottom.status_str = "";
    g_Bottom.status_use_counter = 0;
    g_Bottom.str2 = "DO NOT TURN OFF THE POWER.";
    g_Bottom.hide_power = 1;
    g_Bottom.power_font = 0;
    LeaveCriticalSection(&g_Top.criticalSection);
}

/* Read a version from a text file */
bool GetVersionText(char* version, char* path) {
    char buffer[VERSION_BUFFER];
    FILE* file;

    memset(buffer, 0, sizeof buffer);
    if (fopen_s(&file, path, "r") || file == NULL) {
        amiDebugLog("Error : GetVersionText error path = %s", path);
        return false;
    }

    fgets(buffer, sizeof buffer, file);
    fclose(file);
    strncpy_s(version, VERSION_BUFFER, buffer, VERSION_BUFFER);
    return true;
}
/* Get the currently running version of the system */
bool GetCurrentVersion(char* version) { return GetVersionText(version, VERSION_FILE); }
/* Get the default version of the system */
bool GetDefaultVersion(char* version) { return GetVersionText(version, DEFAULT_VERSION_FILE); }

/* Mount the system volume */
bool MountSystemVolume(void) {
    char command[256];
    snprintf(command, sizeof command, TRUECRYPT " /p %s /k %s /v %s /l %s /w /s /q", TC_PASSWORD,
             SYSTEM_KEY_FILE, SYSTEM_VOLUME, SYSTEM_MOUNT);

    int ret = osuExecProcess(command);
    if (ret == 0) return true;
    amiDebugLog("Error : osuExecProcess error. ret = %d", ret);
    return false;
}
/* Try to mount the system volume with up to 100 retries (20 seconds) */
bool TryMountSystemVolume(void) {
    for (int i = 0; i < 100; i++) {
        if (MountSystemVolume()) return true;
        Sleep(200);
    }
    return false;
}

/* Copies the file at src to dst */
bool GetKeyFile(LPCSTR pszSrc, LPCSTR pszDst) {
    HANDLE hSrc = CreateFileA(pszSrc, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hSrc == INVALID_HANDLE_VALUE) {
        amiDebugLog("Error : CreateFile Src error");
        return false;
    }

    HANDLE hDst =
        CreateFileA(pszDst, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDst == INVALID_HANDLE_VALUE) {
        CloseHandle(hSrc);
        amiDebugLog("Error : CreateFile Dst error");
        return false;
    }

    CHAR readBuffer[260];
    DWORD nRead;
    if (!ReadFile(hSrc, readBuffer, 255, &nRead, NULL)) {
        CloseHandle(hSrc);
        CloseHandle(hDst);
        amiDebugLog("Error : ReadFile error");
        return false;
    }
    readBuffer[nRead] = '\0';

    DWORD written;
    bool ret = WriteFile(hDst, readBuffer, strlen(readBuffer), &written, NULL) != 0;
    if (!ret) amiDebugLog("Error : WriteFile error");
    CloseHandle(hSrc);
    CloseHandle(hDst);
    return ret;
}
void DeleteMounteDevices(void) {
    if (RegDeleteKeyA(HKEY_LOCAL_MACHINE, "SYSTEM\\MountedDevices") != ERROR_SUCCESS)
        amiDebugLog("Error : RegDeleteKeyA failed.");
}

bool GetSystemKeyFile() { return GetKeyFile(SYSTEM_KEYFILE_ADS, SYSTEM_KEY_FILE); };
bool GetOSUpdateKeyFile() { return GetKeyFile(OS_UPDATE_KEYFILE_ADS, OS_UPDATE_KEY_FILE); };
bool SetDiskAccessPrivilege() {
    int i;
    for (i = 0; i < 100; i++) {
        if (osuExecProcess(CACLS_REMPERM_EXECUTE) != -140) break;
        Sleep(200);
    }
    if (i == 100) {
        amiDebugLog("Error : osuExecProcess error");
        return false;
    }

    for (i = 0; i < 100; i++) {
        if (osuExecProcess(CACLS_REMPERM_Z) != -140) break;
        Sleep(200);
    }
    if (i == 100) {
        amiDebugLog("Error : osuExecProcess error");
        return false;
    }

    for (i = 0; i < 100; i++) {
        if (osuExecProcess(CACLS_REMPERM_S) != -140) break;
        Sleep(200);
    }
    if (i == 100) {
        amiDebugLog("Error : osuExecProcess error");
        return false;
    }

    return true;
};
bool ExecuteMxMaster() {
    HINSTANCE ret = ShellExecuteA(NULL, NULL, NEXT_HOP_MXMASTER, NULL, NEXT_HOP_CWD, 0);
    if ((int)ret < 33) {
        amiDebugLog("Error : ShellExecute ret = %d", ret);
        return false;
    }
    return true;
}
BOOL SecurityCheck(void) {
    AM_PLATFORM_BOARD_TYPE board_type;
    if (amPlatformGetBoardType(&board_type) != AM_PLATFORM_STATUS_OK) {
        amiDebugLog("Error : amPlatformGetBoardType error");
        return FALSE;
    }

    if (board_type == AM_PLATFORM_BOARD_TYPE_UNKNOWN) {
        amiDebugLog("Error : BoardType is unknown.");
        return FALSE;
    }

    return TRUE;
}

BOOL UnMountVolume(void) {
    CHAR mount[4];
    mount[1] = ':';
    mount[2] = '\\';
    mount[3] = '\0';
    for (char i = 'D'; i <= 'Z'; i++) {
        mount[0] = i;
        DeleteVolumeMountPointA(mount);
    }
    return TRUE;
}

bool MountRecoveryVolume(DWORD dwDisk) {
    char targetPath[256];

    sprintf_s(targetPath, sizeof targetPath, "\\device\\harddisk%d\\partition2", dwDisk);
    if (osuMountPartition(targetPath, "\\\\.\\Z:\\") != 0) {
        amiDebugLog("Error : osuMountPartition error");
        return FALSE;
    }
    return TRUE;
}

BOOL GetSystemDiskNumber(DWORD* disk_number, DWORD* partition_no) {
    wchar_t windir_path[128];
    UINT UVar1 = GetWindowsDirectoryW(windir_path, _countof(windir_path));
    if (UVar1 >= 0x100) return FALSE;

    wchar_t* pwVar2 = wcschr(windir_path, L'\\');
    if (pwVar2 == NULL) return FALSE;
    *pwVar2 = L'\0';

    wchar_t c_drive_filename[16];
    c_drive_filename[0] = L'\0';

    errno_t e;
    e = wcscat_s(c_drive_filename, _countof(c_drive_filename), L"\\\\.\\");
    if (e < 0) return FALSE;

    e = wcscat_s(c_drive_filename, _countof(c_drive_filename), windir_path);
    if (e < 0) return FALSE;

    HANDLE hCDevice = CreateFileW(c_drive_filename, 0xc0000000, 3, NULL, 3, 0x80, NULL);
    if (hCDevice == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "CreateFileW Failed.\n");
        return FALSE;
    }

    DWORD dwBytesReturned;
    STORAGE_DEVICE_NUMBER device;
    BYTE extentsBuffer[0x110];
    VOLUME_DISK_EXTENTS* lpExtents = (VOLUME_DISK_EXTENTS*)&extentsBuffer;
    BOOL res;
    res = DeviceIoControl(hCDevice, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &device,
                          sizeof device, &dwBytesReturned, NULL);
    if (!res || device.DeviceType != FILE_DEVICE_DISK) {
        CloseHandle(hCDevice);
        return FALSE;
    }

    res = DeviceIoControl(hCDevice, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, &extentsBuffer,
                          sizeof extentsBuffer, &dwBytesReturned, NULL);
    if (!res || lpExtents->NumberOfDiskExtents > 1) {
        CloseHandle(hCDevice);
        return FALSE;
    }

    *disk_number = lpExtents->Extents[0].StartingOffset.LowPart;
    *partition_no = device.PartitionNumber;
    CloseHandle(hCDevice);
    return TRUE;
}

MxError_t MxInitStartup(DWORD* diskNumber) {
    AppendAsterisk();
    if (!SecurityCheck()) {
        amiDebugLog("Error : SecurityCheck error");
        return MxError_SecurityCheck;
    }

    AppendAsterisk();
    if (!UnMountVolume()) {
        amiDebugLog("Error : Unmount error");
        return MxError_UnMountVolume;
    }

    AppendAsterisk();
    DWORD partition_no;
    if (!GetSystemDiskNumber(diskNumber, &partition_no)) {
        amiDebugLog("Error : GetSystemDiskNumber error");
        return MxError_GetCD;
    }

    AppendAsterisk();
    if (!MountRecoveryVolume(*diskNumber)) {
        amiDebugLog("Error : MountRecoveryVolume error");
        return MxError_MountRecovery;
    }

    return MxError_None;
}
BOOL CheckEwfState(EWF_STATE* state) {
    // TODO: Remove this once mice stubs EWF
    // *state = EWF_ENABLED;
    *state = EWF_DISABLED;
    return TRUE;

    PEWF_VOLUME_NAME_ENTRY pProVolList = EwfMgrGetProtectedVolumeList();
    if (pProVolList == NULL) {
        amiDebugLog("Error : Invalid Parameter pProVoList");
        return FALSE;
    }

    if (EwfMgrVolumeNameListIsEmpty(pProVolList)) {
        amiDebugLog("EWF EMPTY");
    }

    while (!EwfMgrVolumeNameListIsEmpty(pProVolList)) {
        WCHAR chDrive = EwfMgrGetDriveLetterFromVolumeName(pProVolList->Name);
        amiDebugLog("Doing one... %c", chDrive);
        if (chDrive == -1) {
            EwfMgrVolumeNameListDelete(pProVolList);
            amiDebugLog("Error : EwfMgrGetDriveLetterFromVolumeName error");
            return FALSE;
        }
        if (chDrive != L'C') {
            EwfMgrVolumeNameEntryPop(&pProVolList);
            continue;
        }

        HANDLE volHandle = EwfMgrOpenProtected(pProVolList->Name);
        if (volHandle == INVALID_HANDLE_VALUE) {
            EwfMgrVolumeNameListDelete(pProVolList);
            amiDebugLog("Error : EwfMgrOpenProtected error");
            return FALSE;
        }

        PEWF_VOLUME_CONFIG config = EwfMgrGetProtectedVolumeConfig(volHandle);
        if (config == NULL) {
            EwfMgrVolumeNameListDelete(pProVolList);
            EwfMgrClose(volHandle);
            amiDebugLog("Error : EwfMgrGetProtectedVolumeConfig error");
            return FALSE;
        }

        *state = config->State;
        EwfMgrClose(volHandle);
        EwfMgrVolumeNameListDelete(pProVolList);
        return TRUE;
    }
    EwfMgrVolumeNameListDelete(pProVolList);
    amiDebugLog("Error : Not Found Volume \"C:\\\"");
    return FALSE;
}

void ReadLittleEndian(LPBYTE lpBuffer, DWORD_PTR nBytes, LPDWORD lpdwOut) {
    *lpdwOut = 0;
    if (nBytes > 0) {
        for (DWORD_PTR i = 0; i < nBytes; i++) {
            *lpdwOut = *lpdwOut + (lpBuffer[i] << ((i * 8) & 0x1f));
        }
    }
}

bool CheckSectorCRC(LPBYTE lpSector) {
    uint32_t table[256];
    DWORD readCrc;

    ReadLittleEndian(lpSector, 4, &readCrc);
    amiCrc32RCreateTable(table);
    return amiCrc32RGet(table, 508, &(lpSector[4]), 0) == readCrc;
}

bool GetSBRSlotOSState(DWORD diskNumber, int* updateState) {
    BYTE buffer[512];
    HANDLE hDisk;
    DWORD dwLBA;

    if (osuOpenDevice(diskNumber, &hDisk) != 0) return 0;
    if (osuReadSector(hDisk, 0, 1, buffer) != 0) {
        CloseHandle(hDisk);
        return 0;
    }
    if (buffer[510] != 0x55 || buffer[511] != 0xaa) {
        CloseHandle(hDisk);
        return 0;
    }
    if (buffer[482] == 5 || buffer[482] == 15) {
        ReadLittleEndian(&(buffer[0x1e6]), 4, &dwLBA);
    } else {
        if (buffer[498] != 5 && buffer[498] != 15) {
            CloseHandle(hDisk);
            return 0;
        }
        ReadLittleEndian(&(buffer[0x1f6]), 4, &dwLBA);
    }

    if (osuReadSector(hDisk, dwLBA + 3, 1, buffer) != 0) {
        CloseHandle(hDisk);
        return 0;
    }

    if (!CheckSectorCRC(buffer)) {
        if (osuReadSector(hDisk, dwLBA + 2, 1, buffer) != 0) {
            CloseHandle(hDisk);
            return 0;
        }
        if (!CheckSectorCRC(buffer)) {
            CloseHandle(hDisk);
            return 0;
        }
    }
    *updateState = (unsigned int)(buffer[67]);

    CloseHandle(hDisk);
    return 1;
}

void SetDefaultShell(void) {
    HKEY hkWinlogon;
    LSTATUS pProVolList = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                                        "Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                                        0, KEY_ALL_ACCESS, &hkWinlogon);
    if (pProVolList != ERROR_SUCCESS) {
        amiDebugLog("Error : RegOpenKeyExA failed.");
    } else {
        pProVolList = RegSetValueExA(hkWinlogon, "shell", 0, REG_SZ, (LPBYTE) "explorer.exe",
                                     strlen("explorer.exe"));
        if (pProVolList != ERROR_SUCCESS) {
            amiDebugLog("Error : RegSetValueExA failed.");
        }
        RegCloseKey(hkWinlogon);
    }
}

BOOL RetryCommand(LPSTR lpszCommand, DWORD dwCount, DWORD dwDelay) {
    for (DWORD i = 0; i < dwCount; i++) {
        if (osuExecProcess(lpszCommand) == 0) return TRUE;
        Sleep(dwDelay);
    }
    return FALSE;
}

BOOL GetSlotDiskNumber(DWORD nDevice, short* pPartition, BYTE slotType) {
    BYTE readBuf[512];
    BOOL local_10 = FALSE;

    HANDLE hDevice;
    if (osuOpenDevice(nDevice, &hDevice) != 0) {
        amiDebugLog("Error : osuOpenDevice error");
        return FALSE;
    }

    if (osuReadSector(hDevice, 0, 1, readBuf) != 0) {
        amiDebugLog("Error : osuReadSector(MBR) error");
        CloseHandle(hDevice);
        return FALSE;
    }

    if (readBuf[510] != 0x55 || readBuf[511] != 0xaa) {
        amiDebugLog("Error : Partition Table is not found.");
        CloseHandle(hDevice);
        return FALSE;
    }

    DWORD extLba;
    if (readBuf[482] == 5 || readBuf[482] == 0xf) {
        ReadLittleEndian(&(readBuf[0x1e6]), 4, &extLba);
        if (readBuf[498] == 0) {
            *pPartition = 3;
        } else {
            *pPartition = 4;
        }
    } else {
        if (readBuf[498] != 5 && readBuf[498] != 0xf) {
            amiDebugLog("Error : Invalid Partition Table error");
            CloseHandle(hDevice);
            return FALSE;
        }
        ReadLittleEndian(&(readBuf[0x1f6]), 4, &extLba);
        *pPartition = 4;
    }
    if (osuReadSector(hDevice, extLba + 1, 1, readBuf) != 0) {
        amiDebugLog("Error : osuReadSector(SPD) error");
        CloseHandle(hDevice);
        return FALSE;
    }

    if (!CheckSectorCRC(readBuf)) {
        amiDebugLog("Error : CheckSectorCRC error");
        CloseHandle(hDevice);
        return FALSE;
    }

    for (int local_218 = 0x10; readBuf[local_218] != 0; local_218 += 0x10) {
        if (readBuf[local_218] == slotType) {
            CloseHandle(hDevice);
            return TRUE;
        }
        *pPartition = *pPartition + 1;
    }
    CloseHandle(hDevice);
    return FALSE;
}

bool GetAppDataSlotDiskNumber(DWORD nDevice, short* pPartition) {
    return GetSlotDiskNumber(nDevice, pPartition, 0x40) != FALSE;
}
bool GetOSSlotDiskNumber(DWORD nDevice, short* pPartition) {
    return GetSlotDiskNumber(nDevice, pPartition, 0x30) != FALSE;
}

BOOL MountUpdate(DWORD nDevice) {
    short nPartition;
    if (!GetOSSlotDiskNumber(nDevice, &nPartition)) return FALSE;

    char drive[256];
    sprintf_s(drive, sizeof drive, "\\Device\\Harddisk%d\\Partition%d", nDevice, nPartition);

    char szCommand[256];
    snprintf(szCommand, sizeof szCommand, TRUECRYPT " /k %s /v %s /l %s /s /q", OS_UPDATE_KEY_FILE,
             drive, UPDATE_MOUNT);
    return RetryCommand(szCommand, 100, 200);
}
BOOL CheckIsRingRegistryPresent(BOOL* lpIsRing) {
    HKEY hKey;
    LSTATUS stat = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\SEGA\\RING\\GRAPHICS", 0,
                                 KEY_QUERY_VALUE, &hKey);
    if (stat == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        *lpIsRing = TRUE;
        return TRUE;
    } else if (stat == ERROR_FILE_NOT_FOUND) {
        *lpIsRing = FALSE;
        return TRUE;
    } else {
        *lpIsRing = FALSE;
        return FALSE;
    }
}
BOOL _MountDriver(void) {
    char szCommand[256];
    snprintf(szCommand, sizeof szCommand, TRUECRYPT " /k %s /v %s /l %s /s /q", OS_UPDATE_KEY_FILE,
             DEFAULT_DRIVERS, DRIVERS_MOUNT);
    return RetryCommand(szCommand, 100, 200);
}
BOOL MountDriver(BOOL isRing) {
    if (!isRing) return TRUE;
    return _MountDriver();
}

typedef enum PLATFORM {
    PLATFORM_RW = 0,
    PLATFORM_RE = 1,
    PLATFORM_RE2 = 2,
    PLATFORM_UNK = 3,
} PLATFORM;

BOOL CheckPlatform(PLATFORM* lpPlatform) {
    AM_PLATFORM_PLATFORM_ID_EX platform;
    if (amPlatformGetPlatformIdEx(&platform) != AM_PLATFORM_STATUS_OK) {
        *lpPlatform = PLATFORM_UNK;
        return FALSE;
    }

    if (strcmp(platform.strPlatformId, PLATFORM_RINGEDGE) == 0) {
        if (platform.platformId == 1) {
            *lpPlatform = PLATFORM_RW;
            return TRUE;
        } else if (platform.platformId == 2) {
            *lpPlatform = PLATFORM_RE;
            return TRUE;
        } else {
            *lpPlatform = PLATFORM_UNK;
            amiDebugLog("Error : Platform is unknown.");
            return FALSE;
        }
    } else if (strcmp(platform.strPlatformId, PLATFORM_RINGWIDE) == 0) {
        if (platform.platformId == 1) {
            *lpPlatform = PLATFORM_RE2;
            return TRUE;
        } else {
            *lpPlatform = PLATFORM_UNK;
            amiDebugLog("Error : Platform is unknown.");
            return FALSE;
        }
    } else {
        *lpPlatform = PLATFORM_UNK;
        amiDebugLog("Error : Platform is unknown.");
        return FALSE;
    }
}

BOOL ExecuteMxOSUpdate(BOOL isMasterRing) {
    HINSTANCE result = NULL;

    PLATFORM platform = PLATFORM_UNK;
    if (!CheckPlatform(&platform)) return FALSE;

    if (platform != PLATFORM_RW) {
        if (platform == PLATFORM_RE) {
            if (!PathFileExistsA(OS_UPDATE_CONF)) {
                if (!isMasterRing) {
                    amiDebugLog("Error : Platform = EDGE2 but Master OS is not EDGE2");
                    return FALSE;
                } else {
                    amiDebugLog("Execute %s", DRIVERS_UPDATE_EXE);
                    result =
                        ShellExecuteA(NULL, NULL, DRIVERS_UPDATE_EXE, NULL, DRIVERS_MOUNT_SLASH, 1);
                    return (int)result < 33 ? FALSE : TRUE;
                }
            } else {
                amiDebugLog("Execute %s\n", UPDATE_EXE);
                result = ShellExecuteA(NULL, NULL, UPDATE_EXE, NULL, UPDATE_MOUNT_SLASH, 1);
                return (int)result < 33 ? FALSE : TRUE;
            }
        }
        if (platform != PLATFORM_RE2) {
            amiDebugLog("Error : Unknown platform");
            return FALSE;
        }
    }
    amiDebugLog("Execute %s", UPDATE_EXE);
    result = ShellExecuteA(NULL, NULL, UPDATE_EXE, NULL, UPDATE_MOUNT_SLASH, 1);
    return (int)result < 33 ? FALSE : TRUE;
}
BOOL ChangeEWFStateEnable(void) {
    // TODO: Remove this once mice stubs EWF
    return TRUE;

    PEWF_VOLUME_NAME_ENTRY pProVolList = EwfMgrGetProtectedVolumeList();
    if (pProVolList == NULL) {
        amiDebugLog("Error : EwfMgrGetProtectedVolumeList error");
        return FALSE;
    }
    while (!EwfMgrVolumeNameListIsEmpty(pProVolList)) {
        WCHAR chDrive = EwfMgrGetDriveLetterFromVolumeName(pProVolList->Name);
        if (chDrive == -1) {
            EwfMgrVolumeNameListDelete(pProVolList);
            amiDebugLog("Error : EwfMgrGetDriveLetterFromVolumeName error");
            return FALSE;
        }
        if (chDrive != L'C') {
            EwfMgrVolumeNameEntryPop(&pProVolList);
            continue;
        }

        HANDLE volHandle = EwfMgrOpenProtected(pProVolList->Name);
        if (volHandle == INVALID_HANDLE_VALUE) {
            EwfMgrVolumeNameListDelete(pProVolList);
            amiDebugLog("Error : EwfMgrOpenProtected error");
            return FALSE;
        }

        if (!EwfMgrEnable(volHandle)) {
            EwfMgrClose(volHandle);
            EwfMgrVolumeNameListDelete(pProVolList);
            amiDebugLog("Error : EwfMgrEnable error");
            return FALSE;
        }
        EwfMgrClose(volHandle);
        EwfMgrVolumeNameListDelete(pProVolList);
        return TRUE;
    }
    EwfMgrVolumeNameListDelete(pProVolList);
    amiDebugLog("Error : Not Found Volume \"C:\\\"");
    return FALSE;
}

BOOL MountAppDataPartition(DWORD diskNumber, short partitionNumber) {
    char targetPath[256];

    sprintf_s(targetPath, sizeof targetPath, "\\Device\\Harddisk%d\\Partition%d", diskNumber,
              partitionNumber);
    if (osuMountPartition(targetPath, "\\\\.\\Y:\\") != 0) {
        amiDebugLog("Error : osuMountPartiotion error");
        return FALSE;
    }
    return TRUE;
}
BOOL ExecCalcToAppData(void) {
    for (int i = 0; i < 100; i++) {
        int err = osuExecProcess("cacls y:\\ /t /e /p everyone:F");
        if (err != -140) {
            amiDebugLog("Error : osuExecProcess ErrCode = %n", err);
            return TRUE;
        }
        Sleep(200);
    }
    return FALSE;
}
BOOL SetAppDataAccessPrivilege(DWORD diskNumber) {
    short appDataPartition;
    if (!GetAppDataSlotDiskNumber(diskNumber, &appDataPartition)) {
        amiDebugLog("Error : GetAppDataDiskNumber error");
        return FALSE;
    }
    if (!MountAppDataPartition(diskNumber, appDataPartition)) {
        amiDebugLog("Error : MountAppDataPartition error");
        return FALSE;
    }
    if (!ExecCalcToAppData()) {
        amiDebugLog("Error : ExecCalcToAppData error");
        return FALSE;
    }
    return TRUE;
}

void SetBackGroundColor(void) {
    HKEY hkColors;
    if (RegOpenKeyExA(HKEY_USERS, ".Default\\Control Panel\\Colors", 0, KEY_ALL_ACCESS,
                      &hkColors) != ERROR_SUCCESS) {
        amiDebugLog("Error : RegOpenKeyExA failed.");
        return;
    }
    if (RegSetValueExA(hkColors, "Background", 0, REG_SZ, (LPBYTE) "255 255 255",
                       strlen("255 255 255")) != ERROR_SUCCESS) {
        amiDebugLog("Error : RegSetValueExA failed.");
    }
    RegCloseKey(hkColors);

    return;
}

BOOL SetBackupComputerName() {
    if (amEepromInit(NULL) != AM_EEPROM_STATUS_OK) {
        amiDebugLog("Error : amEepromInit error");
        return FALSE;
    }
    if (amBackupInit() != 0) {
        amEepromExit();
        amiDebugLog("Error : amBackupInit error");
        return FALSE;
    }

    AM_BACKUP_RECORD* lpConfig = amBackupGetRecord(AM_BACKUP_RECORD_ALPB_COMPUTER_NAME);
    AM_SYSDATAwH_ALPB_COMPUTER_NAME record;
    int err = amBackupRecordReadDup(lpConfig, &record);
    amBackupExit();
    amEepromExit();
    if (err != 0 || strlen(record.m_Name) == 0 || strlen(record.m_Name) > MAX_COMPUTERNAME_LENGTH)
        return FALSE;

    return SetComputerNameA(record.m_Name);
}
bool MxChangeActivePartition(BYTE partition) { return osuSetActivePartition(partition) == 0; }

MxError_t SystemUpdateStep2(DWORD diskNumber) {
    AppendAsterisk();
    SetDefaultShell();
    AppendAsterisk();
    if (!GetOSUpdateKeyFile()) {
        amiDebugLog("Error : GetOSUpdateKeyFile error");
        return MxError_OSUpdateKeyFile;
    }
    AppendAsterisk();
    if (!MountUpdate(diskNumber)) {
        amiDebugLog("Error : MountUpdate error");
        return MxError_MountUpdate;
    }
    AppendAsterisk();
    BOOL isRing = FALSE;
    if (!CheckIsRingRegistryPresent(&isRing)) {
        amiDebugLog("Error : CheckIsRingRegistryPresent error");
        return MxError_RingRegistry;
    }
    AppendAsterisk();
    if (!MountDriver(isRing)) {
        amiDebugLog("Error : MountDriver error");
        return MxError_MountDriver;
    }
    AppendAsterisk();
    if (!DeleteFileA(OS_UPDATE_KEY_FILE)) {
        amiDebugLog("Error : Deleate Key File error");
        return MxError_SysKeyDelete;
    }
    AppendAsterisk();
    if (!ExecuteMxOSUpdate(isRing)) {
        amiDebugLog("Error : ExecuteMxOSUpdate error");
        return MxError_ExecuteOSUpdate;
    }
    return MxError_None;
}
MxError_t SystemUpdateStep3(DWORD diskNumber) {
    AppendAsterisk();
    if (!ChangeEWFStateEnable()) {
        amiDebugLog("Error : ChangeEWFStateEnable error");
        return MxError_EWFEnable;
    }
    AppendAsterisk();
    if (!SetAppDataAccessPrivilege(diskNumber)) {
        amiDebugLog("Error : SetAppDataAccessPrivilege error");
        return MxError_DiskAccessPrivilege;
    }
    AppendAsterisk();
    SetBackGroundColor();
    AppendAsterisk();
    DeleteMounteDevices();
    AppendAsterisk();
    if (!SetBackupComputerName()) {
        amiDebugLog("Error : SetBackupComputerName failed.");
    }
    AppendAsterisk();
    SetDefaultShell();
    AppendAsterisk();

    EnterCriticalSection(&g_Top.criticalSection);
    g_Bottom.status_str = "Reboot";
    LeaveCriticalSection(&g_Top.criticalSection);

    Sleep(3000);
    AppendAsterisk();
    if (!MxChangeActivePartition(1)) {
        amiDebugLog("Error : MxChangeActivePartition error");
        return MxError_ChangeActivePartition;
    }
    AppendAsterisk();
    if (osuSystemReboot() != 0) {
        amiDebugLog("Error : osuSystemReboot error");
        return MxError_Reboot;
    }
    return MxError_None;
}

MxError_t NormalBootStep(void) {
    AppendAsterisk();
    if (!GetSystemKeyFile()) return MxError_NoKeyFile;
    AppendAsterisk();
    if (!TryMountSystemVolume()) return MxError_SystemMount;
    AppendAsterisk();
    if (!DeleteFileA(SYSTEM_KEY_FILE)) return MxError_SysKeyDelete;
    AppendAsterisk();
    if (!SetDiskAccessPrivilege()) return MxError_DiskAccessPrivilege;
    AppendAsterisk();
    if (!ExecuteMxMaster()) return MxError_MxMaster;
    return MxError_None;
}

int DoStartup() {
    StartupStep_t step = Init;
    MxError_t error = MxError_None;
    DWORD diskNumber = 0;

    while (step != Done) {
        printf("Step: %d (%d)\n", step, error);

        switch (step) {
            case Init:
                if (!MxInitStartup(&diskNumber))
                    step = CheckEwf;
                else
                    step = Done;
                break;
            case CheckEwf: {
                AppendAsterisk();
                EWF_STATE state;
                if (!CheckEwfState(&state)) {
                    error = MxError_Ewf;
                    step = Done;
                    break;
                }

                if (state == EWF_ENABLED)
                    step = NormalBoot;
                else
                    step = EwfDisabled;

                break;
            }
            case EwfDisabled: {
                char versionString[VERSION_BUFFER];
                char defaultVersionString[VERSION_BUFFER];

                if (!GetCurrentVersion(versionString) || !GetDefaultVersion(defaultVersionString)) {
                    error = MxError_Version;
                    step = Done;
                    break;
                }

                if (strcmp(versionString, defaultVersionString) == 0)
                    step = GetSBR;
                else
                    step = SystemUpdate3;
                break;
            }
            case GetSBR: {
                int updateState;
                AppendAsterisk();
                if (!GetSBRSlotOSState(diskNumber, &updateState)) {
                    error = MxError_SBR;
                    step = Done;
                    break;
                }

                if (updateState == 2)
                    step = SystemUpdate2;
                else
                    step = SystemUpdate3;
                break;
            }
            case NormalBoot:
                error = NormalBootStep();
                step = Done;
                break;
            case SystemUpdate2:
                setStrForStep2();
                error = SystemUpdateStep2(diskNumber);
                step = Done;
                break;
            case SystemUpdate3:
                setStrForStep3();
                error = SystemUpdateStep3(diskNumber);
                step = Done;
                break;
            default:
                error = MxError_Unknown;
                step = Done;
                break;
        }
    }
    return error;
}

bool ExitGraphics(void) {
    if (!osgExit(&g_Osu)) {
        amiDebugLog("Error : osgExit error");
        return false;
    }
    return true;
}

void DisplayError(MxError_t err) {
    static char message[32];
    message[0] = '\0';
    sprintf_s(message, sizeof message, "Error %d", err);

    EnterCriticalSection(&g_Top.criticalSection);
    g_Bottom.status_str = message;
    g_Bottom.status_use_counter = 0;
    g_Bottom.status_colour = Red;
    g_Bottom.str2 = "";
    g_Bottom.hide_power = 0;
    LeaveCriticalSection(&g_Top.criticalSection);
}

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    PVOID old = NULL;
    Wow64DisableWow64FsRedirection(&old);

    // TODO: Remove this
    nShowCmd = 1;
    if (!InitGraphics(hInstance, nShowCmd)) amiDebugLog("Error : InitGraphics failed.");

    g_Bottom.backgroundColour = Green;

    g_Top.str1 = "MxDraw Demo";
    g_Top.str2 = "owo what's this";
    g_Bottom.status_str = "I blink!!";
    g_Bottom.str2 = "TURN OFF THE POWER, IF YOU DARE";
    g_Bottom.line5 = "Look mum I'm an asteri************";
    g_Bottom.debug_text = "Debug text goes here, but nothing uses me!!";

    while (1) {
        g_Top.show_top = TRUE;
        g_Top.debugEnable = TRUE;
        Sleep(100);
    }
    Sleep(10000);

    if (!amLogInit()) {
        LOG_EN_PLATFORM = 1;
        AM_LOG_OPTION logMxStartup = {
            .m_EventCategory = 1,
            .m_EventId = 1,
            .m_EventSource = "mxstartup",
            .m_EventType = 1,
        };
        amLogSetEventOption(&logMxStartup);
        amiDebugSetCallback(amLogReportEvent);
    }

    MxError_t err = DoStartup();
    if (!err) {
        if (!ExitGraphics()) amiDebugLog("Error : ExitGraphics failed.");
        amLogFinalize();
        return 0;
    }

    while (1) {
        DisplayError(err);
        Sleep(100000);
    }

    return 0;
}

int main() { return WinMain(NULL, NULL, NULL, 0); }
