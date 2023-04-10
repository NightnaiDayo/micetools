#include "callbacks.h"

void mxkPcpVersion(pcpa_t* stream, void* data) {
    char* sCache = pcpaGetCommand(stream, "cache");
    char* sDevice = pcpaGetCommand(stream, "device");

    bool cache = sCache != NULL && strcmp(sCache, "0") == 0;

    char version[5];
    if (sDevice && strcmp(sDevice, "n2") == 0)
        snprintf(version, 5, "%04X", N2_VERSION);
    else
        snprintf(version, 5, "%04X", PIC_VERSION);
    pcpaSetSendPacket(stream, KC_VERSION, version);
}

void mxkPcpStatus(pcpa_t* stream, void* data) {
    pcpaSetSendPacket(stream, KC_STATUS, "available");
}
