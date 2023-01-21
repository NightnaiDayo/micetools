#include "../mice/ioctl.h"
#include "mxk.h"

// TODO: Don't use puts!
#include <stdio.h>

BOOL DO_SLEEP_0 = TRUE;


#define ReadStatus(mxparallel, status, nret) \
    DeviceIoControl(mxparallel, IOCTL_MXPARALLEL_READ_STATUS, NULL, 0, &status, 1, &nret, NULL);

BOOL mxkTransportWaitStrobeReady(HANDLE mxparallel) {
    BYTE status;
    DWORD nbytes = 0;

    ReadStatus(mxparallel, status, nbytes);
    status &= 0x80;
    if (status != 0) {
        ReadStatus(mxparallel, status, nbytes);
        if ((status & 0x80) != 0) return FALSE;
    }
    return TRUE;
}

BOOL mxkTransportWaitStrobeRelease(HANDLE mxparallel) {
    BYTE status;
    DWORD nbytes = 0;

    ReadStatus(mxparallel, status, nbytes);
    status &= 0x80;
    if (status == 0) {
        ReadStatus(mxparallel, status, nbytes);
        if ((status & 0x80) == 0) return FALSE;
    }
    return TRUE;
}

void mxkTransportCtrlPortInAndOut(HANDLE mxparallel, BYTE flag) {
    BYTE ctrl;
    DWORD nbytes = 0;
    DeviceIoControl(mxparallel, IOCTL_MXPARALLEL_READ_CTRL_PORT, NULL, 0, &ctrl, 1, &nbytes, NULL);
    ctrl &= flag;
    DeviceIoControl(mxparallel, IOCTL_MXPARALLEL_WRITE_CTRL_PORT, &ctrl, 1, NULL, 0, &nbytes, NULL);
}

void mxkTransportCtrlPortInOrOut(HANDLE mxparallel, BYTE flag) {
    BYTE ctrl;
    DWORD nbytes = 0;
    DeviceIoControl(mxparallel, IOCTL_MXPARALLEL_READ_CTRL_PORT, NULL, 0, &ctrl, 1, &nbytes, NULL);
    ctrl |= flag;
    DeviceIoControl(mxparallel, IOCTL_MXPARALLEL_WRITE_CTRL_PORT, &ctrl, 1, NULL, 0, &nbytes, NULL);
}

BOOL mxkTransportSend(HANDLE mxparallel, unsigned char *data, DWORD nbytes) {
    DWORD nret;
    BYTE status;

    amtime_t start;
    amtime_t now;

    for (size_t i = 0; i < nbytes; i++) {
        amiTimerGet(&start);
        do {
            ReadStatus(mxparallel, status, nret);
            status &= 0x40;
            if (status == 0) break;
            amiTimerGet(&now);
            if (DO_SLEEP_0) Sleep(0);
        } while (amiTimerDiffUsec(&start, &now) < 1000000);

        if (status != 0) {
            puts("SEND busy error");
            return FALSE;
        }

        while (mxkTransportWaitStrobeRelease(mxparallel)) {
            amiTimerGet(&now);
            if (DO_SLEEP_0) Sleep(0);
            if (amiTimerDiffUsec(&start, &now) > 999999) {
                puts("SEND busy error");
                return FALSE;
            }
        }

        DeviceIoControl(mxparallel, IOCTL_MXPARALLEL_WRITE_DATA, &data[i], 1, NULL, 0, &nret, NULL);

        mxkTransportCtrlPortInAndOut(mxparallel, 0xdf);
        mxkTransportCtrlPortInOrOut(mxparallel, 0x01);

        while (mxkTransportWaitStrobeReady(mxparallel)) {
            amiTimerGet(&now);
            if (DO_SLEEP_0) Sleep(0);
            if (amiTimerDiffUsec(&start, &now) > 999999'000) {
                puts("SEND end error");
                return FALSE;
            }
        }

        mxkTransportCtrlPortInOrOut(mxparallel, 0x20);
        mxkTransportCtrlPortInAndOut(mxparallel, 0xfe);
    }
    return TRUE;
}

HRESULT mxkTransportRecv(HANDLE mxparallel, unsigned char *data, DWORD nbytes) {
    BYTE status;
    DWORD nret;

    amtime_t now;
    amtime_t start;

    for (size_t i = 0; i < nbytes; i++) {
        amiTimerGet(&start);
        do {
            ReadStatus(mxparallel, status, nret);
            status &= 0x40;
            if (status != 0) break;
            amiTimerGet(&now);
            if (DO_SLEEP_0 != 0) Sleep(0);
        } while (amiTimerDiffUsec(&start, &now) < 1000000'000);

        if (status == 0) {
            puts("RECV busy error 1");
            return FALSE;
        }

        while (mxkTransportWaitStrobeReady(mxparallel)) {
            amiTimerGet(&now);
            if (DO_SLEEP_0 != 0) Sleep(0);

            if (amiTimerDiffUsec(&start, &now) > 999999'000) {
                puts("RECV busy error 2");
                return FALSE;
            }
        }

        mxkTransportCtrlPortInOrOut(mxparallel, 0x20);
        mxkTransportCtrlPortInOrOut(mxparallel, 0x01);

        DeviceIoControl(mxparallel, IOCTL_MXPARALLEL_READ_DATA, NULL, 0, &data[i], 1, &nret, NULL);

        while (mxkTransportWaitStrobeRelease(mxparallel)) {
            amiTimerGet(&now);
            if (DO_SLEEP_0) Sleep(0);
            if (amiTimerDiffUsec(&start, &now) > 999999) {
                puts("RECV end error");
                return FALSE;
            }
        }
        mxkTransportCtrlPortInAndOut(mxparallel, 0xfe);
    }
    return TRUE;
}

BOOL mxkSendPacket(HANDLE mxparallel, const unsigned char *packet) {
    unsigned char encrypted[16];
    ZeroMemory(encrypted, 16);
    mxkCryptEncryptData(encrypted, packet);
    return mxkTransportSend(mxparallel, encrypted, 0x10);
}

BOOL mxkRecvPacket(HANDLE mxparallel, unsigned char *packet) {
    unsigned char encrypted[16];
    if (!mxkTransportRecv(mxparallel, encrypted, 0x10)) return FALSE;
    mxkCryptDecryptData(encrypted, packet);
    return TRUE;
}

void mxkTransportInitPic(HANDLE mxparallel) {
    BYTE flags = 0;
    DWORD nbytes = 0;
    Sleep(10);
    DeviceIoControl(mxparallel, IOCTL_MXPARALLEL_READ_FLAGS, NULL, 0, &flags, 1, &nbytes, NULL);
    flags = flags & 0x1f | 0x20;
    DeviceIoControl(mxparallel, IOCTL_MXPARALLEL_WRITE_FLAGS, &flags, 1, NULL, 0, &nbytes, NULL);
    Sleep(10);
    flags = 0x24;
    DeviceIoControl(mxparallel, IOCTL_MXPARALLEL_WRITE_CTRL_PORT, &flags, 1, NULL, 0, &nbytes,
                    NULL);
    Sleep(10);
    mxkTransportCtrlPortInAndOut(mxparallel, 0xfb);
    Sleep(10);
    mxkTransportCtrlPortInOrOut(mxparallel, 0x24);
    Sleep(10);
}
