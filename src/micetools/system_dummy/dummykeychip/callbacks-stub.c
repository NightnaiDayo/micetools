#include "callbacks.h"

void mdkPcpVersion(pcpa_t* stream, void* data) { pcpaSetSendPacket(stream, KC_VERSION, "0104"); }

void mdkPcpStatus(pcpa_t* stream, void* data) { pcpaSetSendPacket(stream, KC_STATUS, "available"); }

void mdkPcpAbGameId(pcpa_t* stream, void* data) {
    // TODO: Can we do better?
    // pcpaSetSendPacket(stream, AB_GAMEID, "----");
    pcpaSetSendPacket(stream, AB_GAMEID, "SDEY");
}
void mdkPcpAbSystemFlag(pcpa_t* stream, void* data) {
    // systemflag 24 = billing + allnet, which should suffice for everything
    pcpaSetSendPacket(stream, AB_SYSTEMFLAG, "24");
}
void mdkPcpAbModelType(pcpa_t* stream, void* data) {
    // ST
    pcpaSetSendPacket(stream, AB_MODELTYPE, "2");
}
void mdkPcpAbFormatType(pcpa_t* stream, void* data) {
    pcpaSetSendPacket(stream, AB_FORMATTYPE, "1");
}
void mdkPcpAbRegion(pcpa_t* stream, void* data) {
    // All regions
    pcpaSetSendPacket(stream, AB_REGION, "FF");
}
void mdkPcpAbPlatformId(pcpa_t* stream, void* data) {
    // TODO: We _can_ do better here
    pcpaSetSendPacket(stream, AB_PLATFORMID, "AAS");
}
void mdkPcpAbNetworkAddress(pcpa_t* stream, void* data) {
    pcpaSetSendPacket(stream, AB_NETWORKADDRESS, "192.168.103.0");
}
void mdkPcpAbDvd(pcpa_t* stream, void* data) {
    pcpaSetSendPacket(stream, AB_DVD, "01");
}

void mdkPcpPbKeyId(pcpa_t* stream, void* data) {
    pcpaSetSendPacket(stream, BIL_KEYID, "A72E-01A00000000");
}
void mdkPcpPbMainId(pcpa_t* stream, void* data) {
    pcpaSetSendPacket(stream, BIL_MAINID, "");
}
void mdkPcpPbPlayCount(pcpa_t* stream, void* data) {
    pcpaSetSendPacket(stream, BIL_PLAYCOUNT, "00000000");
}
void mdkPcpPbPlayLimit(pcpa_t* stream, void* data) {
    // 8192 plays before checkin
    pcpaSetSendPacket(stream, BIL_PLAYLIMIT, "00002000");
}
void mdkPcpPbNearfull(pcpa_t* stream, void* data) {
    // Accounting mode = 1, nearfull = 512
    pcpaSetSendPacket(stream, BIL_NEARFULL, "00010200");
}
