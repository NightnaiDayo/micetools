#include "dummystorage.h"

#include "../../lib/ami/amiLog.h"
#include "../../lib/libpcp/libpcp.h"

typedef struct {
    pcpa_t m_pcp;
    pcpa_cb_table_t m_pcpCallbacks[1];
} mdsd_t;

void mdsdRequest(pcpa_t* stream, void* mdsd) {
    char* szRequest = pcpaGetCommand(stream, pcpaGetKeyword(stream, 0));

    if (szRequest == NULL) {
        pcpaSetSendPacket(stream, "result", "invalid_request");
        return;
    }
    pcpaSetSendPacket(stream, "response", szRequest);

    if (strcmp(szRequest, "query_storage_status") == 0) {
        // pcpaAddSendPacket(stream, "result", "success");
        pcpaAddSendPacket(stream, "result", "notask");
    } else if (strcmp(szRequest, "query_storage_count") == 0) {
        pcpaAddSendPacket(stream, "result", "success");
        // Under normal operation, this is 0 (no USB inserted).
        pcpaAddSendPacket(stream, "count", "1");
    } else if (strcmp(szRequest, "get_volume") == 0) {
        pcpaAddSendPacket(stream, "result", "success");
        pcpaAddSendPacket(stream, "volume", "SEGA_DL");
    } else {
        pcpaAddSendPacket(stream, "result", "invalid_request");
        // TODO: Remove this once enough has been implemented for most games?
        pcpaPrint(stream);
    }
}
e_pcpa_t mdsPcpStreamInit(mdsd_t* mdsd, unsigned short textPort, unsigned short binaryPort,
                           bool global) {
    e_pcpa_t err;

    err = pcpaInitStream(&mdsd->m_pcp);
    if (err != e_pcpa_ok) {
        amiDebugLog("pcpaInitStream Error. Code:%d", err);
        return err;
    }

    err = pcpaSetCallbackFuncBuffer(&mdsd->m_pcp, mdsd->m_pcpCallbacks, 1);
    if (err != e_pcpa_ok) {
        amiDebugLog("pcpaSetCallBackFuncBuffer Error. Code:%d", err);
        return err;
    }

    pcpaSetCallbackFunc(&mdsd->m_pcp, "request", mdsdRequest, mdsd);

    err = pcpaOpenServerWithBinary(&mdsd->m_pcp, global ? OPEN_MODE_GLOBAL : OPEN_MODE_LOCAL,
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

void miceDummyStorage(unsigned short textPort, unsigned short binaryPort, bool global) {
    mdsd_t* mdsd = malloc(sizeof *mdsd);
    e_pcpa_t err;

    WSADATA wsaData;
    if (WSAStartup(2, &wsaData)) {
        amiDebugLog("WSAStartup Error. Code %d", GetLastError());
        return;
    }

    err = mdsPcpStreamInit(mdsd, textPort, binaryPort, global);
    if (err != e_pcpa_ok) {
        amiDebugLog("mdsdPcpStreamInit Error. Code %d", err);
        return;
    }

    while (1) {
        err = pcpaServer(&mdsd->m_pcp, 16);
        if (err == e_pcpa_to || err == e_pcpa_closed) err = e_pcpa_ok;

        if (err != e_pcpa_ok) {
            amiDebugLog("Error pcpaServer. Code %d", err);
            pcpaClose(&mdsd->m_pcp);
            return;
        }
    }
}
