#include <Windows.h>

#include "../comdevice.h"
#include "../common.h"
#include "../input.h"
#include "../key_config.h"
#include "jvs_boards/jvs.h"
#include "mx.h"

#define JVS_SENSE (MiceConfig.keys.board_count == 0 ? FALSE : _MiceJvsBoards[0].m_SenseOut)

JVS_STATUS MiceJvsWrapper(PMICE_JVS this, DWORD nMaxRead) {
    JVS_STATUS status = JVS_STATUS_OK;
    // -1, because we don't want to treat the checksum as a command!
    while (this->m_nInIdx < nMaxRead - 1) {
        JVS_STATUS subStatus = this->vftable->transact(this, MiceJvsRead(this));
        if (subStatus > status) status = subStatus;
    }

    return status;
}

void jvs_send_status(unsigned char status, unsigned char* outData, LPDWORD outCount) {
    short respIndex = 4;

    outData[0] = JVS_SYNC;
    outData[1] = JVS_NODE_MASTER;
    outData[2] = 2;
    if (status == JVS_MARK || status == JVS_SYNC) {
        outData[3] = JVS_MARK;
        outData[4] = status - 1;
        outData[5] = JVS_NODE_MASTER + 2 + status;
        *outCount = 6;
    } else {
        outData[3] = status;
        outData[4] = JVS_NODE_MASTER + 2 + status;
        *outCount = 5;
    }
}

