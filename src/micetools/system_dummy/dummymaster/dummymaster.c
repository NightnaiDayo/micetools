#include "dummymaster.h"

#include "../../lib/ami/amiLog.h"
#include "../../lib/libpcp/libpcp.h"
#include "callbacks.h"

typedef struct {
    pcpa_t m_pcp;
    pcpa_cb_table_t m_pcpCallbacks[MDM_NUM_CALLBACKS];
} mdm_t;

void mdmPcpReconnectUsbDevice(pcpa_t* stream, void* mdm) {
    pcpaSetSendPacket(stream, MXM_RECONNECT_USB, "0");
}
void mdmPcpCheckDevelopMode(pcpa_t* stream, void* mdm) {
    // TODO: Do we want to support the develop flag in dummymaster?
    pcpaSetSendPacket(stream, MXM_DEVELOP, "0");
}
void mdmPcpLogAvailable(pcpa_t* stream, void* mdm) {
    pcpaSetSendPacket(stream, MXM_LOG_AVAILALBE, "0");
}
void mdmPcpOutputLog(pcpa_t* stream, void* mdm) {
    pcpaSetSendPacket(stream, MXM_OUTPUT_LOG, "0");
    pcpaAddSendPacket(stream, "path", "");
}
void mdmPcpEraseLog(pcpa_t* stream, void* mdm) { pcpaSetSendPacket(stream, MXM_ERASE_LOG, "0"); }
void mdmPcpCurrentFgprocess(pcpa_t* stream, void* mdm) {
    // TODO: Handle set
    pcpaSetSendPacket(stream, MXM_FG_CURRENT, "0");  // mxMaster->m_current
}
void mdmPcpNextFgprocess(pcpa_t* stream, void* mdm) {
    // TODO: Handle set, Handle receive if "size" present
    pcpaSetSendPacket(stream, MXM_FG_NEXT, "0");  // mxMaster->m_current
    // mxMaster->m_next
}
void mdmPcpActiveFgprocess(pcpa_t* stream, void* mdm) {
    pcpaSetSendPacket(stream, MXM_FG_ACTIVE, "0");
}
void mdmPcpFaultFgprocess(pcpa_t* stream, void* mdm) {}
void mdmPcpGetStartCount(pcpa_t* stream, void* mdm) {
    pcpaSetSendPacket(stream, MXM_FG_GETCOUNT, "1");
}
void mdmPcpSetStartCount(pcpa_t* stream, void* mdm) {
    pcpaSetSendPacket(stream, MXM_FG_SETCOUNT, "0");
}


void mdmBeforeCb(pcpa_t* stream, void* data) {
    amiDebugLog("in:%s", (char*)data);
}

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
