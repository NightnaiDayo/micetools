#include "dummykeychip.h"

#include "../../lib/ami/amiLog.h"
#include "../../lib/libpcp/libpcp.h"
#include "../../lib/mice/ipc.h"
#include "callbacks.h"

typedef struct {
    pcpa_t m_pcp;
    pcpa_cb_table_t m_pcpCallbacks[40];
} mdk_t;

void mdkBeforeCb(pcpa_t* stream, void* data) {}

e_pcpa_t mdkPcpStreamInit(mdk_t* mdk, unsigned short textPort, unsigned short binaryPort,
                          bool global) {
    e_pcpa_t err;

    err = pcpaInitStream(&mdk->m_pcp);
    if (err != e_pcpa_ok) {
        amiDebugLog("pcpaInitStream Error. Code:%d", err);
        return err;
    }
    // mdk->m_pcp.before_cb = mdkBeforeCb;

    err =
        pcpaSetCallbackFuncBuffer(&mdk->m_pcp, mdk->m_pcpCallbacks, _countof(mdk->m_pcpCallbacks));
    if (err != e_pcpa_ok) {
        amiDebugLog("pcpaSetCallBackFuncBuffer Error. Code:%d", err);
        return err;
    }

    // Misc
    pcpaSetCallbackFunc(&mdk->m_pcp, KC_VERSION, mdkPcpVersion, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, KC_STATUS, mdkPcpStatus, NULL);
    // Crypto
    pcpaSetCallbackFunc(&mdk->m_pcp, DS_COMPUTE, mdkPcpDsCompute, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, SSD_PROOF, mdkPcpSsdProof, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, SSD_HOSTPROOF, mdkPcpSsdHostProof, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, KC_ENCRYPT, mdkPcpEncrypt, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, KC_DECRYPT, mdkPcpDecrypt, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, KC_SETIV, mdkPcpSetIv, NULL);
    // Appboot
    pcpaSetCallbackFunc(&mdk->m_pcp, AB_GAMEID, mdkPcpAbGameId, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, AB_SYSTEMFLAG, mdkPcpAbSystemFlag, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, AB_MODELTYPE, mdkPcpAbModelType, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, AB_FORMATTYPE, mdkPcpAbFormatType, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, AB_REGION, mdkPcpAbRegion, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, AB_PLATFORMID, mdkPcpAbPlatformId, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, AB_NETWORKADDRESS, mdkPcpAbNetworkAddress, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, AB_DVD, mdkPcpAbDvd, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, AB_SEED, mdkPcpAbSeed, NULL);
    // Billing
    pcpaSetCallbackFunc(&mdk->m_pcp, BIL_KEYID, mdkPcpPbKeyId, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, BIL_MAINID, mdkPcpPbMainId, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, BIL_PLAYCOUNT, mdkPcpPbPlayCount, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, BIL_PLAYLIMIT, mdkPcpPbPlayLimit, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, BIL_NEARFULL, mdkPcpPbNearfull, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, BIL_SIGNATURE, mdkPcpPbSignaturePubKey, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, BIL_CACERT, mdkPcpPbCaCertification, NULL);
    // Tracedata
    pcpaSetCallbackFunc(&mdk->m_pcp, TRA_RESTORE, mdkPcpTdRestore, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, TRA_PUT, mdkPcpTdPut, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, TRA_GET, mdkPcpTdGet, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, TRA_LOGICALERASE, mdkPcpTdLogicalErase, NULL);
    pcpaSetCallbackFunc(&mdk->m_pcp, TRA_SECTOREERASE, mdkPcpTdSectorErase, NULL);
    // Storage
    // pcpaSetCallbackFunc(&mdk->m_pcp, KC_EEPROM, mdkPcpEeprom, NULL);
    // pcpaSetCallbackFunc(&mdk->m_pcp, KC_NVRAM0, mdkPcpNvram, NULL);
    // pcpaSetCallbackFunc(&mdk->m_pcp, KC_NVRAM1, mdkPcpNvram, NULL);
    // pcpaSetCallbackFunc(&mdk->m_pcp, KC_NVRAM2, mdkPcpNvram, NULL);
    // pcpaSetCallbackFunc(&mdk->m_pcp, KC_NVRAM3, mdkPcpNvram, NULL);
    // pcpaSetCallbackFunc(&mdk->m_pcp, KC_NVRAM4, mdkPcpNvram, NULL);
    // pcpaSetCallbackFunc(&mdk->m_pcp, KC_NVRAM5, mdkPcpNvram, NULL);
    // pcpaSetCallbackFunc(&mdk->m_pcp, KC_NVRAM6, mdkPcpNvram, NULL);
    // pcpaSetCallbackFunc(&mdk->m_pcp, KC_NVRAM7, mdkPcpNvram, NULL);
    // pcpaSetCallbackFunc(&mdk->m_pcp, KC_NVRAM8, mdkPcpNvram, NULL);
    // pcpaSetCallbackFunc(&mdk->m_pcp, KC_NVRAM9, mdkPcpNvram, NULL);

    err = pcpaOpenServerWithBinary(&mdk->m_pcp, global ? OPEN_MODE_GLOBAL : OPEN_MODE_LOCAL,
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

void miceDummyKeychip(unsigned short textPort, unsigned short binaryPort, bool global) {
    mdk_t* mdk = malloc(sizeof *mdk);
    e_pcpa_t err;

    MiceIpcSetup(FALSE);

    WSADATA wsaData;
    if (WSAStartup(2, &wsaData)) {
        amiDebugLog("WSAStartup Error. Code %d", GetLastError());
        return;
    }

    err = mdkPcpStreamInit(mdk, textPort, binaryPort, global);
    if (err != e_pcpa_ok) {
        amiDebugLog("mdkPcpStreamInit Error. Code %d", err);
        return;
    }

    while (1) {
        err = pcpaServer(&mdk->m_pcp, 16);
        if (err == e_pcpa_to || err == e_pcpa_closed) err = e_pcpa_ok;

        if (err != e_pcpa_ok) {
            amiDebugLog("Error pcpaServer. Code %d", err);
            pcpaClose(&mdk->m_pcp);
            return;
        }
    }
}
