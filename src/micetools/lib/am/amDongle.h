#pragma once

#include <Windows.h>

#include "../_am.h"
#include "../libpcp/pcpa.h"

typedef struct _AM_DONGLE_DS_CHALLENGE {
    byte challenge[7];
    byte pages[4][20];
} AM_DONGLE_DS_CHALLENGE;

typedef struct _AM_DONGLE_SSD_CHALLENGE {
    byte challenge[16];
    byte response[16];
} AM_DONGLE_SSD_CHALLENGE;

typedef enum {
    AM_DONGLE_SEQ_NONE = 0,
    AM_DONGLE_SEQ_WAIT_OPEN = 1,
    AM_DONGLE_SEQ_OPEN = 2,
    AM_DONGLE_SEQ_BINARY = 5,
    AM_DONGLE_SEQ_SEND_BINARY = 6,
    AM_DONGLE_SEQ_RECV_BINARY = 7,
    AM_DONGLE_SEQ_DONE = 8,
} AM_DONGLE_SEQ;

typedef struct _AM_DONGLE {
    BOOL m_init;
    BOOL done_init;
    BOOL available;
    BOOL develop;
    BOOL auth_ready;
    int authCondition;
    //
    int setupSeq;
    int setupSeqNext;
    int setupRetries;
    AM_DONGLE_SEQ seq;
    int field19_0x40;
    int result;
    //
    int challengeNum;
    ushort version;
    ushort ctrl_port;
    ushort data_port;
    byte requestCode;
    //
    pcpa_t pcpa;
    //
    byte ssdChallenge[16];
    byte ssdResponse[16];
    byte ssdExpResponse[16];
    byte dsChallenge[16];
    byte dsResponse[16];
    byte dsExpResponse[16];
    byte challengePage;
    AM_DONGLE_DS_CHALLENGE dsTable[100];
    AM_DONGLE_SSD_CHALLENGE ssdTable[100];
    void *valueBuffer;
    void *dataBuffer;
    uint *field_0x3444;
} AM_DONGLE;

typedef enum {
    AM_DONGLE_STATUS_OK = 0,
    AM_DONGLE_STATUS_BUSY = 1,
    AM_DONGLE_STATUS_PENDING = 2,
    AM_DONGLE_STATUS_NG = -1,
    AM_DONGLE_STATUS_ERR_INVALID_PARAM = -2,
    AM_DONGLE_STATUS_ERR_NO_INIT = -3,
    AM_DONGLE_STATUS_ERR_ALREADY_INIT = -4,
    AM_DONGLE_STATUS_ERR_PCP = -5,
    AM_DONGLE_STATUS_ERR_COMMAND = -6,
    AM_DONGLE_STATUS_ERR_VERIFY = -7,
    AM_DONGLE_STATUS_ERR_LOG_FULL = -8,
    AM_DONGLE_STATUS_ERR_NO_LOG = -9,
    AM_DONGLE_STATUS_ERR_ONE_WRITE = -10,
    AM_DONGLE_STATUS_ERR_KEYCHIP_DATA = -11,
    AM_DONGLE_STATUS_ERR_KEYCHIP = -12,
    AM_DONGLE_STATUS_ERR_NO_SERVER = -13,
    AM_DONGLE_STATUS_ERR_AUTH_READY = -14,
    AM_DONGLE_STATUS_ERR_NO_COMMAND = -15,
    AM_DONGLE_STATUS_ERR_SYS = -16,
    AM_DONGLE_STATUS_ERR_PRECONDITION = -17,
} AM_DONGLE_STATUS;

typedef enum {
    AM_DONGLE_BLOCK = 0,
    AM_DONGLE_NOBLOCK = 1,
} AM_DONGLE_BLOCKING;

