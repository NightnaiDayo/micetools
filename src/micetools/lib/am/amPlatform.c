#include "amPlatform.h"

#include "../ami/amiDebug.h"
#include "amEeprom.h"
#include "amOemstring.h"
#include "amSram.h"

AM_LIB_C_HEADER(amPlatform, AM_PLATFORM)

#define _amEepromRead (amPlatformRead_t *)&amEepromRead
#define _amEepromWrite (amPlatformWrite_t *)&amEepromWrite
#define _amSramRead (amPlatformRead_t *)&amSramRead
#define _amSramWrite (amPlatformWrite_t *)&amSramWrite

AM_PLATFORM_NV_DEVICE_CONFIG amPlatformNvDevices[3][4] = {
    // RingEdge 1
    {
        {
            .m_base = 0x0,
            .m_size = 0x1000,
            .m_blockSize = 1,
            .m_read = _amEepromRead,
            .m_write = _amEepromWrite,
        },
        {
            .m_base = 0x1000,
            .m_size = 0x1000,
            .m_blockSize = 1,
            .m_read = _amEepromRead,
            .m_write = _amEepromWrite,
        },
        {
            .m_base = 0,
            .m_size = 0x4000,
            .m_blockSize = 512,
            .m_read = _amSramRead,
            .m_write = _amSramWrite,
        },
        {
            .m_base = 0x4000,
            .m_size = 0x1FC000,
            .m_blockSize = 512,
            .m_read = _amSramRead,
            .m_write = _amSramWrite,
        },
    },
    // RingEdge 2
    {
        {
            .m_base = 0x0,
            .m_size = 0x1000,
            .m_blockSize = 1,
            .m_read = _amEepromRead,
            .m_write = _amEepromWrite,
        },
        {
            .m_base = 0x1000,
            .m_size = 0x1000,
            .m_blockSize = 1,
            .m_read = _amEepromRead,
            .m_write = _amEepromWrite,
        },
        {
            .m_base = 0,
            .m_size = 0x4000,
            .m_blockSize = 4,
            .m_read = _amSramRead,
            .m_write = _amSramWrite,
        },
        {
            .m_base = 0x4000,
            .m_size = 0x1FC000,
            .m_blockSize = 4,
            .m_read = _amSramRead,
            .m_write = _amSramWrite,
        },
    },
    // RingWide
    {
        {
            .m_base = 0x0,
            .m_size = 0x1000,
            .m_blockSize = 1,
            .m_read = _amEepromRead,
            .m_write = _amEepromWrite,
        },
        {
            .m_base = 0x1000,
            .m_size = 0x1000,
            .m_blockSize = 1,
            .m_read = _amEepromRead,
            .m_write = _amEepromWrite,
        },
        {
            .m_base = 0,
            .m_size = 0x4000,
            .m_blockSize = 512,
            .m_read = _amSramRead,
            .m_write = _amSramWrite,
        },
        {
            .m_base = 0x4000,
            .m_size = 0x3C000,
            .m_blockSize = 512,
            .m_read = _amSramRead,
            .m_write = _amSramWrite,
        },
    },
};

AM_PLATFORM_NV_DEVICE_CONFIG *amPlatformGetNvDevice(AM_PLATFORM_NV_DEVICE device) {
    if (device > 0 && device < _NUM_AM_PLATFORM_NV_DEVICE) {
        AM_PLATFORM_BOARD_TYPE boardType;
        amPlatformGetBoardType(&boardType);
        switch (boardType) {
            case AM_PLATFORM_BOARD_TYPE_RINGEDGE:
                return &(amPlatformNvDevices[0][device]);
            case AM_PLATFORM_BOARD_TYPE_RW_SPEC_1:
            case AM_PLATFORM_BOARD_TYPE_RW_SPEC_2:
                return &(amPlatformNvDevices[2][device]);
            case AM_PLATFORM_BOARD_TYPE_RINGEDGE2:
                return &(amPlatformNvDevices[1][device]);
        }
    }
    return NULL;
}

