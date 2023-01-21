#include "mxm.h"

#define TICK_MS 16

void mxmBeforeBinaryCallback(pcpa_t* stream, MX_MASTER* mxMaster) {
    pcpaSetSendBinaryBuffer(stream, mxMaster->m_binaryMessage, mxMaster->m_binaryMessageLen);
}
void mxmAfterBinaryCallback(pcpa_t* stream, MX_MASTER* mxMaster) {
    mxMaster->m_binaryMessage[0] = '\0';
    mxMaster->m_binaryMessageLen = 0;
}

int mxmInit(MX_MASTER* mxMaster) {

}

e_pcpa_t mxmPcpStreamInit(MX_MASTER* mxMaster) { return 0; }

e_pcpa_t mxmPcpServer(MX_MASTER* mxMaster) {
    int err;
    if (mxMaster->m_serverState == 1) {
        err = mxmPcpStreamInit(mxMaster);
        if (err == 0) {
            mxMaster->m_serverState = 2;
            return err;
        }
    } else {
        if (mxMaster->m_serverState != 2) {
            return (mxMaster->m_serverState == 0) ? e_pcpa_cannot_open : e_pcpa_not_open;
        }
        err = pcpaServer(&mxMaster->m_pcp, TICK_MS);
        if (err == e_pcpa_to || err == e_pcpa_closed) err = e_pcpa_ok;

        if (err) {
            amiDebugLog("Error pcpaServer. Code %d", err);
            pcpaClose(&mxMaster->m_pcp);
            mxMaster->m_serverState = 1;
        }
    }

    return err;
}