BYTE jvsInUnmasked[MICE_JVS_MAX];
BYTE jvsOutUnmasked[MICE_JVS_MAX];
BYTE jvsOutMasked[MICE_JVS_MASKED];
void mxjvs_handle(unsigned char* lpMaskedIn, short nMaskedCount, unsigned char* outData,
                  short maxOut, LPDWORD outCount) {
    SHORT nIn = 0;
    BOOL wasMark = FALSE;
    for (int i = 0; i < nMaskedCount; i++) {
        if (lpMaskedIn[i] == JVS_MARK) {
            wasMark = TRUE;
            continue;
        }
        if (wasMark)
            jvsInUnmasked[nIn++] = lpMaskedIn[i] + 1;
        else
            jvsInUnmasked[nIn++] = lpMaskedIn[i];
    }
    PJVS_PACKET lpPacket = (PJVS_PACKET)jvsInUnmasked;

    // JVS frame is 4 bytes in total
    if (nIn < 4) {
        log_error(plfMxJvs, "inCount impossibly small: %d", nIn);
        jvs_send_status(JVS_STATUS_UNKNOWN, outData, outCount);
        return;
    }
    // This isn't a JVS packet. We should scan for SYNC, but I don't think we'll ever need to
    if (lpPacket->m_Sync != JVS_SYNC) {
        log_error(plfMxJvs, "SYNC missing. Saw 0x%02x", lpPacket->m_Sync);
        jvs_send_status(JVS_STATUS_UNKNOWN, outData, outCount);
        return;
    }

    // Validate the checksum before proceeding
    unsigned char sum = 0;
    bool escape = false;
    for (int i = 1; i < nIn - 1; i++) sum += jvsInUnmasked[i];
    if (sum != jvsInUnmasked[nIn - 1]) {
        log_error(plfMxJvs, "Checksum failed. Computed 0x%02x, expected 0x%02x", sum,
                  jvsInUnmasked[nIn - 1]);
        jvs_send_status(JVS_STATUS_SUM, outData, outCount);
        return;
    }

    DWORD nBoardCount = MiceConfig.keys.board_count;
    if (nBoardCount > _countof(_MiceJvsBoards)) nBoardCount = _countof(_MiceJvsBoards);

    // Not broadcast, not master-bound, and within bounds
    if (lpPacket->m_Dst != 0xff && (lpPacket->m_Dst == 0 || lpPacket->m_Dst > nBoardCount)) {
        log_warning(plfMxJvs, "JVS bound for %02x ignored", lpPacket->m_Dst);
        *outCount = 0;
        return;
    }

    JVS_STATUS status = JVS_STATUS_UNKNOWN;
    DWORD nBytesWrote = 0;
    if (lpPacket->m_Dst == 0xff) {
        for (DWORD i = 0; i < nBoardCount; i++) {
            PMICE_JVS lpMiceJvs = &_MiceJvsBoards[i];

            lpMiceJvs->m_lpInData = jvsInUnmasked;
            lpMiceJvs->m_lpOutData = jvsOutUnmasked;
            lpMiceJvs->m_nInIdx = 3;
            lpMiceJvs->m_nOutIdx = 4;

            status = MiceJvsWrapper(lpMiceJvs, nIn);
            if (lpMiceJvs->m_nOutIdx > 4) {
                nBytesWrote = lpMiceJvs->m_nOutIdx - 4;
                break;
            }
        }
    } else {
        PMICE_JVS lpMiceJvs = &_MiceJvsBoards[nBoardCount - lpPacket->m_Dst];

        lpMiceJvs->m_lpInData = jvsInUnmasked;
        lpMiceJvs->m_lpOutData = jvsOutUnmasked;
        lpMiceJvs->m_nInIdx = 3;
        lpMiceJvs->m_nOutIdx = 4;

        status = MiceJvsWrapper(lpMiceJvs, nIn);
        nBytesWrote = lpMiceJvs->m_nOutIdx - 4;
    }

    if (status == JVS_STATUS_SILENCE) {
        *outCount = 0;
        return;
    }

    if (status == JVS_STATUS_OK) {
        ((PJVS_PACKET)&jvsOutUnmasked)->m_Dst = JVS_NODE_MASTER;
        ((PJVS_PACKET)&jvsOutUnmasked)->m_nBytes = (nBytesWrote + 2) & 0xff;
        jvsOutUnmasked[3] = status;

        sum = 0;
        for (DWORD i = 1; i < nBytesWrote + 4; i++) sum += jvsOutUnmasked[i];
        jvsOutUnmasked[nBytesWrote + 4] = sum;

        DWORD nPadded = 1;
        jvsOutMasked[0] = JVS_SYNC;
        for (DWORD i = 1; i <= nBytesWrote + 4; i++) {
            if (i == JVS_MARK || i == JVS_SYNC) {
                jvsOutMasked[nPadded++] = JVS_MARK;
                jvsOutMasked[nPadded++] = jvsOutUnmasked[i] - 1;
            } else
                jvsOutMasked[nPadded++] = jvsOutUnmasked[i];
        }

        *outCount = nPadded;
        memcpy_s(outData, maxOut, jvsOutMasked, nPadded);
    } else {
        log_error(plfMxJvs, "JVS board %d returned status %d", lpPacket->m_Dst, status);
        jvs_send_status(status, outData, outCount);
    }
}

BOOL WINAPI mxjvs_DeviceIoControl(file_context_t* ctx, DWORD dwIoControlCode, LPVOID lpInBuffer,
                                  DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                                  LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    switch (dwIoControlCode) {
        case IOCTL_MXJVS_EXCHANGE:
            log_trace(plfMxJvs,
                      "DeviceIoControl(<mxjvs>, <exchange>, 0x%p, 0x%x, -, "
                      "0x%x, -, -)",
                      lpInBuffer, nInBufferSize, nOutBufferSize);

            mxjvs_handle(lpInBuffer, nInBufferSize & 0xffff, lpOutBuffer, nOutBufferSize & 0xFFFF,
                         lpBytesReturned);

            break;
        default:
            log_warning(plfMxJvs, "unhandled 0x%08x", dwIoControlCode);
            return FALSE;
    }

    return TRUE;
}

