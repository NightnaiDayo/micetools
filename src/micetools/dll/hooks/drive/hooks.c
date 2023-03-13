#include "drive.h"

typedef struct {
    size_t disk;
    size_t partition;
} find_index_t;
disk_volume_t* incrementFindIndex(HANDLE hFindVolume) {
    find_index_t* find_index = GetDataForHandle(hFindVolume, HDATA_FIND_VOLUME);
    if (find_index == NULL) return NULL;

    while (1) {
        if (find_index->disk >= MAX_DISKS || PHYSICAL_DISKS[find_index->disk] == NULL) {
            return NULL;
        }

        if (find_index->partition > 3) {
            if (PHYSICAL_DISKS[find_index->disk]
                    ->m_Extended[find_index->partition - 4]
                    .m_PartitionNumber == 0) {
                find_index->disk++;
                find_index->partition = 0;
                continue;
            }

            return &(PHYSICAL_DISKS[find_index->disk]
                         ->m_Extended[(find_index->partition++) - 4]
                         .m_Volume);
        }
        if (PHYSICAL_DISKS[find_index->disk]
                ->m_Partitions[find_index->partition]
                .m_PartitionNumber == 0) {
            find_index->partition = 4;
            continue;
        }

        return &(PHYSICAL_DISKS[find_index->disk]->m_Partitions[find_index->partition++].m_Volume);
    }
}

BOOL WINAPI FakeFindNextVolumeW(HANDLE hFindVolume, LPWSTR lpszVolumeName, DWORD cchBufferLength) {
    disk_volume_t* volume = incrementFindIndex(hFindVolume);
    if (volume == NULL) {
        SetLastError(ERROR_NO_MORE_FILES);
        return FALSE;
    }

    wcscpy_s(lpszVolumeName, cchBufferLength, volume->m_FilenameW);
    wcscat_s(lpszVolumeName, cchBufferLength, L"\\");
    return TRUE;
}
BOOL WINAPI FakeFindNextVolumeA(HANDLE hFindVolume, LPSTR lpszVolumeName, DWORD cchBufferLength) {
    disk_volume_t* volume = incrementFindIndex(hFindVolume);
    if (volume == NULL) {
        SetLastError(ERROR_NO_MORE_FILES);
        return FALSE;
    }

    strcpy_s(lpszVolumeName, cchBufferLength, volume->m_FilenameA);
    strcat_s(lpszVolumeName, cchBufferLength, "\\");
    return TRUE;
}
HANDLE WINAPI FakeFindFirstVolumeW(LPWSTR lpszVolumeName, DWORD cchBufferLength) {
    find_index_t* find_index = malloc(sizeof(find_index_t));
    find_index->disk = 0;
    find_index->partition = 0;
    HANDLE handle = GetDummyHandle();
    SetDataForHandle(handle, HDATA_FIND_VOLUME, find_index, TRUE);

    FakeFindNextVolumeW(handle, lpszVolumeName, cchBufferLength);
    return handle;
}
HANDLE WINAPI FakeFindFirstVolumeA(LPSTR lpszVolumeName, DWORD cchBufferLength) {
    find_index_t* find_index = malloc(sizeof(find_index_t));
    find_index->disk = 0;
    find_index->partition = 0;
    HANDLE handle = GetDummyHandle();
    SetDataForHandle(handle, HDATA_FIND_VOLUME, find_index, TRUE);

    FakeFindNextVolumeA(handle, lpszVolumeName, cchBufferLength);
    return handle;
}
BOOL WINAPI FakeFindVolumeClose(HANDLE hFindVolume) {
    if (RemoveDataForHandle(hFindVolume, HDATA_FIND_VOLUME)) return _CloseHandle(hFindVolume);
    return FALSE;
}