AM_PLATFORM_STATUS amPlatformGetPlatformId(AM_PLATFORM_PLATFORM_ID *platformId) {
    if (platformId == NULL) {
        if (amPlatformDebugLevel > 0) amiDebugLog("PARAM Error!!");
        return AM_PLATFORM_STATUS_ERR_INVALID_PARAM;
    }

    char oemstringPlatform[32];
    char oemstringManufacturer[32];

    /**
     * If platform starts with "AAM" (RW) or "AAL" (RE):
     *      copy it into m_platformId verbatim
     * Otherwise
     *      fetch the manufacturer name
     *      If manufacturer is "NEC"
     *          m_platformId = "AAL" (RE)
     *      If manufacturer if "Supermicro" or "Advantech"
     *          m_platformId = "AAM" (RW)
     */

    if (!amPlatform.m_platformIdCached) {
        if (amOemstringGetOemstring(oemstringPlatform, 2) != AM_OEMSTRING_STATUS_OK) {
            if (amPlatformDebugLevel > 0)
                amiDebugLog("amOemstringGetOemstring Error!! (%d)",
                                AM_PLATFORM_STATUS_ERR_SYS);
            return AM_PLATFORM_STATUS_ERR_SYS;
        }

        if (strcmp(oemstringPlatform, PLATFORM_RINGEDGE) == 0 ||
            strcmp(oemstringPlatform, PLATFORM_RINGWIDE) == 0) {
            strncpy_s(amPlatform.m_platformId.strPlatformId,
                      sizeof amPlatform.m_platformId.strPlatformId, oemstringPlatform, 0xffffffff);
        } else if (amOemstringGetManufacturer(oemstringManufacturer) == AM_OEMSTRING_STATUS_OK) {
            if (strcmp(oemstringManufacturer, MANUFACTURER_RE) == 0) {
                strncpy_s(amPlatform.m_platformId.strPlatformId,
                          sizeof amPlatform.m_platformId.strPlatformId, PLATFORM_RINGEDGE,
                          0xffffffff);
            } else if (strcmp(oemstringManufacturer, MANUFACTURER_RW_1) == 0 ||
                       strcmp(oemstringManufacturer, MANUFACTURER_RW_2) == 0) {
                strncpy_s(amPlatform.m_platformId.strPlatformId,
                          sizeof amPlatform.m_platformId.strPlatformId, PLATFORM_RINGWIDE,
                          0xffffffff);
            }
        }
        amPlatform.m_platformIdCached = TRUE;
    }

    memcpy(platformId->strPlatformId, amPlatform.m_platformId.strPlatformId,
           sizeof amPlatform.m_platformId.strPlatformId);
    return amPlatform.m_platformIdCached ? AM_PLATFORM_STATUS_OK : AM_PLATFORM_STATUS_ERR_SYS;
}

AM_PLATFORM_STATUS amPlatformGetBoardType(AM_PLATFORM_BOARD_TYPE *boardType) {
    AM_PLATFORM_PLATFORM_ID platformId;
    char oemstring4[32];
    char oemstringManufacturer[32];

    if (boardType == NULL) {
        if (amPlatformDebugLevel > 0) amiDebugLog("PARAM Error!!");
        return AM_PLATFORM_STATUS_ERR_INVALID_PARAM;
    }

    if (amPlatform.m_boardTypeCached) {
        *boardType = amPlatform.m_boardType;
        return AM_PLATFORM_STATUS_OK;
    }
    if (amPlatformGetPlatformId(&platformId) != AM_PLATFORM_STATUS_OK ||
        amOemstringGetManufacturer(oemstringManufacturer) != AM_OEMSTRING_STATUS_OK) {
        *boardType = amPlatform.m_boardType;
        return AM_PLATFORM_STATUS_ERR_SYS;
    }

    if (strcmp(platformId.strPlatformId, PLATFORM_RINGWIDE) == 0) {
        if (strcmp(oemstringManufacturer, MANUFACTURER_RW_1) == 0) {
            *boardType = amPlatform.m_boardType = AM_PLATFORM_BOARD_TYPE_RW_SPEC_1;
        } else if (strcmp(oemstringManufacturer, MANUFACTURER_RW_2) == 0) {
            *boardType = amPlatform.m_boardType = AM_PLATFORM_BOARD_TYPE_RW_SPEC_2;
        } else {
            *boardType = amPlatform.m_boardType = AM_PLATFORM_BOARD_TYPE_UNKNOWN;
        }

        amPlatform.m_boardTypeCached = TRUE;
        return AM_PLATFORM_STATUS_OK;
    }

    if (strcmp(platformId.strPlatformId, PLATFORM_RINGEDGE) == 0) {
        if (strcmp(oemstringManufacturer, MANUFACTURER_RE) == 0) {
            ZeroMemory(oemstring4, sizeof oemstring4);

            if (amOemstringGetOemstring(oemstring4, 4) != AM_OEMSTRING_STATUS_OK) {
                *boardType = amPlatform.m_boardType = AM_PLATFORM_BOARD_TYPE_UNKNOWN;
                return AM_PLATFORM_STATUS_ERR_SYS;
            }

            if (strcmp(oemstring4, PLATFORM_RINGEDGE2) == 0) {
                *boardType = amPlatform.m_boardType = AM_PLATFORM_BOARD_TYPE_RINGEDGE2;
                amPlatform.m_boardTypeCached = TRUE;
                return AM_PLATFORM_STATUS_OK;
            }

            if (strcmp(oemstring4, "") == 0 || strcmp(oemstring4, " ") == 0) {
                *boardType = amPlatform.m_boardType = AM_PLATFORM_BOARD_TYPE_RINGEDGE;
                amPlatform.m_boardTypeCached = TRUE;
                return AM_PLATFORM_STATUS_OK;
            }
        }
    }

    *boardType = amPlatform.m_boardType = AM_PLATFORM_BOARD_TYPE_UNKNOWN;
    amPlatform.m_boardTypeCached = TRUE;
    return AM_PLATFORM_STATUS_OK;
}
