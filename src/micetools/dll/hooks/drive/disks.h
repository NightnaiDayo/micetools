#pragma once
#include <Windows.h>

#include "segastruct.h"

typedef struct _physical_disk physical_disk_t;
typedef struct _disk_partition disk_partition_t;
typedef struct _disk_volume disk_volume_t;
typedef struct _disk_raw disk_raw_t;

#define MAX_DISKS 32
extern physical_disk_t* PHYSICAL_DISKS[MAX_DISKS];

typedef BOOL(mice_partition_read_function_t)(DWORD nOffset, LPVOID lpBuffer,
                                             DWORD nNumberOfBytesToRead,
                                             LPDWORD lpNumberOfBytesRead);
typedef BOOL(mice_partition_write_function_t)(DWORD nOffset, LPCVOID lpBuffer,
                                              DWORD nNumberOfBytesToWrite,
                                              LPDWORD lpNumberOfBytesWritten);

typedef enum {
    DiskType_HardDisk,
    DiskType_Flash,
    DiskType_CdRom,
    DiskType_Floppy,
} DiskType;
struct _disk_volume {
    wchar_t m_FilenameW[MAX_PATH + 1];
    char m_FilenameA[MAX_PATH + 1];
    LPSTR m_Name;
    char m_MountPoint;

    // Used for: \\Device\\DeviceName (kernel mode)
    wchar_t m_DeviceName[MAX_PATH];
    // Used for: \\OsDevices\\DosDeviceName (kernel mode), \\\\.\\DosDeviceName (user mode)
    wchar_t m_DosDeviceName[MAX_PATH];

    // Back-references
    physical_disk_t* m_pDrive;
    disk_partition_t* m_pPartition;
    BOOL m_NameIsOnHeap;
};
struct _disk_partition {
    DWORD m_Size;
    UCHAR m_Filesystem;
    spd_slot_t m_SPDContent;
    mice_partition_read_function_t* m_ReadFunc;
    mice_partition_write_function_t* m_WriteFunc;

    disk_volume_t m_Volume;

    DWORD m_PhysicalLBA;
    DWORD m_SlotLBA;
    DWORD m_PartitionNumber;
};
struct _disk_raw {
    mice_partition_read_function_t* m_ReadFunc;
    mice_partition_write_function_t* m_WriteFunc;
};
struct _physical_disk {
    DWORD m_SerialNumber;

    DWORD m_DriveNumber;
    STORAGE_BUS_TYPE m_BusType;
    DEVICE_TYPE m_DeviceType;
    LPSTR m_VID;
    LPSTR m_PID;
    DWORD m_BootPartition;
    BOOL m_HasSegaboot;
    DWORD m_BlockSize;
    DWORD m_TotalSize;
    DiskType m_DiskType;

    // Used for: \\Device\\DeviceName (kernel mode)
    wchar_t m_DeviceName[MAX_PATH];
    // Used for: \\OsDevices\\DosDeviceName (kernel mode), \\\\.\\DosDeviceName (user mode)
    wchar_t m_DosDeviceName[MAX_PATH];

    BOOL m_IsFormatted;
    disk_partition_t m_Partitions[4];
    disk_partition_t m_Extended[];
};