BOOL WINAPI FakeGetVolumeInformationA(LPCSTR lpRootPathName, LPSTR lpVolumeNameBuffer,
                                      DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber,
                                      LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags,
                                      LPSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize) {
    disk_volume_t* volume = getVolumeByPath(lpRootPathName, VOL_MATCH_ALL);
    if (volume == NULL) return FALSE;

    if (lpVolumeSerialNumber) *lpVolumeSerialNumber = volume->m_pDrive->m_SerialNumber;

    // TODO: Ripped from a RingEdge. Make these part of our emulation.
    if (lpFileSystemFlags) *lpFileSystemFlags = 0x700ff;
    if (lpMaximumComponentLength) *lpMaximumComponentLength = 255;
    // TODO: This isn't even true for non-NTFS partitions
    if (lpFileSystemNameBuffer) strncpy_s(lpFileSystemNameBuffer, nFileSystemNameSize, "NTFS", 5);

    if (lpVolumeNameBuffer && nVolumeNameSize) {
        if (volume->m_Name)
            strcpy_s(lpVolumeNameBuffer, nVolumeNameSize, volume->m_Name);
        else
            lpVolumeNameBuffer[0] = '\0';
    }
    return TRUE;
}
BOOL WINAPI FakeSetVolumeLabelA(LPCSTR lpRootPathName, LPCSTR lpVolumeName) {
    disk_volume_t* volume = getVolumeByPath(lpRootPathName, VOL_MATCH_ALL);
    if (volume == NULL) return FALSE;

    if (lpVolumeName) {
        size_t len = strlen(lpVolumeName);
        // By default volume names are pointers to constants!
        if (volume->m_NameIsOnHeap)
            realloc(volume->m_Name, len + 1);
        else {
            volume->m_Name = malloc(len + 1);
            volume->m_NameIsOnHeap = TRUE;
        }
        memcpy(volume->m_Name, lpVolumeName, len);
    } else {
        volume->m_Name = NULL;
    }
    return TRUE;
}
BOOL WINAPI FakeGetVolumeNameForVolumeMountPointA(LPCSTR lpszVolumeMountPoint, LPSTR lpszVolumeName,
                                                  DWORD cchBufferLength) {
    log_trace(plfDrive, "GetVolumeNameForVolumeMountPointA(%s)", lpszVolumeMountPoint);

    disk_volume_t* volume = getVolumeByPath(lpszVolumeMountPoint, VOL_MATCH_PATH);
    if (volume == NULL) {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    strcpy_s(lpszVolumeName, cchBufferLength, volume->m_FilenameA);
    return TRUE;
}
BOOL WINAPI FakeGetVolumePathNamesForVolumeNameA(LPCSTR lpszVolumeName, LPCH lpszVolumePathNames,
                                                 DWORD cchBufferLength, PDWORD lpcchReturnLength) {
    log_trace(plfDrive, "GetVolumeNameForVolumeMountPointA(%s)", lpszVolumeName);

    disk_volume_t* volume = getVolumeByPath(lpszVolumeName, VOL_MATCH_GUID);
    if (volume == NULL) {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    if (volume->m_MountPoint) {
        // TODO: Handle the situation where the buffer is too small
        *lpcchReturnLength =
            sprintf_s(lpszVolumePathNames, cchBufferLength, "\\\\.\\%c:", volume->m_MountPoint);
        if (*lpcchReturnLength != cchBufferLength) {
            lpszVolumePathNames[*lpcchReturnLength] = '\0';
            *lpcchReturnLength++;
        }
    } else {
        *lpcchReturnLength = 0;
    }

    return TRUE;
}
BOOL WINAPI FakeDeleteVolumeMountPointA(LPCSTR lpszVolumeMountPoint) {
    log_trace(plfDrive, "DeleteVolumeMountPointA(%s)", lpszVolumeMountPoint);
    disk_volume_t* volume = getVolumeByPath(lpszVolumeMountPoint, VOL_MATCH_PATH);
    if (volume == NULL) {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    volume->m_MountPoint = '\0';
    return TRUE;
}
BOOL WINAPI FakeSetVolumeMountPointA(LPCSTR lpszVolumeMountPoint, LPCSTR lpszVolumeName) {
    log_trace(plfDrive, "SetVolumeMountPointA(%s)", lpszVolumeMountPoint);
    disk_volume_t* volume = getVolumeByPath(lpszVolumeName, VOL_MATCH_GUID);
    if (volume == NULL) {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    char cMountPoint;
    DWORD nScan;
    if (sscanf_s(lpszVolumeMountPoint, "\\\\.\\%c:%n", &cMountPoint, 1, &nScan) == 1 &&
        nScan == 6) {
        log_info(plfDrive, "Mounting %s at %c:\\", volume->m_Name, cMountPoint);
        volume->m_MountPoint = cMountPoint;
        return TRUE;
    }
    if (sscanf_s(lpszVolumeMountPoint, "%c:\\%n", &cMountPoint, 1, &nScan) == 1 && nScan == 3) {
        log_info(plfDrive, "Mounting %s at %c:\\", volume->m_Name, cMountPoint);
        volume->m_MountPoint = cMountPoint;
        return TRUE;
    }

    return FALSE;
}

DWORD WINAPI FakeQueryDosDeviceA(LPCSTR lpDeviceName, LPSTR lpTargetPath, DWORD ucchMax) {
    if (lpDeviceName != NULL) {
        disk_volume_t* volume = getVolumeByPath(lpDeviceName, VOL_MATCH_DOS_DEVICE);
        if (volume == NULL) {
            SetLastError(ERROR_FILE_NOT_FOUND);
            return 0;
        }

        size_t mountLen = wcslen(volume->m_DeviceName);
        if (ucchMax < mountLen + 1) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return 0;
        }
        sprintf_s(lpTargetPath, ucchMax, "\\Device\\%ls", volume->m_DeviceName);
        return ucchMax;
    }
    return 0;
}

MCIERROR WINAPI Fake_mciSendStringA(LPCTSTR lpszCommand, LPTSTR lpszReturnString, UINT cchReturn,
                                    HANDLE hwndCallback) {
    if (strcmp(lpszCommand, "open cdaudio") == 0) {
        return 0;
    }
    if (strcmp(lpszCommand, "set cdaudio door open") == 0) {
        log_game(plfDrive, "mci:Cupholder opened!");
        return 0;
    }
    if (strcmp(lpszCommand, "status cdaudio media present") == 0) {
        if (lpszReturnString) {
            if (TRUE)
                strcpy_s(lpszReturnString, cchReturn, "true");
            else
                strcpy_s(lpszReturnString, cchReturn, "false");
        }
        return 0;
    }
    if (strcmp(lpszCommand, "close cdaudio") == 0) {
        return 0;
    }
    return MCIERR_UNRECOGNIZED_COMMAND;
}

UINT WINAPI FakeGetDriveTypeA(LPCSTR lpRootPathName) {
    disk_volume_t* volume = getVolumeByPath(lpRootPathName, VOL_MATCH_PATH | VOL_MATCH_DOS_DEVICE);
    if (volume == NULL) {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return DRIVE_NO_ROOT_DIR;
    }

    switch (volume->m_pDrive->m_DiskType) {
        case DiskType_CdRom:
            return DRIVE_CDROM;
        case DiskType_HardDisk:
            return DRIVE_FIXED;
        case DiskType_Flash:
        case DiskType_Floppy:
            return DRIVE_REMOVABLE;
        default:
            return DRIVE_UNKNOWN;
    }
}

BOOL WINAPI FakeGetDiskFreeSpaceExA(LPCSTR lpDirectoryName,
                                    PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                                    PULARGE_INTEGER lpTotalNumberOfBytes,
                                    PULARGE_INTEGER lpTotalNumberOfFreeBytes) {
    // disk_volume_t* volume = getVolumeByPath(lpDirectoryName, VOL_MATCH_ALL);
    // if (volume == NULL) {
    //     SetLastError(ERROR_FILE_NOT_FOUND);
    //     return FALSE;
    // }

    // We're going to be remapping the drive to ./dev/, so the free bytes are whatever the current
    // real drive has free. No point claiming we have more than we do!
    return TrueGetDiskFreeSpaceExA(NULL, lpFreeBytesAvailableToCaller, lpTotalNumberOfBytes,
                                   lpTotalNumberOfFreeBytes);
}