typedef enum {
    AM_DONGLE_REQUEST_SET_IV = 1,
    AM_DONGLE_REQUEST_DECRYPT = 2,
    AM_DONGLE_REQUEST_ENCRYPT = 3,
    AM_DONGLE_REQUEST_GET_GAME_ID = 4,
    AM_DONGLE_REQUEST_GET_SYSTEMFLAG = 5,
    AM_DONGLE_REQUEST_GET_MODEL_TYPE = 6,
    AM_DONGLE_REQUEST_GET_REGION = 7,
    AM_DONGLE_REQUEST_GET_PLATFORM_ID = 8,
    AM_DONGLE_REQUEST_GET_NETWORK_ADDRESS = 9,
    AM_DONGLE_REQUEST_GET_VERSION = 10,
    AM_DONGLE_REQUEST_BILLING_GET_KEYCHIP_ID = 11,
    AM_DONGLE_REQUEST_BILLING_GET_MAIN_ID = 12,
    AM_DONGLE_REQUEST_BILLING_SET_MAIN_ID = 13,
    AM_DONGLE_REQUEST_BILLING_GET_PLAYCOUNT = 14,
    AM_DONGLE_REQUEST_BILLING_ADD_PLAYCOUNT = 15,
    AM_DONGLE_REQUEST_BILLING_GET_PLAYLIMIT = 16,
    AM_DONGLE_REQUEST_BILLING_GET_NEARFULL = 17,
    AM_DONGLE_REQUEST_BILLING_TD_RESTORE = 18,
    AM_DONGLE_REQUEST_BILLING_PUT_TRACEDATA = 19,
    AM_DONGLE_REQUEST_BILLING_TD_GET_LOG_NUM = 20,
    AM_DONGLE_REQUEST_BILLING_TD_GET_FREE_LOG_NUM = 21,
    AM_DONGLE_REQUEST_BILLING_TD_LOGICAL_ERASE = 22,
    AM_DONGLE_REQUEST_BILLING_TD_ERASE_USED_SECTOR = 23,
    AM_DONGLE_REQUEST_BILLING_TD_ERASE_ALL = 24,
    AM_DONGLE_REQUEST_BILLING_TD_RESERVE_ERASABLE_SECTOR = 25,
    AM_DONGLE_REQUEST_GET_DVDFLAG = 26,
    AM_DONGLE_REQUEST_GET_DS_COMPUTE = 27,
    AM_DONGLE_REQUEST_GET_SSD_PROOF = 28,
    // 29
    AM_DONGLE_REQUEST_GET_FORMAT_TYPE = 30,

    AM_DONGLE_APPBOOT_SEED = 0x80 | 0,
    AM_DONGLE_WTF_81 = 0x80 | 1,
    AM_DONGLE_REQUEST_BILLING_GET_TRACEDATA = 0x80 | 2,
    AM_DONGLE_REQUEST_BILLING_GET_SIGNATURE_PK = 0x80 | 3,
    AM_DONGLE_REQUEST_BILLING_GET_CA_CERT = 0x80 | 4,
    AM_DONGLE_WTF_85 = 0x80 | 5,

    AM_DONGLE_WTF_C0 = 0xc0 | 0,
    AM_DONGLE_REQUEST_BILLING_UPDATE_PLAYLIMIT = 0xc0 | 1,
    AM_DONGLE_REQUEST_BILLING_UPDATE_NEARFULL = 0xc0 | 2,
    AM_DONGLE_WTF_C3 = 0xc0 | 3,
} AM_DONGLE_REQUEST;

typedef enum _AM_DONGLE_SETUP_SEQ {
    AM_DONGLE_SETUP_SEQ_NONE = 0,
    AM_DONGLE_SETUP_SEQ_START = 1,
    AM_DONGLE_SETUP_SEQ_WAIT = 2,
    AM_DONGLE_SETUP_SEQ_SEND = 3,
    AM_DONGLE_SETUP_SEQ_RECV = 4,
    AM_DONGLE_SETUP_SEQ_READ_RESPONSE = 5,
    AM_DONGLE_SETUP_SEQ_CLOSE = 6,
    AM_DONGLE_SETUP_SEQ_RETRY = 7,
    AM_DONGLE_SETUP_SEQ_DONE = 8,
    AM_DONGLE_SETUP_SEQ_ERR = 9,
} AM_DONGLE_SETUP_SEQ;

AM_LIB_H_HEADER(amDongle, AM_DONGLE)

