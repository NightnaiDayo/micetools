#include "dummymaster.h"

#include "../../lib/ami/amiLog.h"
#include "../../lib/libpcp/libpcp.h"
#include "callbacks.h"

typedef struct {
    pcpa_t m_pcp;
    pcpa_cb_table_t m_pcpCallbacks[MDM_NUM_CALLBACKS];
} mdm_t;

// USB
void mdmPcpReconnectUsbDevice(pcpa_t* stream, void* mdm) {
    pcpaSetSendPacket(stream, pcpaGetKeyword(stream, 0), "0");
}

// Logs
void mdmPcpEraseLog(pcpa_t* stream, void* mdm) { pcpaSetSendPacket(stream, MXM_ERASE_LOG, "0"); }
void mdmPcpOutputLog(pcpa_t* stream, void* mdm) {
    pcpaSetSendPacket(stream, pcpaGetKeyword(stream, 0), "0");
    pcpaAddSendPacket(stream, "path", "");
}
void mdmPcpLogAvailable(pcpa_t* stream, void* mdm) {
    static bool logs_on = false;

    pcpaSetSendPacket(stream, pcpaGetKeyword(stream, 0), logs_on ? "1" : "0");

    char* command = pcpaGetCommand(stream, pcpaGetKeyword(stream, 0));
    if (command) {
        if (strcmp(command, "0") == 0)
            logs_on = false;
        else if (strcmp(command, "1") == 0)
            logs_on = true;
        else if (strcmp(command, "?") != 0)
            pcpaAddSendPacket(stream, "code", "1");
    }

    pcpaAddSendPacket(stream, "backup_count", "1");
    pcpaAddSendPacket(stream, "clear_count", "100");
    pcpaAddSendPacket(stream, "interval", "60");
}

// Develop flag
void mdmPcpCheckDevelopMode(pcpa_t* stream, void* mdm) {
    // TODO: Do we want to support the develop flag in dummymaster?
    pcpaSetSendPacket(stream, pcpaGetKeyword(stream, 0), "0");
}

// Foreground control
static bool fault = false;
void mdmPcpGetStartCount(pcpa_t* stream, void* mdm) {
    pcpaSetSendPacket(stream, pcpaGetKeyword(stream, 0), "1");
}
void mdmPcpFaultFgprocess(pcpa_t* stream, void* mdm) {
    pcpaSetSendPacket(stream, pcpaGetKeyword(stream, 0), fault ? "1" : "0");
}
static BYTE nextFg = 3;
unsigned char g_szFault[255 + 1];
BYTE g_nFault = 0;

extern HANDLE _MiceLauncherNextEvent;
extern BYTE _MiceLauncherNext;
static void _switchNext() {
    _MiceLauncherNext = nextFg;
    if (_MiceLauncherNextEvent != INVALID_HANDLE_VALUE) SetEvent(_MiceLauncherNextEvent);
}
static void _faultRecv(pcpa_t* stream, void* data) {
    g_szFault[g_nFault] = '\0';
    printf("Received fault: %.*s\n", g_nFault, g_szFault);
    _switchNext();
}

void mdmPcpNextFgprocess(pcpa_t* stream, void* mdm) {
    // TODO: Handle set, Handle receive if "size" present
    char szNext[2];
    szNext[0] = '0' + nextFg;
    szNext[1] = '\0';

    char* szSize = pcpaGetCommand(stream, "size");
    char* szCommand = pcpaGetCommand(stream, pcpaGetKeyword(stream, 0));

    if (szSize != NULL) {
        int nSize = 0;
        if (!szCommand || sscanf_s(szSize, "%d", &nSize) != 1) {
            pcpaSetSendPacket(stream, pcpaGetKeyword(stream, 0), szNext);
            pcpaAddSendPacket(stream, "code", "1");
            return;
        }
        if (strcmp(szCommand, "?") == 0) {
            pcpaSetSendPacket(stream, pcpaGetKeyword(stream, 0), szNext);

            pcpaAddSendPacket(stream, "port", "40101");
            char szSendSize[4];
            sprintf_s(szSendSize, sizeof szSendSize, "%d", g_nFault);
            szSendSize[3] = '\0';
            pcpaAddSendPacket(stream, "size", szSendSize);
            if (g_nFault) {
                // SEND FAULT
                pcpaSetBinaryMode(stream, binary_mode_send);
                pcpaSetSendBinaryBuffer(stream, g_szFault, g_nFault);
            }
            return;
        }

        if (nSize > 255) nSize = 255;
        g_nFault = nSize & 0xff;
        pcpaSetBinaryMode(stream, binary_mode_recv);
        pcpaSetAfterBinaryModeCallBackFunc(stream, _faultRecv, NULL);
        pcpaSetRecvBinaryBuffer(stream, g_szFault, g_nFault);

        // RECEIVE FAULT
        nextFg = szCommand[0] - '0';
        pcpaSetSendPacket(stream, pcpaGetKeyword(stream, 0), szCommand);
        pcpaAddSendPacket(stream, "size", szSize);
        pcpaAddSendPacket(stream, "port", "40101");
        return;
    }

    if (!szCommand) {
        pcpaSetSendPacket(stream, pcpaGetKeyword(stream, 0), "?");
        pcpaAddSendPacket(stream, "code", "1");
        return;
    }
    if (strcmp(szCommand, "?") == 0) {
        pcpaSetSendPacket(stream, pcpaGetKeyword(stream, 0), szNext);
        return;
    }
    pcpaSetSendPacket(stream, pcpaGetKeyword(stream, 0), szCommand);
    nextFg = szCommand[0] - '0';
    _switchNext();
    return;
}
void mdmPcpCurrentFgprocess(pcpa_t* stream, void* mdm) {
    pcpaSetSendPacket(stream, MXM_FG_CURRENT, "1");
}
void mdmPcpActiveFgprocess(pcpa_t* stream, void* mdm) {
    pcpaSetSendPacket(stream, MXM_FG_ACTIVE, "0");
}
void mdmPcpSetStartCount(pcpa_t* stream, void* mdm) {
    pcpaSetSendPacket(stream, MXM_FG_SETCOUNT, "0");
}

