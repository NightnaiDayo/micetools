#include "mxkTransport.h"

#include "../mice/ioctl.h"
#include "mxkCrypt.h"

// TODO: Don't use puts!
#include <stdio.h>

static BOOL mxkTransportWaitStrobeReady(void);
static BOOL mxkTransportWaitStrobeRelease(void);
static BOOL mxkTransportCtrlPortInAndOut(BYTE flag);
static BOOL mxkTransportCtrlPortInOrOut(BYTE flag);
static BOOL mxkTransportInitPic(void);

static struct {
    BOOL m_bParallelInit;
    HANDLE m_hParallel;
} mxkTransport;

BOOL DO_SLEEP_0 = TRUE;

#define ReadStatus(mxparallel, status, nret) \
    DeviceIoControl(mxparallel, IOCTL_MXPARALLEL_READ_STATUS, NULL, 0, &status, 1, &nret, NULL)

static MXK_TRANSPORT_STATUS mxkTransportWaitStrobeReady(void) {
    BYTE status;
    DWORD nbytes = 0;

    if (!ReadStatus(mxkTransport.m_hParallel, status, nbytes)) {
        amiDebugLog("Error mxkTransportWaitStrobeReady 1");
        return MXK_TRANSPORT_STATUS_NG;
    }
    status &= PARALLEL_STATUS_BUSY;
    if (status != 0) {
        if (!ReadStatus(mxkTransport.m_hParallel, status, nbytes)) {
            amiDebugLog("Error mxkTransportWaitStrobeReady 2");
            return MXK_TRANSPORT_STATUS_NG;
        }
        if ((status & PARALLEL_STATUS_BUSY) != 0) return MXK_TRANSPORT_STATUS_OK;
    }
    return MXK_TRANSPORT_STATUS_BUSY;
}

static MXK_TRANSPORT_STATUS mxkTransportWaitStrobeRelease(void) {
    BYTE status;
    DWORD nbytes = 0;

    if (!ReadStatus(mxkTransport.m_hParallel, status, nbytes)) {
        amiDebugLog("Error mxkTransportWaitStrobeRelease 1");
        return MXK_TRANSPORT_STATUS_NG;
    }
    status &= PARALLEL_STATUS_BUSY;
    if (status == 0) {
        if (!ReadStatus(mxkTransport.m_hParallel, status, nbytes)) {
            amiDebugLog("Error mxkTransportWaitStrobeRelease 2");
            return MXK_TRANSPORT_STATUS_NG;
        }
        if ((status & PARALLEL_STATUS_BUSY) == 0) return MXK_TRANSPORT_STATUS_OK;
    }
    return MXK_TRANSPORT_STATUS_BUSY;
}

static BOOL mxkTransportCtrlPortInAndOut(BYTE flag) {
    BYTE ctrl;
    DWORD nbytes = 0;
    if (!DeviceIoControl(mxkTransport.m_hParallel, IOCTL_MXPARALLEL_READ_CTRL_PORT, NULL, 0, &ctrl,
                         1, &nbytes, NULL)) {
        amiDebugLog("Error mxkTransportCtrlPortInAndOut 1");
        return FALSE;
    }
    ctrl &= flag;
    if (!DeviceIoControl(mxkTransport.m_hParallel, IOCTL_MXPARALLEL_WRITE_CTRL_PORT, &ctrl, 1, NULL,
                         0, &nbytes, NULL)) {
        amiDebugLog("Error mxkTransportCtrlPortInAndOut 2");
        return FALSE;
    }
    return TRUE;
}

static BOOL mxkTransportCtrlPortInOrOut(BYTE flag) {
    BYTE ctrl;
    DWORD nbytes = 0;
    if (!DeviceIoControl(mxkTransport.m_hParallel, IOCTL_MXPARALLEL_READ_CTRL_PORT, NULL, 0, &ctrl,
                         1, &nbytes, NULL)) {
        amiDebugLog("Error mxkTransportCtrlPortInOrOut 1");
        return FALSE;
    }
    ctrl |= flag;
    if (!DeviceIoControl(mxkTransport.m_hParallel, IOCTL_MXPARALLEL_WRITE_CTRL_PORT, &ctrl, 1, NULL,
                         0, &nbytes, NULL)) {
        amiDebugLog("Error mxkTransportCtrlPortInOrOut 2");
        return FALSE;
    }
    return TRUE;
}

