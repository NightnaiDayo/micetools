#pragma once

#ifndef _MICE_DRIVE
#define _MICE_DRIVE extern
#endif

#include <Windows.h>
// (clang-format moves Windows.h if this isn't here!)
#include <Mmsystem.h>
#include <Ntddscsi.h>
#include <Objbase.h>

#include "../../common.h"
#include "../files.h"
#include "disks.h"
#include "irb.h"

#define DISK_PATH "dev/disk/"
#define SPD_PATH (DISK_PATH "spd.bin")
#define SBR0_PATH (DISK_PATH "sbr0.bin")
#define SBR1_PATH (DISK_PATH "sbr1.bin")
#define ORIGINAL0_PATH (DISK_PATH "original0.bin")
#define PATCH0_PATH (DISK_PATH "patch0.bin")

extern physical_disk_t SSD;
extern physical_disk_t UPDATE_USB;
extern physical_disk_t LOG_USB;
extern physical_disk_t ALPHA_DVD;

extern sbr_t SegaBootRecordDefault;
extern sbr_t SegaBootRecord0;
extern sbr_t SegaBootRecord1;

// Functions
void init_volume(disk_volume_t* vol);
void init_pd(physical_disk_t* pd);
void init_all_pd();

mice_partition_read_function_t ReadFunc_Original0;
mice_partition_write_function_t WriteFunc_Original0;
mice_partition_read_function_t ReadFunc_Patch0;
mice_partition_write_function_t WriteFunc_Patch0;
mice_partition_read_function_t ReadFunc_OSLogFiles;
mice_partition_write_function_t WriteFunc_OS;

FnDeviceIoControl c_drive_DeviceIoControl;
FnDeviceIoControl x_drive_DeviceIoControl;
FnDeviceIoControl q_drive_DeviceIoControl;
FnDeviceIoControl pd_DeviceIoControl;
FnDeviceIoControl volume_DeviceIoControl;

FnReadFile pd_ReadFile;
FnWriteFile pd_WriteFile;
FnReadFile q_drive_ReadFile;

sbr_slot_t* get_sbr_slot(spd_slot_t slot);
disk_volume_t* incrementFindIndex(HANDLE hFindVolume);

#define VOL_MATCH_PATH 1
#define VOL_MATCH_GUID 2
#define VOL_MATCH_DOS_DEVICE 4
#define VOL_MATCH_ALL (VOL_MATCH_PATH | VOL_MATCH_GUID | VOL_MATCH_DOS_DEVICE)
inline static BOOL matchVolume(disk_volume_t* volume, LPCSTR lpRootPathName, DWORD match);
disk_volume_t* getVolumeByPath(LPCSTR lpRootPathName, DWORD match);

// Hooks
BOOL WINAPI FakeFindNextVolumeW(HANDLE hFindVolume, LPWSTR lpszVolumeName, DWORD cchBufferLength);
BOOL WINAPI FakeFindNextVolumeA(HANDLE hFindVolume, LPSTR lpszVolumeName, DWORD cchBufferLength);
HANDLE WINAPI FakeFindFirstVolumeW(LPWSTR lpszVolumeName, DWORD cchBufferLength);
HANDLE WINAPI FakeFindFirstVolumeA(LPSTR lpszVolumeName, DWORD cchBufferLength);
BOOL WINAPI FakeFindVolumeClose(HANDLE hFindVolume);
BOOL WINAPI FakeGetVolumeInformationA(LPCSTR lpRootPathName, LPSTR lpVolumeNameBuffer,
                                      DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber,
                                      LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags,
                                      LPSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize);
BOOL WINAPI FakeSetVolumeLabelA(LPCSTR lpRootPathName, LPCSTR lpVolumeName);
BOOL WINAPI FakeGetVolumeNameForVolumeMountPointA(LPCSTR lpszVolumeMountPoint, LPSTR lpszVolumeName,
                                                  DWORD cchBufferLength);
