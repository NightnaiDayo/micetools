#include "mxkAb.h"

#include "mxkPacket.h"
#include "mxkTransport.h"

APP_BOOT AppBoot;

MXK_STATUS mxkGetAppBootInfo(void) {
    unsigned char packet[16];
    MXK_STATUS status;

    if (mxkPacketReqGetAppBootInfo(packet) != MXK_STATUS_OK) {
        amiDebugLog("Error mxkPacketReqGetAppBootInfo");
        return MXK_STATUS_ERROR;
    }

    status = mxkSendPacket(packet);
    if (status != MXK_STATUS_OK) {
        amiDebugLog("Error mxkSendPacket GetAppBootInfo");
        return status;
    }

    appboot_t appBoot;
    for (int i = 0; i < sizeof appBoot; i += 16) {
        status = mxkRecvPacket((unsigned char*)(&appBoot) + i);
        if (status != MXK_STATUS_OK) {
            amiDebugLog("Error mxkRecvPacket AppBootInfo");
            return status;
        }
    }

    amiCrc32RInit();
    if (amiCrc32RCalc(sizeof appBoot - 4, (unsigned char*)&appBoot + 4, 0) != appBoot.crc) {
        amiDebugLog("Error CRC AppBootInfo");
        return MXK_STATUS_ERROR;
    }

    memcpy(&AppBoot.m_Cache, &appBoot, sizeof appBoot);
    AppBoot.m_cacheDirty = false;
    return MXK_STATUS_OK;
}

MXK_STATUS mxkAbSystemFlag(MXK_CACHE cache, unsigned char* systemFlag, unsigned char* err) {
    if (systemFlag == NULL || (err == NULL)) return MXK_STATUS_INVALID_PARAM;
    if (KEYCHIP_STATUS == 2) {
        *err = KEYCHIP_ERROR;
        return MXK_STATUS_ERROR;
    }

    MXK_STATUS status;

    *err = 0;
    if (cache == MXK_CACHE_USE) {
        if (AppBoot.m_cacheDirty != true) goto LAB_00401d7b;
    } else {
        AppBoot.m_cacheDirty = true;
    }
    if (!AppBoot.m_useFlash) {
        // TODO: N2
        // if (HAS_N2 == true) {
        //     status = mxkGetKeychipIdFromN2();
        //     if (status == 0) goto LAB_00401d7b;
        //     mxkN2GetErrorCode(status);
        //     if (0 < LOG_EN_MXK) {
        //         log("mxkAbSystemFlag", 0x338, "Error mxkGetKeychipIdFromN2!!\n");
        //     }
        //     goto LAB_00401d64;
        // }
        status = mxkGetAppBootInfo();
    } else {
        status = MXK_STATUS_ERROR;
        // TODO: This
        // status = appboot_flash();
    }

    if (status == MXK_STATUS_OK) {
    LAB_00401d7b:
        if (AppBoot.m_Cache.format != 1) {
            *systemFlag = 0;
            return MXK_STATUS_OK;
        }
        *systemFlag = AppBoot.m_Cache.system_flag;
        return MXK_STATUS_OK;
    }

    // LAB_00401d64:
    KEYCHIP_ERROR = 2;
    KEYCHIP_STATUS = 2;
    *err = 2;
    return MXK_STATUS_ERROR;
}
MXK_STATUS mxkAbGameId(MXK_CACHE cache, char* gameId, unsigned char* err) {
    if (gameId == NULL || (err == NULL)) return MXK_STATUS_INVALID_PARAM;
    if (KEYCHIP_STATUS == 2) {
        *err = KEYCHIP_ERROR;
        return MXK_STATUS_ERROR;
    }

    *err = 0;
    if (cache == false) {
        if (AppBoot.m_cacheDirty != true) goto LAB_00401c7f;
    } else {
        AppBoot.m_cacheDirty = true;
    }

    MXK_STATUS status;
    if (!AppBoot.m_useFlash) {
        // if (HAS_N2 == true) {
        //     status = mxkGetKeychipIdFromN2();
        //     if (status == 0) goto LAB_00401c7f;
        //     mxkN2GetErrorCode(status);
        //     if (0 < LOG_EN_MXK) {
        //         log("mxkAbGameId", 0x2f5, "Error mxkGetKeychipIdFromN2!!\n");
        //         KEYCHIP_ERROR = 2;
        //         KEYCHIP_STATUS = 2;
        //         *err = 2;
        //         return 1;
        //     }
        //     goto LAB_00401c6a;
        // }
        status = MXK_STATUS_ERROR;
        // TODO: This
        status = mxkGetAppBootInfo();
    } else {
        // status = appboot_flash();
    }

    if (status == MXK_STATUS_OK) {
    LAB_00401c7f:
        if (AppBoot.m_Cache.format != 1) {
            gameId[0] = '_';
            gameId[1] = '_';
            gameId[2] = '_';
            gameId[3] = '_';
            return MXK_STATUS_OK;
        }
        if (!mxkValidString(AppBoot.m_Cache.game_id, 4)) {
            *err = 55;
            return MXK_STATUS_ERROR;
        }
        memcpy(gameId, AppBoot.m_Cache.game_id, 4);
        return MXK_STATUS_OK;
    }
LAB_00401c6a:
    KEYCHIP_ERROR = 2;
    KEYCHIP_STATUS = 2;
    *err = 2;
    return MXK_STATUS_ERROR;
}
