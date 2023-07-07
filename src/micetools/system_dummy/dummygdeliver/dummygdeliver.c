#include "dummygdeliver.h"

#include "../../lib/ami/amiLog.h"
#include "../../lib/libpcp/libpcp.h"

typedef struct {
    pcpa_t m_pcp;
    pcpa_cb_table_t m_pcpCallbacks[1];
} mdgd_t;

void mdgdRequest(pcpa_t* stream, void* mdgd) {
    char* szRequest = pcpaGetCommand(stream, pcpaGetKeyword(stream, 0));

    if (szRequest == NULL) {
        pcpaSetSendPacket(stream, "result", "invalid_request");
        return;
    }
    pcpaSetSendPacket(stream, "response", szRequest);

    if (strcmp(szRequest, "terminatebeacon") == 0) {
        // pcpaAddSendPacket(stream, "result", "success");
        pcpaAddSendPacket(stream, "result", "error");
        pcpaAddSendPacket(stream, "code", "4");
        return;
    } else {
        pcpaAddSendPacket(stream, "result", "invalid_request");
        // TODO: Remove this once enough has been implemented for most games?
        pcpaPrint(stream);
        return;
    }
}
e_pcpa_t mdgdPcpStreamInit(mdgd_t* mdgd, unsigned short textPort, unsigned short binaryPort,
                           bool global) {
    e_pcpa_t err;

    err = pcpaInitStream(&mdgd->m_pcp);
    if (err != e_pcpa_ok) {
        amiDebugLog("pcpaInitStream Error. Code:%d", err);
        return err;
    }

    err = pcpaSetCallbackFuncBuffer(&mdgd->m_pcp, mdgd->m_pcpCallbacks, 1);
    if (err != e_pcpa_ok) {
        amiDebugLog("pcpaSetCallBackFuncBuffer Error. Code:%d", err);
        return err;
    }

    pcpaSetCallbackFunc(&mdgd->m_pcp, "request", mdgdRequest, mdgd);

    err = pcpaOpenServerWithBinary(&mdgd->m_pcp, global ? OPEN_MODE_GLOBAL : OPEN_MODE_LOCAL,
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

void miceDummyGDeliver(unsigned short textPort, unsigned short binaryPort, bool global) {
    mdgd_t* mdgd = malloc(sizeof *mdgd);
    e_pcpa_t err;

    WSADATA wsaData;
    if (WSAStartup(2, &wsaData)) {
        amiDebugLog("WSAStartup Error. Code %d", GetLastError());
        return;
    }

    err = mdgdPcpStreamInit(mdgd, textPort, binaryPort, global);
    if (err != e_pcpa_ok) {
        amiDebugLog("mdgdPcpStreamInit Error. Code %d", err);
        return;
    }

    while (1) {
        err = pcpaServer(&mdgd->m_pcp, 16);
        if (err == e_pcpa_to || err == e_pcpa_closed) err = e_pcpa_ok;

        if (err != e_pcpa_ok) {
            amiDebugLog("Error pcpaServer. Code %d", err);
            pcpaClose(&mdgd->m_pcp);
            return;
        }
    }
}