BOOL WINAPI FakeGetVolumePathNamesForVolumeNameA(LPCSTR lpszVolumeName, LPCH lpszVolumePathNames,
                                                 DWORD cchBufferLength, PDWORD lpcchReturnLength);
BOOL WINAPI FakeDeleteVolumeMountPointA(LPCSTR lpszVolumeMountPoint);
BOOL WINAPI FakeSetVolumeMountPointA(LPCSTR lpszVolumeMountPoint, LPCSTR lpszVolumeName);
DWORD WINAPI FakeQueryDosDeviceA(LPCSTR lpDeviceName, LPSTR lpTargetPath, DWORD ucchMax);
DWORD WINAPI FakeQueryDosDeviceW(LPCWSTR lpDeviceName, LPWSTR lpTargetPath, DWORD ucchMax);
BOOL WINAPI FakeDefineDosDeviceW(DWORD dwFlags, LPCWSTR lpDeviceName, LPCWSTR lpTargetPath);
BOOL WINAPI FakeSetVolumeMountPointW(LPCWSTR lpszVolumeMountPoint, LPCWSTR lpszVolumeName);
MCIERROR WINAPI Fake_mciSendStringA(LPCTSTR lpszCommand, LPTSTR lpszReturnString, UINT cchReturn,
                                    HANDLE hwndCallback);
UINT WINAPI FakeGetDriveTypeA(LPCSTR lpRootPathName);
UINT(WINAPI* TrueGetDriveTypeA)(LPCSTR lpRootPathName);
BOOL WINAPI FakeGetDiskFreeSpaceExA(LPCSTR lpDirectoryName,
                                    PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                                    PULARGE_INTEGER lpTotalNumberOfBytes,
                                    PULARGE_INTEGER lpTotalNumberOfFreeBytes);

_MICE_DRIVE BOOL(WINAPI* TrueGetDiskFreeSpaceExA)(LPCSTR lpDirectoryName,
                                                  PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                                                  PULARGE_INTEGER lpTotalNumberOfBytes,
                                                  PULARGE_INTEGER lpTotalNumberOfFreeBytes);

void hook_drives();

// MBR
#define MBR_LBA_GAP 0x3f  // 1 track worth of sectors is claimed for the MBR
#define EXT_HEADER_GAP 0x3f

#define MBR_FLAG_NONE 0x00
#define MBR_FLAG_BOOTABLE 0x80

#define MBR_FS_NONE 0x00
#define MBR_FS_EXT_CHS 0x05
#define MBR_FS_FAT16 0x06
#define MBR_FS_NTFS 0x07
#define MBR_FS_EXT_LBA 0x0F

#pragma pack(push, 1)
typedef struct mbr {
    BYTE bootstrap_code[446];
    struct {
        BYTE status;
        BYTE start_chs[3];
        BYTE type;
        BYTE end_chs[3];
        DWORD lba;
        DWORD sectors;
    } partitions[4];
    BYTE sig[2];
} mbr_t;
#pragma pack(pop)

#define IOCTL_CDROM_GET_DRIVE_GEOMETRY 0x2404c  // #include <Ntddcdrm.h> causes errors

#define BLOCK_SIZE_FLASH 4096
#define BLOCK_SIZE_HDD 512
#define BLOCK_SIZE_CDROM 2048
#define BLOCK_SIZE_DVD BLOCK_SIZE_CDROM

// SCSI Peripheral Device Type
typedef enum {
    DeviceTypeDAB = 0,
    DeviceTypeSAB,
    DeviceTypePrinter,
    DeviceTypeProcessor,
    DeviceTypeWriteOnce,
    DeviceTypeCdRom,
    DeviceTypeScanner,
    DeviceTypeOpticalMemory,
    DeviceTypeMediumChanger,
    DeviceTypeCommunications,
    DeviceTypeSAC = 12,
    DeviceTypeEnclosure,
    DeviceTypeOpticalCard,
    DeviceTypeAutomationInterface,
    DeviceTypeSecurityManager,
    DeviceTypeHMBlock,
    DeviceTypeUnknown = 31,
} DEVICE_TYPE;
