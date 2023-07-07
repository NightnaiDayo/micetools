#include "dummynetwork.h"

#include "../../lib/ami/amiLog.h"
#include "../../lib/libpcp/libpcp.h"
#include "../../lib/mice/config.h"

typedef struct {
    pcpa_t m_pcp;
    pcpa_cb_table_t m_pcpCallbacks[1];
} mdn_t;

void mdnRequest(pcpa_t* stream, void* mdn) {
    char* szRequest = pcpaGetCommand(stream, pcpaGetKeyword(stream, 0));

    if (szRequest == NULL) {
        pcpaSetSendPacket(stream, "result", "invalid_request");
        return;
    }
    pcpaSetSendPacket(stream, "response", szRequest);

    if (strcmp(szRequest, "query_dhcp_status") == 0) {
        pcpaAddSendPacket(stream, "result", "0");
        pcpaAddSendPacket(stream, "dhcp_status", "3");
        return;
    } else if (strcmp(szRequest, "query_nic_status") == 0) {
        char ipAddress[4 * 4];

        pcpaAddSendPacket(stream, "result", "0");
        pcpaAddSendPacket(stream, "status", "1");

        sprintf_s(ipAddress, _countof(ipAddress), "%hhu.%hhu.%hhu.%hhu",
                  (MiceConfig.network.ip_address >> 24) & 0xff,
                  (MiceConfig.network.ip_address >> 16) & 0xff,
                  (MiceConfig.network.ip_address >> 8) & 0xff,
                  MiceConfig.network.ip_address & 0xff);
        pcpaAddSendPacket(stream, "ip_address", ipAddress);
        sprintf_s(ipAddress, _countof(ipAddress), "%hhu.%hhu.%hhu.%hhu",
                  (MiceConfig.network.subnet_mask >> 24) & 0xff,
                  (MiceConfig.network.subnet_mask >> 16) & 0xff,
                  (MiceConfig.network.subnet_mask >> 8) & 0xff,
                  MiceConfig.network.subnet_mask & 0xff);
        pcpaAddSendPacket(stream, "subnetmask", ipAddress);
        sprintf_s(ipAddress, _countof(ipAddress), "%hhu.%hhu.%hhu.%hhu",
                  (MiceConfig.network.gateway >> 24) & 0xff,
                  (MiceConfig.network.gateway >> 16) & 0xff,
                  (MiceConfig.network.gateway >> 8) & 0xff, MiceConfig.network.gateway & 0xff);
        pcpaAddSendPacket(stream, "gateway", ipAddress);
        if (MiceConfig.network.primary_dns) {
            sprintf_s(ipAddress, _countof(ipAddress), "%hhu.%hhu.%hhu.%hhu",
                      (MiceConfig.network.primary_dns >> 24) & 0xff,
                      (MiceConfig.network.primary_dns >> 16) & 0xff,
                      (MiceConfig.network.primary_dns >> 8) & 0xff,
                      MiceConfig.network.primary_dns & 0xff);
            pcpaAddSendPacket(stream, "primary_dns", ipAddress);
        } else {
            pcpaAddSendPacket(stream, "primary_dns", "-");
        }
        if (MiceConfig.network.secondary_dns) {
            sprintf_s(ipAddress, _countof(ipAddress), "%hhu.%hhu.%hhu.%hhu",
                      (MiceConfig.network.secondary_dns >> 24) & 0xff,
                      (MiceConfig.network.secondary_dns >> 16) & 0xff,
                      (MiceConfig.network.secondary_dns >> 8) & 0xff,
                      MiceConfig.network.secondary_dns & 0xff);
            pcpaAddSendPacket(stream, "secondary_dns", ipAddress);
        } else {
            pcpaAddSendPacket(stream, "secondary_dns", "-");
        }
        return;
    } else {
        pcpaAddSendPacket(stream, "result", "invalid_request");
        return;
    }
}

e_pcpa_t mdnPcpStreamInit(mdn_t* mdn, unsigned short textPort, unsigned short binaryPort,
                          bool global) {
    e_pcpa_t err;

    err = pcpaInitStream(&mdn->m_pcp);
    if (err != e_pcpa_ok) {
        amiDebugLog("pcpaInitStream Error. Code:%d", err);
        return err;
    }

    err = pcpaSetCallbackFuncBuffer(&mdn->m_pcp, mdn->m_pcpCallbacks, 1);
    if (err != e_pcpa_ok) {
        amiDebugLog("pcpaSetCallBackFuncBuffer Error. Code:%d", err);
        return err;
    }

    pcpaSetCallbackFunc(&mdn->m_pcp, "request", mdnRequest, mdn);

    err = pcpaOpenServerWithBinary(&mdn->m_pcp, global ? OPEN_MODE_GLOBAL : OPEN_MODE_LOCAL,
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

void miceDummyNetwork(unsigned short textPort, unsigned short binaryPort, bool global) {
    mdn_t* mdn = malloc(sizeof *mdn);
    e_pcpa_t err;

    WSADATA wsaData;
    if (WSAStartup(2, &wsaData)) {
        amiDebugLog("WSAStartup Error. Code %d", GetLastError());
        return;
    }

    load_mice_config();

    err = mdnPcpStreamInit(mdn, textPort, binaryPort, global);
    if (err != e_pcpa_ok) {
        amiDebugLog("mdnPcpStreamInit Error. Code %d", err);
        return;
    }

    while (1) {
        err = pcpaServer(&mdn->m_pcp, 16);
        if (err == e_pcpa_to || err == e_pcpa_closed) err = e_pcpa_ok;

        if (err != e_pcpa_ok) {
            amiDebugLog("Error pcpaServer. Code %d", err);
            pcpaClose(&mdn->m_pcp);
            return;
        }
    }
}
