#pragma once

#include <Windows.h>

#include "../_am.h"
#include "../libpcp/pcpa.h"
#include "amSysData.h"

AM_LIB_H_HEADER(amInstall, AM_INSTALL)

typedef enum {
    AM_INSTALL_APPLICATION_STATUS_INVALID = -1,
    AM_INSTALL_APPLICATION_STATUS_INACTIVE = 0,
    AM_INSTALL_APPLICATION_STATUS_ACTIVE = 1,
} AM_INSTALL_APPLICATION_STATUS;

typedef enum {
    AM_INSTALL_BLOCK = 0,
    AM_INSTALL_NOBLOCK = 1,
} AM_INSTALL_BLOCKING;

typedef enum {
    AM_INSTALL_FORMAT_STATUS_ERROR = -1,
    AM_INSTALL_FORMAT_STATUS_UNKNOWN = 0,
    AM_INSTALL_FORMAT_STATUS_CHECKING = 1,
    AM_INSTALL_FORMAT_STATUS_FORMATTING = 2,
    AM_INSTALL_FORMAT_STATUS_NEEDED = 3,
    AM_INSTALL_FORMAT_STATUS_AVAILABLE = 4,
    AM_INSTALL_FORMAT_STATUS_RESTORED = 5,
} AM_INSTALL_FORMAT_STATUS;

typedef enum {
    AM_INSTALL_INSTALL_STATUS_INVALID = -1,
    AM_INSTALL_INSTALL_STATUS_EMPTY = 0,
    AM_INSTALL_INSTALL_STATUS_INSTALL = 1,
    AM_INSTALL_INSTALL_STATUS_CHECK = 2,
    AM_INSTALL_INSTALL_STATUS_COMPLETE = 3,
} AM_INSTALL_INSTALL_STATUS;

typedef enum {
    AM_INSTALL_SLOT_INVALID = -1,
    AM_INSTALL_SLOT_ORIGINAL_0 = 0,
    AM_INSTALL_SLOT_ORIGINAL_1 = 1,
    AM_INSTALL_SLOT_ORIGINAL_F = 2,
    AM_INSTALL_SLOT_ORIGINAL_B = 3,
    AM_INSTALL_SLOT_PATCH_0 = 4,
    AM_INSTALL_SLOT_PATCH_1 = 5,
    AM_INSTALL_SLOT_PATCH_F = 6,
    AM_INSTALL_SLOT_PATCH_B = 7,
    AM_INSTALL_SLOT_OS = 8,
    AM_INSTALL_SLOT_APPLICATION = 9,
} AM_INSTALL_SLOT;

typedef enum {
    AM_INSTALL_STATUS_OK = 0,
    AM_INSTALL_STATUS_BUSY = 1,

    AM_INSTALL_STATUS_NG = -1,
    AM_INSTALL_STATUS_ERR_INVALID_PARAM = -2,
    AM_INSTALL_STATUS_ERR_NO_INIT = -3,
    AM_INSTALL_STATUS_ERR_ALREADY_INIT = -4,
    AM_INSTALL_STATUS_ERR_INVALID_COMMAND = -5,
    AM_INSTALL_STATUS_ERR_PCP = -6,
    AM_INSTALL_STATUS_ERR_NO_SERVER = -7,
    AM_INSTALL_STATUS_ERR_NO_STORAGE = -8,
    AM_INSTALL_STATUS_ERR_FORMAT = -9,
    AM_INSTALL_STATUS_ERR_EX_PARTITION = -10,
    AM_INSTALL_STATUS_ERR_READ_STORAGE = -11,
    AM_INSTALL_STATUS_ERR_WRITE_STORAGE = -12,
    AM_INSTALL_STATUS_ERR_SPD_CRC = -13,
    AM_INSTALL_STATUS_ERR_SPD_VERSION = -14,
    AM_INSTALL_STATUS_ERR_SEMAPHORE_ID = -15,
    AM_INSTALL_STATUS_ERR_GET_SEMAPHORE = -16,
    AM_INSTALL_STATUS_ERR_NO_SLOT = -17,
    AM_INSTALL_STATUS_ERR_READ_SLOT = -18,
    AM_INSTALL_STATUS_ERR_WRITE_SLOT = -19,
    AM_INSTALL_STATUS_ERR_DIFF_APPLICATION = -20,
    AM_INSTALL_STATUS_ERR_APPLICATION_SIZE = -21,
    AM_INSTALL_STATUS_ERR_SEGMENT_OFFSET = -22,
    AM_INSTALL_STATUS_ERR_UPDATE_STATUS = -23,
    AM_INSTALL_STATUS_ERR_REQUEST = -24,
    AM_INSTALL_STATUS_ERR_INVALID_LOCK_ID = -25,
    AM_INSTALL_STATUS_ERR_NO_COMMAND = -26,
    AM_INSTALL_STATUS_ERR_SYS = -27,
    AM_INSTALL_STATUS_ERR_PRECONDITION = -28,
} AM_INSTALL_STATUS;

typedef struct {
    AM_INSTALL_INSTALL_STATUS m_Status;
    char m_GameId[8];
    unsigned int m_PackageVersion;
    AM_SYSDATA_TIMESTAMP m_TimeStamp;
    unsigned int m_NumSegs;
    unsigned int m_SegSize;
    char m_PlatformId[4];
    unsigned int m_Flag;
    unsigned int m_OsVersion;
    unsigned int m_OsSegCount;
    AM_SYSDATA_TIMESTAMP m_OrgTimeStamp;
    unsigned int m_OrgPackageVersion;
    unsigned int m_InstalledSegs;
} AM_INSTALL_SLOT_STATUS;

