#include "mxkPacket.h"

#include <Windows.h>

#include "../ami/ami.h"

void inline _mxkPacketInjectJunk(unsigned char* packet, size_t i) {
    FILETIME filetime;
    amtime_t now;

    for (; i < 16; i++) {
        amiTimerGet(&now);
        GetSystemTimeAsFileTime(&filetime);
        packet[i] = (filetime.dwHighDateTime & 0xff) ^ (filetime.dwLowDateTime & 0xff) ^
                    (now.microseconds & 0xff);
    }
}

MXK_STATUS mxkPacketReqSetKeyS(unsigned char* packet) {
    if (packet == NULL) return MXK_STATUS_INVALID_PARAM_1;
    packet[0] = SetKeyS;
    _mxkPacketInjectJunk(packet, 1);
    return MXK_STATUS_OK;
}
MXK_STATUS mxkPacketReqSetKeyR(unsigned char* packet) {
    if (packet == NULL) return MXK_STATUS_INVALID_PARAM_1;
    packet[0] = SetKeyR;
    _mxkPacketInjectJunk(packet, 1);
    return MXK_STATUS_OK;
}
MXK_STATUS mxkPacketReqGetAppBootInfo(unsigned char* packet) {
    if (packet == NULL) return MXK_STATUS_INVALID_PARAM_1;
    packet[0] = GetAppBootInfo;
    packet[1] = 0;
    _mxkPacketInjectJunk(packet, 2);
    return MXK_STATUS_OK;
}
MXK_STATUS mxkPacketReqEepromRead(unsigned char* packet, unsigned char page) {
    if (packet == NULL) return MXK_STATUS_INVALID_PARAM_1;
    packet[0] = EepromRead;
    packet[1] = page;
    _mxkPacketInjectJunk(packet, 2);
    return MXK_STATUS_OK;
}
MXK_STATUS mxkPacketReqGetVersion(unsigned char* packet) {
    if (packet == NULL) return MXK_STATUS_INVALID_PARAM_1;
    packet[0] = KcGetVersion;
    _mxkPacketInjectJunk(packet, 1);
    return MXK_STATUS_OK;
}
MXK_STATUS mxkPacketReqSetMainId(unsigned char* packet) {
    if (packet == NULL) return MXK_STATUS_INVALID_PARAM_1;
    packet[0] = SetMainId;
    _mxkPacketInjectJunk(packet, 1);
    return MXK_STATUS_OK;
}
MXK_STATUS mxkPacketReqGetMainId(unsigned char* packet) {
    if (packet == NULL) return MXK_STATUS_INVALID_PARAM_1;
    packet[0] = GetMainId;
    _mxkPacketInjectJunk(packet, 1);
    return MXK_STATUS_OK;
}
MXK_STATUS mxkPacketReqGetKeyId(unsigned char* packet) {
    if (packet == NULL) return MXK_STATUS_INVALID_PARAM_1;
    packet[0] = GetKeyId;
    _mxkPacketInjectJunk(packet, 1);
    return MXK_STATUS_OK;
}
MXK_STATUS mxkPacketReqGetPlayCounter(unsigned char* packet) {
    if (packet == NULL) return MXK_STATUS_INVALID_PARAM_1;
    packet[0] = GetPlayCounter;
    _mxkPacketInjectJunk(packet, 1);
    return MXK_STATUS_OK;
}
MXK_STATUS mxkPacketReqFlashRead(unsigned char* packet, unsigned int address, unsigned int nbytes) {
    if (packet == NULL) return MXK_STATUS_INVALID_PARAM_1;
    packet[0] = FlashRead;
    packet[1] = address & 0xff;
    packet[2] = (address >> 8) & 0xff;
    packet[3] = (address >> 16) & 0xff;
    packet[4] = nbytes & 0xff;
    packet[5] = (nbytes >> 8) & 0xff;
    packet[6] = (nbytes >> 16) & 0xff;
    _mxkPacketInjectJunk(packet, 7);
    return MXK_STATUS_OK;
}
MXK_STATUS mxkPacketReqNvramRead(unsigned char* packet, unsigned short addr, unsigned char blocks) {
    if (packet == NULL) return MXK_STATUS_INVALID_PARAM_1;
    packet[0] = NvramRead;
    packet[1] = addr & 0xff;
    packet[2] = (addr >> 8) & 0xff;
    packet[3] = blocks;
    _mxkPacketInjectJunk(packet, 4);
    return MXK_STATUS_OK;
}
