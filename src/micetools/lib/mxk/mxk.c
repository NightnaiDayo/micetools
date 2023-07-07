#include "mxk.h"

unsigned int KEYCHIP_STATUS;
unsigned char KEYCHIP_ERROR;

static MXK_STATUS mxkPacketReqRandomBytes(unsigned char* packet) {
    if (packet == NULL) return MXK_STATUS_INVALID_PARAM;

    amtime_t now;
    FILETIME filetime;
    for (int i = 0; i < MXK_BLOCK_SIZE; i++) {
        amiTimerGet(&now);
        GetSystemTimeAsFileTime(&filetime);
        packet[i] = (filetime.dwHighDateTime & 0xff) ^ (filetime.dwLowDateTime & 0xff) ^
                    (now.microseconds & 0xff);
    }
    return MXK_STATUS_OK;
}

MXK_STATUS mxkExchengeAesKey(void) {
    unsigned char key_s[16];
    unsigned char key_r[16];
    unsigned char packet[16];

    mxkPacketReqRandomBytes(key_s);
    mxkPacketReqRandomBytes(key_r);

    if (mxkPacketReqSetKeyS(packet) != MXK_STATUS_OK) {
        amiDebugLog("Error mxkPacketReqSetKeyS");
        return MXK_STATUS_ERROR;
    }
    if (mxkSendPacket(packet) != MXK_STATUS_OK) {
        amiDebugLog("Error mxkSendPacket Command KeyS");
        return MXK_STATUS_ERROR;
    }
    if (mxkSendPacket(key_s) != MXK_STATUS_OK) {
        amiDebugLog("Error mxkSendPacket KeyS");
        return MXK_STATUS_ERROR;
    }
    if (mxkRecvPacket(packet) != MXK_STATUS_OK || packet[0] != SetKeyS) {
        amiDebugLog("Error mxkRecvPacket KeyS");
        return MXK_STATUS_ERROR;
    }
    mxkSetKeyS(key_s);

    if (mxkPacketReqSetKeyR(packet) != MXK_STATUS_OK) {
        amiDebugLog("Error mxkPacketReqSetKeyR");
        return MXK_STATUS_ERROR;
    }
    if (mxkSendPacket(packet) != MXK_STATUS_OK) {
        amiDebugLog("Error mxkSendPacket Command SetKeyR");
        return MXK_STATUS_ERROR;
    }
    if (mxkSendPacket(key_r) != MXK_STATUS_OK) {
        amiDebugLog("Error mxkSendPacket KeyR");
        return MXK_STATUS_ERROR;
    }
    mxkSetKeyR(key_r);
    if (mxkRecvPacket(packet) != MXK_STATUS_OK) {
        amiDebugLog("Error mxkRecvPacket KeyR");
        return MXK_STATUS_ERROR;
    }

    return MXK_STATUS_OK;
}

BOOL mxkVersionDirty;
unsigned short mxkVersionCache;
MXK_STATUS mxkVersion(unsigned short* version, MXK_CACHE cache, unsigned char* err) {
    unsigned char packet[16];

    if (version == NULL || err == NULL) return MXK_STATUS_INVALID_PARAM;

    if (KEYCHIP_STATUS == 2) {
        amiDebugLog("Error MXK_STATUS_ERROR!!");
        *err = KEYCHIP_ERROR;
        return MXK_STATUS_ERROR;
    }

    if (cache == MXK_CACHE_USE) {
        if (!mxkVersionDirty) {
            *version = mxkVersionCache;
            return MXK_STATUS_OK;
        }
    } else {
        mxkVersionDirty = true;
    }

    mxkPacketReqGetVersion(packet);
    if (mxkSendPacket(packet) != MXK_STATUS_OK) {
        amiDebugLog("Error mxkSendPacket!!");
        goto error;
    }
    if (mxkRecvPacket(packet) != MXK_STATUS_OK) {
        amiDebugLog("Error mxkRecvPacket!!");
        goto error;
    }

    *version = mxkVersionCache = ((unsigned short*)packet)[0];
    mxkVersionDirty = false;
    return MXK_STATUS_OK;

error:
    KEYCHIP_ERROR = 2;
    KEYCHIP_STATUS = 2;
    *err = 2;
    return MXK_STATUS_ERROR;
}

BOOL mxkSetMainId(const unsigned char* main_id) {
    unsigned char packet[16];

    mxkPacketReqSetMainId(packet);
    if (!mxkSendPacket(packet)) return FALSE;
    if (!mxkSendPacket(main_id)) return FALSE;

    mxkRecvPacket(packet);

    return packet[0] != 0xff;
}
BOOL mxkGetMainId(unsigned char* main_id) {
    unsigned char packet[16];

    mxkPacketReqGetMainId(packet);
    if (!mxkSendPacket(packet)) return FALSE;
    if (!mxkRecvPacket(main_id)) return FALSE;

    return TRUE;
}
BOOL mxkGetKeyId(unsigned char* key_id) {
    unsigned char packet[16];

    mxkPacketReqGetKeyId(packet);
    if (!mxkSendPacket(packet)) return FALSE;
    if (!mxkRecvPacket(key_id)) return FALSE;

    return TRUE;
}

