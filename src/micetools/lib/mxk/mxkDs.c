#include <Windows.h>
//

#include "../../dll/devices/smb_ds2460.h"
#include "../../dll/devices/smb_ds28cn01.h"
#include "../ami/ami.h"
#include "mxkDs.h"
#include "mxkSmbus.h"

// TODO: Better name
static unsigned int randomNumberFromTime(void) {
    FILETIME fileTime;
    amtime_t amTime;

    amiTimerGet(&amTime);
    GetSystemTimeAsFileTime(&fileTime);
    return amTime.microseconds ^ fileTime.dwLowDateTime ^ fileTime.dwHighDateTime;
}

bool mxkDsExioWaitNotBusy(void) {
    amtime_t start;
    amtime_t now;

    amiTimerGet(&start);
    while (1) {
        amiTimerGet(&now);
        unsigned char val = 0;
        if (mxkSmbusReadByte(SYS_DS_ADDRESS, &val, 0) == 0) return true;
        if (amiTimerDiffUsec(&start, &now) > 1000000) return false;
        Sleep(10);
    }
}

int mxkDsExioRequestComputeMac(void) {
    if (mxkSmbusRequestMutex() != 0) return -7;

    if (mxkSmbusWriteByte(SYS_DS_ADDRESS, 0x5c, 0x94) != 0) return -9;
    if (!mxkDsExioWaitNotBusy()) return -10;

    return mxkSmbusReleaseMutex();
}

int INT_004ab34c = 1;  // TODO: What?
int mxkDsKeychipComputeMac(unsigned char page, void *challenge, unsigned char *mac, int flag) {
    if (INT_004ab34c == 0) return -3;
    if (mac == NULL || challenge == NULL || page > 3) return -2;

    unsigned char buffer[7];
    unsigned char command_code = (flag ? DS28CN01_COMPUTE_1 : DS28CN01_COMPUTE_2) | (page & 3);

    memcpy_s(buffer, sizeof buffer, challenge, 7);
    if (mxkSmbusRequestMutex() != 0) return -7;

    int error = 0;
    int status = mxkSmbusI2CWriteCommand(command_code, buffer, sizeof buffer);
    if (status == 0) {
        Sleep(16);
        status = mxkDsWaitNotBusy(&INT_004ab34c);
        if (status == 0) {
            if (!mxkSmbusI2CReadBlock(KC_DS_ADDRESS, DS28CN01_REG_MAC, mac, 20)) {
                amiDebugLog("Error mxkSmbusI2CReadBlock()!!!");
                error = -8;
            }
        } else {
            error = status;
            amiDebugLog("Error mxkDsKeychipBusy()!!! code:%d", status);
        }
    } else {
        amiDebugLog("Error mxkSmbusI2CWriteCommand()!!!");
        error = -9;
    }

    if (mxkSmbusReleaseMutex() != 0) {
        amiDebugLog("ReleaseMutex Error(%d).\n", GetLastError());
        return -5;
    }
    return error;
}

int mxkDsGetUniqueNumber(unsigned char *param_1) {
    if (INT_004ab34c == 0) return -3;
    if (param_1 == NULL) return -2;

    if (mxkSmbusRequestMutex() != 0) return -7;

    unsigned char cmd_code = 0xa0;
    for (int i = 0; i < 8; i++) {
        if (mxkSmbusReadByte(KC_DS_ADDRESS, &param_1[i], cmd_code) != 0) return -8;
        cmd_code += 1;
    }

    return mxkSmbusReleaseMutex();
}

bool mxkDsMakeSha1InputForVerify(unsigned char page, dsShaPayload_t *payload,
                                 unsigned char *challenge) {
    if (payload == NULL || challenge == NULL) return false;

    unsigned char uniqueId[8] = { 0 };
    if (mxkDsGetUniqueNumber(uniqueId) != 0) {
        amiDebugLog("Error mxkDsGetUniqueNumber()!!!");
        return false;
    }

    unsigned char pageData[32] = { 0 };
    if (mxkDsKeychipReadEeprom(pageData, page) != 0) {
        amiDebugLog("Error mxkDsKeychipReadEeprom()!!!");
        return false;
    }

    unsigned char random[20];
    for (int i = 0; i < 17; i++) random[i] = randomNumberFromTime() & 0xff;

    memcpy(payload->Unk0, random, 4);
    memcpy(payload->m_EepromPage, pageData, sizeof pageData);
    memcpy(payload->m_ChallengeLow, challenge, 4);
    payload->m_Page = page & 3 | 0x40;
    memcpy(payload->m_UniqueId, uniqueId, 7);
    memcpy(payload->Unk30, random + 4, 4);
    memcpy(payload->m_ChallengeHigh, challenge + 4, 3);
    memcpy(payload->Unk37, random + 8, 9);

    return true;
}