AM_DONGLE_STATUS amDongleInit(void);
AM_DONGLE_STATUS amDongleOpen(void);
AM_DONGLE_STATUS amDongleOpenEx(void);
BOOL amDongleIsDevelop(void);
BOOL amDongleIsAvailable(void);
AM_DONGLE_STATUS amDongleCodeToStatus(void);
AM_DONGLE_STATUS amDongleSetupKeychip(void);
AM_DONGLE_STATUS amDongleSendAndReceiveEx(void);
AM_DONGLE_STATUS amDongleResponseCheck(void);
AM_DONGLE_STATUS amDongleExit(void);
AM_DONGLE_STATUS amDongleSetAuthConfig(char *authConfig);

AM_DONGLE_STATUS amDongleGetGameId(char *gameId, AM_DONGLE_BLOCKING blocking);
AM_DONGLE_STATUS amDongleGetPlatformId(char *platformId, AM_DONGLE_BLOCKING blocking);
AM_DONGLE_STATUS amDongleGetSystemFlag(unsigned char *systemFlag, AM_DONGLE_BLOCKING blocking);
AM_DONGLE_STATUS amDongleGetModelType(unsigned char *modelType, AM_DONGLE_BLOCKING blocking);
AM_DONGLE_STATUS amDongleGetRegion(unsigned char *region, AM_DONGLE_BLOCKING blocking);
AM_DONGLE_STATUS amDongleGetNetworkAddress(unsigned int *networkAddress,
                                           AM_DONGLE_BLOCKING blocking);
AM_DONGLE_STATUS amDongleGetVersion(unsigned short *version, AM_DONGLE_BLOCKING blocking);
AM_DONGLE_STATUS amDongleGetPicVersion(unsigned short *picVersion, AM_DONGLE_BLOCKING blocking);

AM_DONGLE_STATUS amDongleBillingGetKeychipId(void *keychipId, AM_DONGLE_BLOCKING blocking);
AM_DONGLE_STATUS amDongleBillingGetPlayCount(unsigned int *playCount, AM_DONGLE_BLOCKING blocking);
AM_DONGLE_STATUS amDongleBillingGetPlayLimit(unsigned int *playLimit, AM_DONGLE_BLOCKING blocking);
AM_DONGLE_STATUS amDongleBillingAddPlayCount(void *playCount, AM_DONGLE_BLOCKING blocking);
AM_DONGLE_STATUS amDongleBillingGetNearfull(unsigned int *nearfull, AM_DONGLE_BLOCKING blocking);
AM_DONGLE_STATUS amDongleEncrypt(unsigned char *pt, unsigned char *ct, AM_DONGLE_BLOCKING blocking);
AM_DONGLE_STATUS amDongleDecrypt(unsigned char *ct, unsigned char *pt, AM_DONGLE_BLOCKING blocking);
AM_DONGLE_STATUS amDongleSetIv(AM_DONGLE_BLOCKING blocking);
AM_DONGLE_STATUS amDongleGetSeed(unsigned char* seed, AM_DONGLE_BLOCKING blocking);

void amDongleBillingGetCaCertification(void);
void amDongleBillingGetMainId(void);
void amDongleBillingGetPlaylimit(void);
void amDongleBillingGetSignaturePubKey(void);
void amDongleBillingGetTraceData(void);
void amDongleBillingPutTraceData(void);
void amDongleBillingSetMainId(void);
void amDongleBillingTdEraseAll(void);
void amDongleBillingTdEraseUsedSector(void);
void amDongleBillingTdGetFreeLogNum(void);
void amDongleBillingTdGetLogNum(void);
void amDongleBillingTdLogicalErase(void);
void amDongleBillingTdReserveErasableSector(void);
void amDongleBillingTdRestore(void);
void amDongleBillingUpdateNearfull(void);
void amDongleBillingUpdatePlaylimit(void);

void amDongleBusy(void);
void amDongleGetAuthCondition(void);
void amDongleGetDsMac(void);
void amDongleGetDvdFlag(void);
void amDongleGetFormatType(void);
void amDongleGetResult(void);
void amDongleGetSsdResponse(void);
void amDongleRequestSsdHostProof(void);
void amDongleSetAuthCondition(void);
void amDongleUpdate(void);