BOOL mxkGetPlayCounter(DWORD* play_counter) {
    unsigned char packet[16];

    mxkPacketReqGetPlayCounter(packet);
    if (!mxkSendPacket(packet)) return FALSE;
    if (!mxkRecvPacket(packet)) return FALSE;

    *play_counter = ((DWORD*)packet)[0];
    return TRUE;
}
BOOL mxkFlashRead(unsigned int address, unsigned int nbytes, unsigned char* buffer) {
    unsigned char packet[16];
    mxkPacketReqFlashRead(packet, address, nbytes);
    if (!mxkSendPacket(packet)) return FALSE;

    for (unsigned int i = 0; i < nbytes; i += 0x100) {
        unsigned int rest = (nbytes - i) > 0x100 ? 0x100 : (nbytes - i);
        if (!mxkTransportRecv(buffer + i, rest)) return FALSE;
    }
    return TRUE;
}
BOOL mxkEepromRead(unsigned char page, unsigned char* data) {
    unsigned char packet[16];
    mxkPacketReqEepromRead(packet, page);
    if (!mxkSendPacket(packet)) return FALSE;
    if (!mxkRecvPacket(data)) return FALSE;
    return TRUE;
}
BOOL mxkNvramRead(unsigned short addr, unsigned char blocks, unsigned char* data) {
    unsigned char packet[16];
    mxkPacketReqNvramRead(packet, addr, blocks);
    if (!mxkSendPacket(packet)) return FALSE;

    for (size_t i = 0; i < blocks; i++) {
        if (!mxkRecvPacket(data + (i * 16))) return FALSE;
    }
    return TRUE;
}

bool mxkValidString(const char* string, unsigned int length) {
    for (unsigned int i = 0; i < length; i++) {
        char c = string[i];
        if (isalnum(c)) continue;
        if (c != '.' && c != '_' && c != '-' && c != ':' && c != '@' && c != '%' && c != '/' &&
            c != '\\')
            return false;
    }
    return true;
}

MXK_STATUS mxkGetKeychipIdFromN2(void) {
    N2KeychipId_t kcId;

    memset(&kcId, 0, sizeof kcId);
    int err = mxkN2CmdReadKeychipID(&kcId);
    if (err != 0) {
        amiDebugLog("Error: mxkN2CmdReadKeychipID().ErrorCode %d", err);
        return MXK_STATUS_ERROR;
    }

    amiCrc32RInit();
    uint32_t crcCalc = amiCrc32RCalc((sizeof kcId.m_AppBoot) - 4, (LPBYTE)&kcId.m_AppBoot + 4, 0);
    if (crcCalc != kcId.m_AppBoot.m_Header.m_Crc) {
        amiDebugLog("Error AppBootInfo CRC!!!");
        return MXK_STATUS_ERROR;
    }

    crcCalc = amiCrc32RCalc(sizeof kcId - 4, (LPBYTE)&kcId + 4, 0);
    if (crcCalc != kcId.m_Crc) {
        amiDebugLog("Error KeychipID CRC!!!");
        return MXK_STATUS_ERROR;
    }

    // N2 keychip ID is a smaller, more compact, format than normal appboot. Restructure it to match
    // before committing it to memory!
    unsigned char keyId[16];
    AM_APPBOOT_256 appboot;

    memcpy_s(keyId, sizeof keyId, kcId.m_KeyId, sizeof kcId.m_KeyId);
    memcpy_s(&appboot, sizeof appboot, &kcId.m_AppBoot,
             sizeof kcId.m_AppBoot - sizeof kcId.m_AppBoot.m_Seed);
    memcpy_s(appboot.m_Seed, sizeof appboot.m_Seed, kcId.m_AppBoot.m_Seed,
             sizeof kcId.m_AppBoot.m_Seed);

    printf("Got N2 key ID: %.16s\n", keyId);
    printf("Got N2 game ID: %.4s\n", kcId.m_AppBoot.m_Header.m_GameId);

    // ...but we're not actually committing it to memory at the moment!
    // memcpy_s(&APPBOOT, 256, &appboot, sizeof appboot);
    // memcpy_s(N2_KEYID, 16, keyId, 16);
    // DAT_004adc95 = 0;
    // DAT_004adda4 = 0;
    return MXK_STATUS_OK;
}

MXK_STATUS mxkInit(void) {
    mxkVersionDirty = true;
    AppBoot.m_cacheDirty = true;

    if (mxkTransportInit() != MXK_STATUS_OK) {
        amiDebugLog("Error mxkTransportInit!!");
        return MXK_STATUS_ERROR;
    }
    if (mxkCryptInit() != MXK_STATUS_OK) {
        amiDebugLog("Error mxkCryptInit!!");
        return MXK_STATUS_ERROR;
    }
    if (mxkExchengeAesKey() != MXK_STATUS_OK) {
        amiDebugLog("Error mxkExchengeAesKey!!");
        return MXK_STATUS_ERROR;
    }
    if (mxkSmbusInit() != MXK_STATUS_OK) {
        amiDebugLog("Error mxkSmbusInit!!");
        return MXK_STATUS_ERROR;
    }
    // TODO: SSD
    // mxkSsdInit();
    if (mxkAuthenticationDs() != MXK_STATUS_OK) {
        amiDebugLog("Error mxkAuthenticationDs!!");
        return MXK_STATUS_ERROR;
    }
    if (mxkGetAppBootInfo() != MXK_STATUS_OK) {
        amiDebugLog("Error mxkGetAppBootInfo!!");
        return MXK_STATUS_ERROR;
    }

    unsigned char system_flag;
    unsigned char err;
    if (mxkAbSystemFlag(MXK_CACHE_USE, &system_flag, &err) == MXK_STATUS_OK && system_flag & 1) {
        // appboot_flash();
        // eeprom_playcount();
    }

    if (mxkN2Init() != 0) {
        amiDebugLog("Error mxkN2Init!!");
        return MXK_STATUS_ERROR;
    }

    if (mxkGetKeychipIdFromN2() != 0) {
        amiDebugLog("Error mxkGetKeychipIdFromN2!!");
        return MXK_STATUS_ERROR;
    }

    return MXK_STATUS_OK;
}
