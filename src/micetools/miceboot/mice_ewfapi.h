#include <Windows.h>

typedef struct __EWF_LEVEL_DESC {
    WCHAR
    LevelName[EWF_MAX_LEVEL_NAME_LENGTH];
    FILETIME
    LevelEndTime;
    LONGLONG
    LevelDataSize;
} EWF_LEVEL_DESC, *PEWF_LEVEL_DESC;

typedef struct _EWF_OVERLAY_STORE_CONFIG {
    ULONG
    FormatVersion;
    LONGLONG
    VolumeSize;
    ULONG
    NumSegments;
    ULONG
    FreeSegments;
    ULONG
    SegmentSize;
    ULONG
    MaxVolumes;
    ULONG
    NumVolumes;
    USHORT
    MaxLevels;
    EWF_VOLUME_DESC
    VolumeDescArray[1];
} EWF_OVERLAY_STORE_CONFIG, *PEWF_OVERLAY_STORE_CONFIG;

typedef struct _EWF_VOLUME_CONFIG {
    EWF_TYPE
    Type;
    EWF_STATE
    State;
    EWF_COMMAND
    BootCommand;
    UCHAR
    PersistentData[EWF_MAX_PERSISTENT_DATA];
    USHORT
    MaxLevels;
    ULONG
    ClumpSize;
    USHORT
    CurrentLevel;
    union {
        struct {
            LONGLONG
            DiskMapSize;
            LONGLONG
            DiskDataSize;
        } DiskOverlay;
        struct {
            LONGLONG
            RamDataSize;
        } RamOverlay;
    };
    ULONG
    MemMapSize;
    EWF_VOLUME_DESC
    VolumeDesc;
    EWF_LEVEL_DESC
    LevelDescArray[1];
} EWF_VOLUME_CONFIG, *PEWF_VOLUME_CONFIG;

typedef struct _EWF_VOLUME_DESC {
    WCHAR _ DeviceName[EWF_MAX_DEVICE_NAME_LENGTH];
    UCHAR
    VolumeID[EWF_VOLUME_ID_SIZE];
} EWF_VOLUME_DESC, *PEWF_VOLUME_DESC;

typedef struct _EWF_VOLUME_NAME_ENTRY {
    struct _EWF_VOLUME_NAME_ENTRY* Next;
    WCHAR
    Name[1];
} EWF_VOLUME_NAME_ENTRY, *PEWF_VOLUME_NAME_ENTRY;

HRESULT WINAPI ConfigureEwf(VOID);
BOOL EwfMgrActivateHorm(HANDLE hDevice);
BOOL EwfMgrCheckpoint(HANDLE hDevice, OPTIONAL LPCWSTR lpDescription);
BOOL EwfMgrClearCommand(HANDLE hDevice);
BOOL EwfMgrClose(HANDLE hDevice);
BOOL EwfMgrCommit(HANDLE hDevice);
BOOL WINAPI EwfMgrCommitAndDisableLive(IN HANDLE hDevice, );
BOOL WINAPI EwfMgrCommitFile(IN HANDLE hDevice, IN LPWSTR lpFile);
BOOL EwfMgrDeactivateHorm(HANDLE hDevice);
BOOL EwfMgrDisable(HANDLE hDevice, BOOLfCommit);
BOOL EwfMgrEnable(HANDLE hDevice);
WCHAR EwfMgrGetDriveLetterFromVolumeName(LPCWSTR lpVolumeName);
PEWF_OVERLAY_STORE_CONFIG EwfMgrGetOverlayStoreConfig(HANDLE hDevice);
BOOL EwfMgrGetPeristentData(HANDLE hDevice, LPBYTElpPersistentData, DWORDcbPersistentData);
PEWF_VOLUME_CONFIG EwfMgrGetProtectedVolumeConfig(HANDLE hDevice);
PEWF_VOLUME_NAME_ENTRY EwfMgrGetProtectedVolumeList(VOID);
BOOL EwfMgrIsHormActivated(HANDLE hDevice);
HANDLE EwfMgrOpenOverlayStore(BOOL fOpenForAsyncIO);
HANDLE EwfMgrOpenProtected(LPCWSTR lpVolume);
BOOL EwfMgrRegisterLowSpaceNotification(HANDLE hDevice, LONGLONGFreeBytesRemaining, LPOVERLAPPEDlpOverlapped);
BOOL EwfMgrRestore(HANDLE hDevice);
BOOL EwfMgrSetLevel(HANDLE hDevice, OPTIONAL LPCWSTRlpDescription, intLevel, BOOLfDeleteLevel);
BOOL EwfMgrSetPeristentData(HANDLE hDevice, LPBYTE lpPersistentData, DWORD cbPersistentData);
VOID EwfMgrVolumeNameEntryPop(PEWF_VOLUME_NAME_ENTRY* ppVolumeNameList);
VOID EwfMgrVolumeNameListDelete(PEWF_VOLUME_NAME_ENTRY pVolumeNameList);
BOOL EwfMgrVolumeNameListIsEmpty(PEWF_VOLUME_NAME_ENTRY pVolumeNameList);
