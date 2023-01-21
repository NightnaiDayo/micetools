#include "mxk.h"

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

void mxkPacketReqSetKeyS(unsigned char* packet) {
    packet[0] = SetKeyS;
    _mxkPacketInjectJunk(packet, 1);
}
void mxkPacketReqSetKeyR(unsigned char* packet) {
    packet[0] = SetKeyR;
    _mxkPacketInjectJunk(packet, 1);
}
void mxkPacketReqGetAppBootInfo(unsigned char* packet) {
    packet[0] = GetAppBootInfo;
    packet[1] = 0;
    _mxkPacketInjectJunk(packet, 2);
}
void mxkPacketReqEepromRead(unsigned char* packet, unsigned char page) {
    packet[0] = EepromRead;
    packet[1] = page;
    _mxkPacketInjectJunk(packet, 2);
}
void mxkPacketReqGetVersion(unsigned char* packet) {
    packet[0] = KcGetVersion;
    _mxkPacketInjectJunk(packet, 1);
}
void mxkPacketReqSetMainId(unsigned char* packet) {
    packet[0] = SetMainId;
    _mxkPacketInjectJunk(packet, 1);
}
void mxkPacketReqGetMainId(unsigned char* packet) {
    packet[0] = GetMainId;
    _mxkPacketInjectJunk(packet, 1);
}
void mxkPacketReqGetKeyId(unsigned char* packet) {
    packet[0] = GetKeyId;
    _mxkPacketInjectJunk(packet, 1);
}
void mxkPacketReqGetPlayCounter(unsigned char* packet) {
    packet[0] = GetPlayCounter;
    _mxkPacketInjectJunk(packet, 1);
}
void mxkPacketReqFlashRead(unsigned char* packet, unsigned int address, unsigned int nbytes) {
    packet[0] = FlashRead;
    packet[1] = address & 0xff;
    packet[2] = (address >> 8) & 0xff;
    packet[3] = (address >> 16) & 0xff;
    packet[4] = nbytes & 0xff;
    packet[5] = (nbytes >> 8) & 0xff;
    packet[6] = (nbytes >> 16) & 0xff;
    _mxkPacketInjectJunk(packet, 7);
}
void mxkPacketReqNvramRead(unsigned char* packet, unsigned short addr, unsigned char blocks) {
    packet[0] = NvramRead;
    packet[1] = addr & 0xff;
    packet[2] = (addr >> 8) & 0xff;
    packet[3] = blocks;
    _mxkPacketInjectJunk(packet, 4);
}
