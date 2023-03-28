#include "callbacks.h"

void mxkPcpDsCompute(pcpa_t* stream, void* data) { pcpaSetSendPacket(stream, "code", "54"); }
void mxkPcpSsdProof(pcpa_t* stream, void* data) { pcpaSetSendPacket(stream, "code", "54"); }
void mxkPcpSsdHostProof(pcpa_t* stream, void* data) {}
void mxkPcpEncrypt(pcpa_t* stream, void* data) {
    pcpaSetSendPacket(stream, KC_ENCRYPT, "B6787E941C3956EAC70095D6A91E635C");
}
void mxkPcpDecrypt(pcpa_t* stream, void* data) {
    pcpaSetSendPacket(stream, KC_DECRYPT, "68271898FB92ABFB49321FAF72F04FF1");
}
void mxkPcpSetIv(pcpa_t* stream, void* data) { pcpaSetSendPacket(stream, KC_SETIV, "1"); }
