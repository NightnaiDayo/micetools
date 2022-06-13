#include "callbacks.h"

void mxkPcpDsCompute(pcpa_t* stream, void* data) {
    pcpaSetSendPacket(stream, "code", "54");
}
void mxkPcpSsdProof(pcpa_t* stream, void* data) {
    pcpaSetSendPacket(stream, "code", "54");
}
void mxkPcpSsdHostProof(pcpa_t* stream, void* data) {}
void mkxPcpEncrypt(pcpa_t* stream, void* data) {}
void mxkPcpDecrypt(pcpa_t* stream, void* data) {}
void mxkPcpSetIv(pcpa_t* stream, void* data) {}