void mdmBeforeCb(pcpa_t* stream, void* data) { amiDebugLog("in:%s", (char*)data); }

e_pcpa_t mdmPcpStreamInit(mdm_t* mdm, unsigned short textPort, unsigned short binaryPort,
                          bool global) {
    e_pcpa_t err;

    err = pcpaInitStream(&mdm->m_pcp);
    if (err != e_pcpa_ok) {
        amiDebugLog("pcpaInitStream Error. Code:%d", err);
        return err;
    }
    // mdm->m_pcp.before_cb = mdmBeforeCb;

    err = pcpaSetCallbackFuncBuffer(&mdm->m_pcp, mdm->m_pcpCallbacks, MDM_NUM_CALLBACKS);
    if (err != e_pcpa_ok) {
        amiDebugLog("pcpaSetCallBackFuncBuffer Error. Code:%d", err);
        return err;
    }

    // Misc
    pcpaSetCallbackFunc(&mdm->m_pcp, MXM_RECONNECT_USB, mdmPcpReconnectUsbDevice, mdm);
    pcpaSetCallbackFunc(&mdm->m_pcp, MXM_DEVELOP, mdmPcpCheckDevelopMode, mdm);
    // Logs
    pcpaSetCallbackFunc(&mdm->m_pcp, MXM_LOG_AVAILALBE, mdmPcpLogAvailable, mdm);
    pcpaSetCallbackFunc(&mdm->m_pcp, MXM_OUTPUT_LOG, mdmPcpOutputLog, mdm);
    pcpaSetCallbackFunc(&mdm->m_pcp, MXM_ERASE_LOG, mdmPcpEraseLog, mdm);
    // Foreground control
    pcpaSetCallbackFunc(&mdm->m_pcp, MXM_FG_CURRENT, mdmPcpCurrentFgprocess, mdm);
    pcpaSetCallbackFunc(&mdm->m_pcp, MXM_FG_NEXT, mdmPcpNextFgprocess, mdm);
    pcpaSetCallbackFunc(&mdm->m_pcp, MXM_FG_ACTIVE, mdmPcpActiveFgprocess, mdm);
    pcpaSetCallbackFunc(&mdm->m_pcp, MXM_FG_FAULT, mdmPcpFaultFgprocess, mdm);
    pcpaSetCallbackFunc(&mdm->m_pcp, MXM_FG_GETCOUNT, mdmPcpGetStartCount, mdm);
    pcpaSetCallbackFunc(&mdm->m_pcp, MXM_FG_SETCOUNT, mdmPcpSetStartCount, mdm);

    err = pcpaOpenServerWithBinary(&mdm->m_pcp, global ? OPEN_MODE_GLOBAL : OPEN_MODE_LOCAL,
                                   textPort, binaryPort, 300000);
    if (err != e_pcpa_ok && err != e_pcpa_to) {
        amiDebugLog("pcpaOpenServerWithBinary Error. Code %d", err);
        return e_pcpa_not_open;
    }
    if (global)
        amiDebugLog("Listening on 0.0.0.0:%d (:%d)", textPort, binaryPort);
    else
        amiDebugLog("Listening on 127.0.0.1:%d (:%d)", textPort, binaryPort);
    return e_pcpa_ok;
}

void miceDummyMaster(unsigned short textPort, unsigned short binaryPort, bool global) {
    mdm_t* mdm = malloc(sizeof *mdm);
    e_pcpa_t err;

    WSADATA wsaData;
    if (WSAStartup(2, &wsaData)) {
        amiDebugLog("WSAStartup Error. Code %d", GetLastError());
        return;
    }

    err = mdmPcpStreamInit(mdm, textPort, binaryPort, global);
    if (err != e_pcpa_ok) {
        amiDebugLog("mdmPcpStreamInit Error. Code %d", err);
        return;
    }

    while (1) {
        err = pcpaServer(&mdm->m_pcp, 16);
        if (err == e_pcpa_to || err == e_pcpa_closed) err = e_pcpa_ok;

        if (err != e_pcpa_ok) {
            amiDebugLog("Error pcpaServer. Code %d", err);
            pcpaClose(&mdm->m_pcp);
            return;
        }
    }
}
