#include "dummyinstaller.h"

#include "../../lib/ami/amiLog.h"
#include "../../lib/libpcp/libpcp.h"

typedef struct {
    pcpa_t m_pcp;
    pcpa_cb_table_t m_pcpCallbacks[1];
} mdi_t;

void mdiPcpRequest(pcpa_t* stream, void* mdi) {
    char* szRequest = pcpaGetCommand(stream, "request");
    pcpaSetSendPacket(stream, "response", szRequest);

    if (strcmp(szRequest, "check_appdata") == 0) {
        pcpaAddSendPacket(stream, "result", "success");
    } else if (strcmp(szRequest, "query_appdata_status") == 0) {
        pcpaAddSendPacket(stream, "result", "success");
        pcpaAddSendPacket(stream, "status", "available");
        pcpaAddSendPacket(stream, "id", "----");
    } else if (strcmp(szRequest, "format_appdata") == 0) {
        char* szId = pcpaGetCommand(stream, "id");
        if (szId) {
            pcpaAddSendPacket(stream, "gameid", szId);
            pcpaAddSendPacket(stream, "result", "success");
        } else {
            pcpaAddSendPacket(stream, "result", "error");
        }
    } else {
        pcpaAddSendPacket(stream, "result", "invalid_request");
        // TODO: Remove this once enough has been implemented for most games?
        pcpaPrint(stream);
    }
}

void mdiBeforeCb(pcpa_t* stream, void* data) {}

e_pcpa_t mdiPcpStreamInit(mdi_t* mdi, unsigned short textPort, unsigned short binaryPort,
                          bool global) {
    e_pcpa_t err;

    err = pcpaInitStream(&mdi->m_pcp);
    if (err != e_pcpa_ok) {
        amiDebugLog("pcpaInitStream Error. Code:%d", err);
        return err;
    }
    // mdi->m_pcp.before_cb = mdiBeforeCb;

    err = pcpaSetCallbackFuncBuffer(&mdi->m_pcp, mdi->m_pcpCallbacks, 1);
    if (err != e_pcpa_ok) {
        amiDebugLog("pcpaSetCallBackFuncBuffer Error. Code:%d", err);
        return err;
    }

    pcpaSetCallbackFunc(&mdi->m_pcp, "request", mdiPcpRequest, mdi);

    err = pcpaOpenServerWithBinary(&mdi->m_pcp, global ? OPEN_MODE_GLOBAL : OPEN_MODE_LOCAL,
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

void miceDummyInstaller(unsigned short textPort, unsigned short binaryPort, bool global) {
    mdi_t* mdi = malloc(sizeof *mdi);
    e_pcpa_t err;

    WSADATA wsaData;
    if (WSAStartup(2, &wsaData)) {
        amiDebugLog("WSAStartup Error. Code %d", GetLastError());
        return;
    }

    err = mdiPcpStreamInit(mdi, textPort, binaryPort, global);
    if (err != e_pcpa_ok) {
        amiDebugLog("mdiPcpStreamInit Error. Code %d", err);
        return;
    }

    while (1) {
        err = pcpaServer(&mdi->m_pcp, 16);
        if (err == e_pcpa_to || err == e_pcpa_closed) err = e_pcpa_ok;

        if (err != e_pcpa_ok) {
            amiDebugLog("Error pcpaServer. Code %d", err);
            pcpaClose(&mdi->m_pcp);
            return;
        }
    }
}