typedef struct {
    unsigned long long epbr;
    unsigned long long original0;
    unsigned long long original1;
    unsigned long long patch0;
    unsigned long long patch1;
    unsigned long long os;
} AM_INSTALL_BOOT_RECORD;

typedef enum {
    AM_INSTALL_SEQ_INVALID = -1,

    AM_INSTALL_SEQ_NONE = 0,
    AM_INSTALL_SEQ_BUSY = 1,
    AM_INSTALL_SEQ_SEND_REQUEST = 2,
    AM_INSTALL_SEQ_RECV_RESPONSE = 3,
    AM_INSTALL_SEQ_CHECK_RESPONSE = 4,
    AM_INSTALL_SEQ_OPEN_BINARY = 5,
    AM_INSTALL_SEQ_SEND_BINARY = 6,
    AM_INSTALL_SEQ_EXIT = 7,
} AM_INSTALL_SEQ;

typedef enum {
    AM_INSTALL_REQUEST_QUERY_SLOT_STATUS = 1,
    AM_INSTALL_REQUEST_INSTALL = 2,
    AM_INSTALL_REQUEST_UNINSTALL = 3,
    AM_INSTALL_REQUEST_CHECK = 4,
    AM_INSTALL_REQUEST_QUERY_APPLICATION_STATUS = 5,
    AM_INSTALL_REQUEST_SET_APPLICATION_STATUS = 6,
    AM_INSTALL_REQUEST_QUERY_SBR_BOOTSLOT = 7,
    AM_INSTALL_REQUEST_SET_SBR_BOOTSLOT = 8,
    AM_INSTALL_REQUEST_QUERY_SEMAPHORE_STATUS = 9,
    AM_INSTALL_REQUEST_GET_SEMAPHORE = 10,
    AM_INSTALL_REQUEST_RELEASE_SEMAPHORE = 11,
    AM_INSTALL_REQUEST_QUERY_SPD = 12,
    AM_INSTALL_REQUEST_QUERY_BR = 13,
    AM_INSTALL_REQUEST_QUERY_APPDATA_STATUS = 14,
    AM_INSTALL_REQUEST_15 = 15,
    AM_INSTALL_REQUEST_16 = 16,
    AM_INSTALL_REQUEST_QUERY_VOLUME_NAME = 17,
    AM_INSTALL_REQUEST_18 = 18,
} AM_INSTALL_REQUEST;

typedef struct AM_INSTALL {
    BOOL m_init;
    AM_INSTALL_SEQ m_nextSeq;
    AM_INSTALL_REQUEST m_requestCode;
    AM_INSTALL_STATUS m_result;
    unsigned int m_sendBufferLen;
    unsigned int m_semid;
    AM_INSTALL_SEQ m_seq;
    pcpa_t m_pcp;
    unsigned int Unk510;
    unsigned short m_cmdPort;
    unsigned short m_dataPort;
    void* m_value;
    void* Unk51c;
} AM_INSTALL;

bool amInstallPcpaGetUlonglong(unsigned long long *dest, char *keyword);
bool amInstallPcpaGetInt(char *keyword, unsigned int *dest);

AM_INSTALL_STATUS amInstallResponseCheckQueryBr(AM_INSTALL_BOOT_RECORD *bootRecord);

AM_INSTALL_STATUS amInstallInit(void);
AM_INSTALL_STATUS amInstallUpdate(void);
AM_INSTALL_STATUS amInstallBusy(void);
AM_INSTALL_STATUS amInstallSendRequest(void);
AM_INSTALL_STATUS amInstallRecvResponse(void);
AM_INSTALL_STATUS amInstallCodeToStatus(void);
AM_INSTALL_STATUS amInstallResponseCheck(void);
AM_INSTALL_STATUS amInstallOpenBinary(void);
AM_INSTALL_STATUS amInstallOpenBinaryEx(void);
AM_INSTALL_STATUS amInstallSendBinaryBuffer(AM_INSTALL_BLOCKING blocking);
AM_INSTALL_STATUS amInstallExit(void);

AM_INSTALL_STATUS amInstallSendAndReceiveEx(void);
AM_INSTALL_STATUS amInstallSendAndRecvEx(void);
AM_INSTALL_STATUS amInstallOpenEx(void);
AM_INSTALL_STATUS amInstallOpen(void);

AM_INSTALL_STATUS amInstallGetBr(void *bootRecord, AM_INSTALL_BLOCKING blocking);
AM_INSTALL_STATUS amInstallGetResult(void);

void amInstallGetAppdataStatus(void);
void amInstallGetApplicationStatus(void);
void amInstallGetSbrBootslot(void);
void amInstallGetSemaphore(void);
void amInstallGetSemaphoreStatus(void);
void amInstallGetSlotStatus(void);
void amInstallGetSpd(void);
void amInstallGetVolumeName(void);
void amInstallPcpAddCommandApplicationStatus(void);
void amInstallPcpAddCommandBootSlot(void);
void amInstallPcpAddCommandForce(void);
void amInstallPcpAddCommandInteger(void);
void amInstallPcpAddCommandSlot(void);
void amInstallPcpAddCommandSlotStatus(void);
void amInstallReleaseSemaphore(void);
void amInstallRequestCheck(void);
void amInstallRequestCheckAppdataInternal(void);
void amInstallRequestFormatAppdata(void);
void amInstallRequestInstall(void);
void amInstallRequestUninstall(void);
void amInstallSetApplicationStatusEx(void);
void amInstallSetSbrBootslot(void);
AM_INSTALL_STATUS amInstallTaskRequest(char*);
AM_INSTALL_STATUS amInstallSuicideSub(char*);