int mxkDsWaitNotBusy(int *param_1) {
    if (param_1 == NULL) return -2;

    amtime_t now;
    amtime_t start;
    amiTimerGet(&start);
    amiTimerGet(&now);

    unsigned char status;
    while (1) {
        if (mxkSmbusReadByte(KC_DS_ADDRESS, &status, 0xa8) != 0) return -8;

        if ((status & 2) == 0) return 0;
        if (amiTimerDiffUsec(&start, &now) > 1000000) return -10;

        Sleep(10);
        amiTimerGet(&now);
    }
}

int mxkDsKeychipReadEeprom(unsigned char *pageData, unsigned char page) {
    if (INT_004ab34c == 0) return -3;
    if (pageData == NULL || page > 3) return -2;

    if (mxkSmbusRequestMutex() != 0) return -7;

    int err = 0;
    unsigned char addr = (page << 5) & 0xff;
    for (int i = 0; i < 32; i++) {
        if (mxkSmbusReadByte(KC_DS_ADDRESS, &pageData[i], addr) != 0) {
            err = -8;
            break;
        }
        addr += 1;
    }
    if (mxkSmbusReleaseMutex() != 0) {
        amiDebugLog("ReleaseMutex Error(%d).", GetLastError());
        return -5;
    }
    return err;
}

int mxkDsExioWriteInputBuffer(dsShaPayload_t *buffer) {
    if (INT_004ab34c == 0) return -3;
    if (buffer == NULL) return -2;
    if (mxkSmbusRequestMutex() != 0) return -7;

    int error = 0;
    unsigned char offset = 0;
    for (int i = 0; i < 64;) {
        unsigned char nbytes = (64 - i <= 8) ? 64 - offset : 8;

        if (mxkSmbusI2CWriteBlock(SYS_DS_ADDRESS, offset, &((unsigned char *)buffer)[offset], nbytes) !=
            0) {
            error = -9;
            break;
        }
        if (!mxkDsExioWaitNotBusy()) {
            error = -10;
            break;
        }
        offset += nbytes;
        i = offset & 0xffff;
    }
    if (mxkSmbusReleaseMutex() != 0) {
        amiDebugLog("ReleaseMutex Error(%d).", GetLastError());
        error = -5;
    }
    return (int)error;
}

int mxkDsExioReadMacOutputBuffer(unsigned char *macBuffer) {
    if (INT_004ab34c == 0) return -3;
    if (macBuffer == NULL) return -2;

    if (mxkSmbusRequestMutex() != 0) return -7;

    unsigned char cmd_code = 0x40;
    for (int offset = 0; offset < 20; offset++, cmd_code++) {
        if (mxkSmbusReadByte(SYS_DS_ADDRESS, &macBuffer[offset], cmd_code) != 0) {
            if (!mxkSmbusReleaseMutex()) return -5;
            return -8;
        }
    }

    return mxkSmbusReleaseMutex();
}

int mxkAuthenticationDs(void) {
    FILETIME sysTime;
    amtime_t amTime;
    unsigned char randomMacChallenge[8];
    unsigned char keychipMac[20];
    unsigned char mezzanineMac[20];
    dsShaPayload_t sha1_payload;

    int e = 0;

    for (int i = 0; i < 7; i++) {
        amiTimerGet(&amTime);
        GetSystemTimeAsFileTime(&sysTime);
        randomMacChallenge[i] = (unsigned char)sysTime.dwHighDateTime ^
                                (unsigned char)sysTime.dwLowDateTime ^
                                (unsigned char)amTime.microseconds;
    }
    amiTimerGet(&amTime);
    GetSystemTimeAsFileTime(&sysTime);
    unsigned char page =
        ((unsigned char)sysTime.dwHighDateTime ^ (unsigned char)sysTime.dwLowDateTime ^
         (unsigned char)amTime.microseconds) &
        3;

    if (mxkDsKeychipComputeMac(page, randomMacChallenge, keychipMac, 1) != 0) {
        amiDebugLog("Error mxkDsKeychipComputeMac()!!!");
        return 1;
    }
    if (mxkDsMakeSha1InputForVerify(page, &sha1_payload, randomMacChallenge) == 0) {
        amiDebugLog("Error mxkDsMakeSha1InputForVerify()!!!");
        return 1;
    }

    e = mxkDsExioWriteInputBuffer(&sha1_payload);
    if (e != 0) {
        amiDebugLog("Error mxkDsExioWriteInputBuffer()!!! code:%d", e);
        return 1;
    }

    e = mxkDsExioRequestComputeMac();
    if (e != 0) {
        amiDebugLog("Error mxkDsExioRequestComputeMac()!!! code:%d", e);
        return 1;
    }

    e = mxkDsExioReadMacOutputBuffer(mezzanineMac);
    if (e != 0) {
        amiDebugLog("Error mxkDsExioReadMacOutputBuffer()!!! code:%d", e);
        return 1;
    }

    if (memcmp(keychipMac, mezzanineMac, sizeof keychipMac) == 0) return 0;

    amiDebugLog("Error Mac Verify!!!");
    return 1;
}