MXK_TRANSPORT_STATUS mxkTransportSend(unsigned char *data, DWORD nbytes) {
    DWORD nret;
    BYTE status;
    MXK_TRANSPORT_STATUS err;

    amtime_t start;
    amtime_t now;

    if (!mxkTransport.m_bParallelInit) return MXK_TRANSPORT_STATUS_NO_INIT;
    if (nbytes == 0) return MXK_TRANSPORT_STATUS_OK;

    for (size_t i = 0; i < nbytes; i++) {
        amiTimerGet(&start);
        do {
            if (!ReadStatus(mxkTransport.m_hParallel, status, nret)) {
                amiDebugLog("Error mxkTransportSend 1");
                return MXK_TRANSPORT_STATUS_SEND_NG;
            }
            status &= PARALLEL_STATUS_ACK;
            if (status == 0) break;
            amiTimerGet(&now);
            if (DO_SLEEP_0) Sleep(0);
        } while (amiTimerDiffUsec(&start, &now) < 1000000);

        if (status != 0) {
            amiDebugLog("SEND busy error");
            return MXK_TRANSPORT_STATUS_SEND_NG;
        }

        while (1) {
            err = mxkTransportWaitStrobeRelease();
            if (err == MXK_TRANSPORT_STATUS_OK) break;
            if (err == MXK_TRANSPORT_STATUS_NG) {
                amiDebugLog("Error mxkTransportSend 2");
                return MXK_TRANSPORT_STATUS_SEND_NG;
            }
            amiTimerGet(&now);
            if (DO_SLEEP_0) Sleep(0);
            if (amiTimerDiffUsec(&start, &now) > 999999) {
                amiDebugLog("state_busy != 0 // timeout?");
                return MXK_TRANSPORT_STATUS_SEND_NG;
            }
        }

        if (!DeviceIoControl(mxkTransport.m_hParallel, IOCTL_MXPARALLEL_WRITE_DATA, &data[i], 1,
                             NULL, 0, &nret, NULL)) {
            amiDebugLog("Error mxkTransportSend 3");
            return MXK_TRANSPORT_STATUS_SEND_NG;
        }

        if (!mxkTransportCtrlPortInAndOut(~PARALLEL_CONTROL_BIDI & 0xff)) {
            amiDebugLog("Error mxkTransportSend 4");
            return MXK_TRANSPORT_STATUS_SEND_NG;
        }
        if (!mxkTransportCtrlPortInOrOut(PARALLEL_CONTROL_STROBE)) {
            amiDebugLog("Error mxkTransportSend 5");
            return MXK_TRANSPORT_STATUS_SEND_NG;
        }

        while (1) {
            err = mxkTransportWaitStrobeReady();
            if (err == MXK_TRANSPORT_STATUS_OK) break;
            if (err == MXK_TRANSPORT_STATUS_NG) {
                amiDebugLog("Error mxkTransportSend 6");
                return MXK_TRANSPORT_STATUS_SEND_NG;
            }
            amiTimerGet(&now);
            if (DO_SLEEP_0) Sleep(0);
            if (amiTimerDiffUsec(&start, &now) > 999999'000) {
                amiDebugLog("RA1が1でエラー");
                return MXK_TRANSPORT_STATUS_SEND_NG;
            }
        }

        if (!mxkTransportCtrlPortInOrOut(PARALLEL_CONTROL_BIDI)) {
            amiDebugLog("Error mxkTransportSend 7");
            return MXK_TRANSPORT_STATUS_SEND_NG;
        }
        if (!mxkTransportCtrlPortInAndOut(~PARALLEL_CONTROL_STROBE & 0xff)) {
            amiDebugLog("Error mxkTransportSend 8");
            return MXK_TRANSPORT_STATUS_SEND_NG;
        }
    }
    return MXK_TRANSPORT_STATUS_OK;
}