BOOL mxjvs_SetupComm(void* com, DWORD dwInQueue, DWORD dwOutQueue) { return TRUE; }
BOOL mxjvs_PurgeComm(void* com, DWORD dwFlags) { return TRUE; }
BOOL mxjvs_GetCommState(void* com, LPDCB lpDCB) { return TRUE; }
BOOL mxjvs_SetCommTimeouts(void* com, LPCOMMTIMEOUTS lpDCB) { return TRUE; }

BOOL mxjvs_GetCommModemStatus(void* com, LPDWORD lpModelStat) {
    *lpModelStat = JVS_SENSE ? 0 : MS_DSR_ON;
    return TRUE;
}

BOOL mxjvs_SetCommState(void* com, LPDCB lpDCB) {
    char PARITY[] = { 'N', 'O', 'E', 'M', 'S' };
    char* STOP[] = { "1", "1.5", "2" };
    log_misc(plfMxJvs, "Switching to %d baud (%d%c%s)", lpDCB->BaudRate, lpDCB->ByteSize,
             PARITY[lpDCB->Parity], STOP[lpDCB->StopBits]);
    return TRUE;
}

void init_jvs_boards() {
    // new (MiceJvs837_14895, _MiceJvsBoards[0]);
    new (MiceJvs837_14572, _MiceJvsBoards[0]);

    // Init all the others
    for (int i = 1; i < _countof(_MiceJvsBoards); i++) {
        new (MiceJvs837_14572, _MiceJvsBoards[i]);
    }
}

DWORD __stdcall mxjvs_comdev_thread(com_device_t* com) {
    BYTE inBuffer[512];
    BYTE outBuffer[512];
    DWORD bytesReturned;

    while (1) {
        do {
            comdev_read_blocking(com, inBuffer, 1);
        } while (inBuffer[0] != 0xE0);
        comdev_read_blocking(com, inBuffer + 1, 2);
        BYTE nbytes = inBuffer[2];

        BYTE* bufferPtr = inBuffer + 3;
        if (nbytes == 0xD0) {
            comdev_read_blocking(com, bufferPtr++, 1);
            nbytes = *bufferPtr;
        }
        while (nbytes) {
            comdev_read_blocking(com, bufferPtr++, 1);
            if (bufferPtr[-1] == 0xD0) comdev_read_blocking(com, bufferPtr++, 1);
            nbytes--;
        }

        short packetSize = (bufferPtr - inBuffer) & 0xFFFF;
        mxjvs_handle(inBuffer, packetSize, outBuffer, _countof(outBuffer), &bytesReturned);
        com->modemStatus = JVS_SENSE ? 0 : MS_DSR_ON;
        comdev_write(com, outBuffer, bytesReturned & 0xFFFF);
    }
}

VIRTUAL_SERIAL_DEVICE jvsSerialDevice = {
    .m_Name = "JVS",
    .m_Entrypoint = mxjvs_comdev_thread,
};

void setup_mxjvs() {
    init_jvs_boards();

    file_hook_t* mxjvs = new_file_hook(L"\\\\.\\mxjvs");
    mxjvs->DeviceIoControl = &mxjvs_DeviceIoControl;
    hook_file(mxjvs);

    com_hook_t* jvscom = new_com_hook(4);
    wcscpy_s(jvscom->wName, sizeof jvscom->wName, L"\\\\.\\mxjvs");
    wcscpy_s(jvscom->wDosName, sizeof jvscom->wDosName, L"\\\\.\\mxjvs");
    jvscom->GetCommState = mxjvs_GetCommState;
    jvscom->SetCommState = mxjvs_SetCommState;
    jvscom->SetCommTimeouts = mxjvs_SetCommTimeouts;
    jvscom->SetupComm = mxjvs_SetupComm;
    jvscom->PurgeComm = mxjvs_PurgeComm;
    jvscom->GetCommModemStatus = mxjvs_GetCommModemStatus;

    mxjvs->com_hook = jvscom;

    attach_com_device(4, &jvsSerialDevice);
}
