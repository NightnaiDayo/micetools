#include "mxk.h"

BOOL mxkExchengeAesKey(HANDLE mxparallel) {
    FILETIME filetime;
    amtime_t now;

    unsigned char key_s[16];
    unsigned char key_r[16];

    size_t i;
    for (i = 0; i < 16; i++) {
        amiTimerGet(&now);
        GetSystemTimeAsFileTime(&filetime);
        key_s[i] = (filetime.dwHighDateTime & 0xff) ^ (filetime.dwLowDateTime & 0xff) ^
                   (now.microseconds & 0xff);
    }

    for (i = 0; i < 16; i++) {
        amiTimerGet(&now);
        GetSystemTimeAsFileTime(&filetime);
        key_r[i] = (filetime.dwHighDateTime & 0xff) ^ (filetime.dwLowDateTime & 0xff) ^
                   (now.microseconds & 0xff);
    }

    unsigned char packet[16];

    mxkPacketReqSetKeyS(packet);
    if (!mxkSendPacket(mxparallel, packet)) return FALSE;
    if (!mxkSendPacket(mxparallel, key_s)) return FALSE;
    if (!mxkRecvPacket(mxparallel, packet)) return FALSE;
    if (packet[0] != SetKeyS) return FALSE;
    mxkSetKeyS(key_s);

    mxkPacketReqSetKeyR(packet);
    if (!mxkSendPacket(mxparallel, packet)) return FALSE;
    if (!mxkSendPacket(mxparallel, key_r)) return FALSE;
    mxkSetKeyR(key_r);
    if (!mxkRecvPacket(mxparallel, packet)) return FALSE;

    return TRUE;
}

BOOL mxkVersion(HANDLE mxparallel, unsigned short* version) {
    unsigned char packet[16];

    mxkPacketReqGetVersion(packet);
    if (!mxkSendPacket(mxparallel, packet)) return FALSE;
    if (!mxkRecvPacket(mxparallel, packet)) return FALSE;

    *version = ((unsigned short*)packet)[0];
    return TRUE;
}

BOOL mxkSetMainId(HANDLE mxparallel, const unsigned char* main_id) {
    unsigned char packet[16];

    mxkPacketReqSetMainId(packet);
    if (!mxkSendPacket(mxparallel, packet)) return FALSE;
    if (!mxkSendPacket(mxparallel, main_id)) return FALSE;

    mxkRecvPacket(mxparallel, packet);

    return packet[0] != 0xff;
}
BOOL mxkGetMainId(HANDLE mxparallel, unsigned char* main_id) {
    unsigned char packet[16];

    mxkPacketReqGetMainId(packet);
    if (!mxkSendPacket(mxparallel, packet)) return FALSE;
    if (!mxkRecvPacket(mxparallel, main_id)) return FALSE;

    return TRUE;
}
BOOL mxkGetKeyId(HANDLE mxparallel, unsigned char* key_id) {
    unsigned char packet[16];

    mxkPacketReqGetKeyId(packet);
    if (!mxkSendPacket(mxparallel, packet)) return FALSE;
    if (!mxkRecvPacket(mxparallel, key_id)) return FALSE;

    return TRUE;
}
BOOL mxkGetAppBootInfo(HANDLE mxparallel, appboot_t* appboot) {
    unsigned char packet[16];

    mxkPacketReqGetAppBootInfo(packet);
    if (!mxkSendPacket(mxparallel, packet)) return FALSE;
    for (int i = 0; i < sizeof *appboot; i += 16) {
        if (!mxkRecvPacket(mxparallel, (unsigned char*)appboot + i)) return FALSE;
    }
    return TRUE;
}
BOOL mxkGetPlayCounter(HANDLE mxparallel, DWORD* play_counter) {
    unsigned char packet[16];

    mxkPacketReqGetPlayCounter(packet);
    if (!mxkSendPacket(mxparallel, packet)) return FALSE;
    if (!mxkRecvPacket(mxparallel, packet)) return FALSE;

    *play_counter = ((DWORD*)packet)[0];
    return TRUE;
}
BOOL mxkFlashRead(HANDLE mxparallel, unsigned int address, unsigned int nbytes,
                  unsigned char* buffer) {
    unsigned char packet[16];
    mxkPacketReqFlashRead(packet, address, nbytes);
    if (!mxkSendPacket(mxparallel, packet)) return FALSE;

    for (size_t i = 0; i < nbytes; i += 0x100) {
        unsigned int rest = (nbytes - i) > 0x100 ? 0x100 : (nbytes - i);
        if (!mxkTransportRecv(mxparallel, buffer + i, rest)) return FALSE;
    }
    return TRUE;
}
BOOL mxkEepromRead(HANDLE mxparallel, unsigned char page, unsigned char* data) {
    unsigned char packet[16];
    mxkPacketReqEepromRead(packet, page);
    if (!mxkSendPacket(mxparallel, packet)) return FALSE;
    if (!mxkRecvPacket(mxparallel, data)) return FALSE;
    return TRUE;
}
BOOL mxkNvramRead(HANDLE mxparallel, unsigned short addr, unsigned char blocks, unsigned char* data) {
    unsigned char packet[16];
    mxkPacketReqNvramRead(packet, addr, blocks);
    if (!mxkSendPacket(mxparallel, packet)) return FALSE;

    for (size_t i = 0; i < blocks; i++) {
        if (!mxkRecvPacket(mxparallel, data + (i * 16))) return FALSE;
    }
    return TRUE;
}