MXK_TRANSPORT_STATUS mxkTransportRecv(unsigned char *data, DWORD nbytes) {
    BYTE status;
    DWORD nret;
    MXK_TRANSPORT_STATUS err;

    amtime_t now;
    amtime_t start;

    if (!mxkTransport.m_bParallelInit) return MXK_TRANSPORT_STATUS_NO_INIT;
    if (nbytes == 0) return MXK_TRANSPORT_STATUS_OK;

    for (size_t i = 0; i < nbytes; i++) {
        amiTimerGet(&start);
        do {
            if (!ReadStatus(mxkTransport.m_hParallel, status, nret)) {
                amiDebugLog("Error mxkTransportRecv 1");
                return MXK_TRANSPORT_STATUS_RECV_NG;
            }
            status &= PARALLEL_STATUS_ACK;
            if (status != 0) break;
            amiTimerGet(&now);
            if (DO_SLEEP_0 != 0) Sleep(0);
        } while (amiTimerDiffUsec(&start, &now) < 1000000);

        if (status == 0) {
            amiDebugLog("RECV busy error 1");
            return MXK_TRANSPORT_STATUS_RECV_NG;
        }

        while (1) {
            err = mxkTransportWaitStrobeReady();
            if (err == MXK_TRANSPORT_STATUS_OK) break;
            if (err == MXK_TRANSPORT_STATUS_NG) {
                amiDebugLog("Error mxkTransportRecv 2");
                return MXK_TRANSPORT_STATUS_RECV_NG;
            }
            amiTimerGet(&now);
            if (DO_SLEEP_0 != 0) Sleep(0);

            if (amiTimerDiffUsec(&start, &now) > 999999) {
                amiDebugLog("RECV busy error 2");
                return MXK_TRANSPORT_STATUS_RECV_NG;
            }
        }

        if (!mxkTransportCtrlPortInOrOut(PARALLEL_CONTROL_BIDI)) {
            amiDebugLog("Error mxkTransportRecv 3");
            return MXK_TRANSPORT_STATUS_RECV_NG;
        }
        if (!mxkTransportCtrlPortInOrOut(PARALLEL_CONTROL_STROBE)) {
            amiDebugLog("Error mxkTransportRecv 4");
            return MXK_TRANSPORT_STATUS_RECV_NG;
        }

        if (!DeviceIoControl(mxkTransport.m_hParallel, IOCTL_MXPARALLEL_READ_DATA, NULL, 0,
                             &data[i], 1, &nret, NULL)) {
            amiDebugLog("Error mxkTransportRecv 5");
            return MXK_TRANSPORT_STATUS_RECV_NG;
        }

        while (1) {
            err = mxkTransportWaitStrobeRelease();
            if (err == MXK_TRANSPORT_STATUS_OK) break;
            if (err == MXK_TRANSPORT_STATUS_NG) {
                amiDebugLog("Error mxkTransportRecv 6");
                return MXK_TRANSPORT_STATUS_RECV_NG;
            }

            amiTimerGet(&now);
            if (DO_SLEEP_0) Sleep(0);
            if (amiTimerDiffUsec(&start, &now) > 999999) {
                amiDebugLog("RECV end error");
                return MXK_TRANSPORT_STATUS_RECV_NG;
            }
        }
        if (!mxkTransportCtrlPortInAndOut(~PARALLEL_CONTROL_STROBE & 0xff)) {
            amiDebugLog("Error mxkTransportRecv 7");
            return MXK_TRANSPORT_STATUS_RECV_NG;
        }
    }
    return MXK_TRANSPORT_STATUS_OK;
}

