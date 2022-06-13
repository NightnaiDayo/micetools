#include "callbacks.h"

void mxkPcpEeprom(pcpa_t* stream, void* data) {
    pcpaSetSendPacket(stream, KC_EEPROM, "0");
}

void mxkPcpNvram(pcpa_t* stream, void* data) {
    char* keyword = pcpaGetKeyword(stream, 0);
    pcpaSetSendPacket(stream, keyword, "0");
}
