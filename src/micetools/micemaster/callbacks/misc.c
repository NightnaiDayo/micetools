#include "../mxm.h"

void mxmPcpReconnectUsbDevice(pcpa_t* stream, MX_MASTER* mxMaster) {
    // Only allowed: 0, 1, 2
    pcpaSetSendPacket(stream, MXM_RECONNECT_USB, "0");
}

void mxmPcpCheckDevelopMode(pcpa_t* stream, MX_MASTER* mxMaster) {
    pcpaSetSendPacket(stream, MXM_DEVELOP, mxMaster->m_develop ? "1" : 0);

    char* command = pcpaGetCommand(stream, MXM_DEVELOP);
    if (command == NULL || strcmp(command, "?") != 0)
        pcpaAddSendPacket(stream, "code", "1");
}