static BOOL mxkTransportInitPic(void) {
    BYTE flags = 0;
    DWORD nbytes = 0;
    Sleep(10);
    if (!DeviceIoControl(mxkTransport.m_hParallel, IOCTL_MXPARALLEL_READ_FLAGS, NULL, 0, &flags, 1,
                         &nbytes, NULL)) {
        amiDebugLog("Error KeychipInit 1");
        return FALSE;
    }
    flags = (flags & 0b11111) | (PARALLEL_EC_MODE_BYTE << 5);
    if (!DeviceIoControl(mxkTransport.m_hParallel, IOCTL_MXPARALLEL_WRITE_FLAGS, &flags, 1, NULL, 0,
                         &nbytes, NULL)) {
        amiDebugLog("Error KeychipInit 2");
        return FALSE;
    }
    Sleep(10);
    flags = PARALLEL_CONTROL_BIDI | PARALLEL_CONTROL_INITIALISE;
    if (!DeviceIoControl(mxkTransport.m_hParallel, IOCTL_MXPARALLEL_WRITE_CTRL_PORT, &flags, 1,
                         NULL, 0, &nbytes, NULL)) {
        amiDebugLog("Error KeychipInit 3");
        return FALSE;
    }
    Sleep(10);
    if (!mxkTransportCtrlPortInAndOut(~PARALLEL_CONTROL_INITIALISE & 0xff)) {
        amiDebugLog("Error KeychipInit 4");
        return FALSE;
    }
    Sleep(10);
    if (!mxkTransportCtrlPortInOrOut(PARALLEL_CONTROL_BIDI | PARALLEL_CONTROL_INITIALISE)) {
        amiDebugLog("Error KeychipInit 5");
        return FALSE;
    }
    Sleep(10);
    return TRUE;
}

MXK_TRANSPORT_STATUS mxkTransportInit(void) {
    amtime_t start;
    amtime_t now;

    if (mxkTransport.m_bParallelInit) return 0;

    amiTimerGet(&start);
    do {
        mxkTransport.m_hParallel =
            CreateFileA("\\\\.\\mxparallel", GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (mxkTransport.m_hParallel != INVALID_HANDLE_VALUE) break;
        amiTimerGet(&now);
        Sleep(500);
    } while (amiTimerDiffSec(&start, &now) < 10);

    if (mxkTransport.m_hParallel == INVALID_HANDLE_VALUE) {
        amiDebugLog("Error Timeout CreateFile MxParallel");
        return MXK_TRANSPORT_STATUS_NG;
    }

    if (!mxkTransportInitPic()) {
        amiDebugLog("Error InitPic()");
        return MXK_TRANSPORT_STATUS_NG;
    }
    DO_SLEEP_0 = 1;
    mxkTransport.m_bParallelInit = 1;
    return MXK_TRANSPORT_STATUS_OK;
}

MXK_STATUS mxkSendPacket(const unsigned char *packet) {
    unsigned char encrypted[MXK_BLOCK_SIZE];

    if (packet == NULL) {
        amiDebugLog("Error invalid param!!!");
        return MXK_STATUS_INVALID_PARAM;
    }

    ZeroMemory(encrypted, sizeof encrypted);
    mxkCryptEncryptData(encrypted, packet);
    if (0) {  // TODO: if (mxkCryptEncryptData() != 0)
        int err = 0;
        amiDebugLog("Error mxkCryptEncryptData(), %d", err);
        return MXK_STATUS_CRYPT_NG;
    }
    MXK_TRANSPORT_STATUS status = mxkTransportSend(encrypted, MXK_BLOCK_SIZE);
    if (status != MXK_TRANSPORT_STATUS_OK) return MXK_STATUS_SEND_NG;
    return MXK_STATUS_OK;
}

MXK_STATUS mxkRecvPacket(unsigned char *packet) {
    unsigned char encrypted[MXK_BLOCK_SIZE];

    if (packet == NULL) {
        amiDebugLog("Error invalid param!!!");
        return MXK_STATUS_INVALID_PARAM;
    }

    MXK_TRANSPORT_STATUS status = mxkTransportRecv(encrypted, MXK_BLOCK_SIZE);
    if (status != MXK_TRANSPORT_STATUS_OK) {
        amiDebugLog("Error mxkTransportRecv(), %d", status);
        return MXK_STATUS_RECV_NG;
    }

    ZeroMemory(packet, MXK_BLOCK_SIZE);
    mxkCryptDecryptData(encrypted, packet);
    if (0) {  // TODO: if (mxkCryptDecryptData() != 0)
        int err = 0;
        amiDebugLog("Error mxkCryptDecryptData(), %d", err);
        return MXK_STATUS_CRYPT_NG;
    }
    return MXK_STATUS_OK;
}
