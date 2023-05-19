#include "callbacks.h"

void mxkPcpTdRestore(pcpa_t* stream, void* data) {
    char* sUpdate = pcpaGetCommand(stream, "update");
    char* sRestart = pcpaGetCommand(stream, "restart");
    bool update = sUpdate != NULL && strcmp(sUpdate, "1") == 0;
    bool restart = sRestart != NULL && strcmp(sRestart, "1") == 0;

    if (restart) {
        pcpaSetSendPacket(stream, TRA_RESTORE, "1");
    } else {
        pcpaSetSendPacket(stream, TRA_RESTORE, "2");
    }
}
void mxkPcpTdPut(pcpa_t* stream, void* data) {
    char* sPut = pcpaGetCommand(stream, TRA_PUT);

    if (strcmp(sPut, "?")) {
        pcpaSetSendPacket(stream, TRA_PUT, "0");
        return;
    }

    // Process packet maybe?
    pcpaSetSendPacket(stream, TRA_PUT, "0");
}
void mxkPcpTdGet(pcpa_t* stream, void* data) {
    char* sPut = pcpaGetCommand(stream, TRA_GET);
    pcpaSetSendPacket(stream, TRA_GET, "0");
    if (sPut && strcmp(sPut, "?") == 0) {
        pcpaAddSendPacket(stream, "address", "0");
    } else {
        pcpaSetBinaryMode(stream, binary_mode_send);
        pcpaSetBeforeBinaryModeCallBackFunc(stream, mxkBinaryCallback, NULL);

        BINARY_DATA_LEN = 0;
        pcpaSetRecvBinaryBuffer(stream, BINARY_DATA, BINARY_DATA_LEN);
        pcpaAddSendPacket(stream, "port", "40107");
        pcpaAddSendPacket(stream, "size", "0");
    }
}
void mxkPcpTdLogicalErase(pcpa_t* stream, void* data) {
    pcpaSetSendPacket(stream, TRA_LOGICALERASE, "");
}
void mxkPcpTdSectorErase(pcpa_t* stream, void* data) {
    pcpaSetSendPacket(stream, TRA_SECTOREERASE, "");
}
