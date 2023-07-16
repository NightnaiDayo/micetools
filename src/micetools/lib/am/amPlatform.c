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
            .m_Base = 0x0,
            .m_Size = 0x1000,
            .m_BlockSize = 1,
            .m_ReadFunc = _amEepromRead,
            .m_WriteFunc = _amEepromWrite,
        },
        {
            .m_Base = 0x1000,
            .m_Size = 0x1000,
            .m_BlockSize = 1,
            .m_ReadFunc = _amEepromRead,
            .m_WriteFunc = _amEepromWrite,
        },
        {
            .m_Base = 0,
            .m_Size = 0x4000,
            .m_BlockSize = 512,
            .m_ReadFunc = _amSramRead,
            .m_WriteFunc = _amSramWrite,
        },
        {
            .m_Base = 0x4000,
            .m_Size = 0x1FC000,
            .m_BlockSize = 512,
            .m_ReadFunc = _amSramRead,
            .m_WriteFunc = _amSramWrite,
        },
    },
    // RingEdge 2
    {
        {
            .m_Base = 0x0,
            .m_Size = 0x1000,
            .m_BlockSize = 1,
            .m_ReadFunc = _amEepromRead,
            .m_WriteFunc = _amEepromWrite,
        },
        {
            .m_Base = 0x1000,
            .m_Size = 0x1000,
            .m_BlockSize = 1,
            .m_ReadFunc = _amEepromRead,
            .m_WriteFunc = _amEepromWrite,
        },
        {
            .m_Base = 0,
            .m_Size = 0x4000,
            .m_BlockSize = 4,
            .m_ReadFunc = _amSramRead,
            .m_WriteFunc = _amSramWrite,
        },
        {
            .m_Base = 0x4000,
            .m_Size = 0x1FC000,
            .m_BlockSize = 4,
            .m_ReadFunc = _amSramRead,
            .m_WriteFunc = _amSramWrite,
        },
    },
    // RingWide
    {
        {
            .m_Base = 0x0,
            .m_Size = 0x1000,
            .m_BlockSize = 1,
            .m_ReadFunc = _amEepromRead,
            .m_WriteFunc = _amEepromWrite,
        },
        {
            .m_Base = 0x1000,
            .m_Size = 0x1000,
            .m_BlockSize = 1,
            .m_ReadFunc = _amEepromRead,
            .m_WriteFunc = _amEepromWrite,
        },
        {
            .m_Base = 0,
            .m_Size = 0x4000,
            .m_BlockSize = 512,
            .m_ReadFunc = _amSramRead,
            .m_WriteFunc = _amSramWrite,
        },
        {
            .m_Base = 0x4000,
            .m_Size = 0x3C000,
            .m_BlockSize = 512,
            .m_ReadFunc = _amSramRead,
            .m_WriteFunc = _amSramWrite,
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
     * If platform starts with PLATFORM_RINGWIDE (RW) or PLATFORM_RINGEDGE (RE):
     *      copy it into m_platformId verbatim
     * Otherwise
     *      fetch the manufacturer name
     *      If manufacturer is MANUFACTURER_RE
     *          m_platformId = PLATFORM_RINGEDGE (RE)
     *      If manufacturer if MANUFACTURER_RW_1 or MANUFACTURER_RW_2
     *          m_platformId = PLATFORM_RINGWIDE (RW)
     */

    if (!amPlatform.m_platformIdCached) {
        if (amOemstringGetOemstring(oemstringPlatform, OEMSTRING_PLATFORM) !=
            AM_OEMSTRING_STATUS_OK) {
            if (amPlatformDebugLevel > 0)
                amiDebugLog("amOemstringGetOemstring Error!! (%d)", AM_PLATFORM_STATUS_ERR_SYS);
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

            if (amOemstringGetOemstring(oemstring4, OEMSTRING_BOARD_TYPE) !=
                AM_OEMSTRING_STATUS_OK) {
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

AM_PLATFORM_STATUS amPlatformGetPlatformIdEx(AM_PLATFORM_PLATFORM_ID_EX *lpPlatform) {
    char oemstring[32];
    char manufacturer[32];

    if (lpPlatform == NULL) {
        if (amPlatformDebugLevel > 0) amiDebugLog("PARAM Error!!");
        return AM_PLATFORM_STATUS_ERR_INVALID_PARAM;
    }

    static char m_strPlatformId[4];
    static DWORD m_dwPlatformId;
    static bool m_platformIdCached = false;

    int err;

    ZeroMemory(oemstring, sizeof oemstring);
    if (!m_platformIdCached) {
        err = amOemstringGetOemstring(oemstring, OEMSTRING_PLATFORM);
        if (err != AM_OEMSTRING_STATUS_OK) {
            if (amPlatformDebugLevel > 0) amiDebugLog("amOemstringGetOemstring Error!! (%d)", err);
            goto oemstring_bad;
        }

        if (strcmp(oemstring, PLATFORM_RINGEDGE) == 0 ||
            strcmp(oemstring, PLATFORM_RINGWIDE) == 0) {
            strncpy_s(m_strPlatformId, sizeof m_strPlatformId, oemstring, 0xFFFFFFFF);
        } else if (!amOemstringGetManufacturer(manufacturer)) {
            if (strcmp(manufacturer, MANUFACTURER_RE) == 0) {
                strncpy_s(m_strPlatformId, sizeof m_strPlatformId, PLATFORM_RINGEDGE, 0xFFFFFFFF);
            } else if (strcmp(manufacturer, MANUFACTURER_RW_1) == 0 ||
                       strcmp(manufacturer, MANUFACTURER_RW_2) == 0) {
                strncpy_s(m_strPlatformId, sizeof m_strPlatformId, PLATFORM_RINGWIDE, 0xFFFFFFFF);
            }
        }

        oemstring[0] = 0;
        err = amOemstringGetOemstring(oemstring, OEMSTRING_BOARD_TYPE);
        if (err != AM_OEMSTRING_STATUS_OK) {
            if (amPlatformDebugLevel > 0) amiDebugLog("amOemstringGetOemstring Error!! (%d)", err);
            goto oemstring_bad;
        }

        size_t boardTypeLen = strlen(oemstring);
        m_dwPlatformId = 0;
        if (strcmp(m_strPlatformId, PLATFORM_RINGEDGE) == 0) {
            if (boardTypeLen == 0) {
                m_dwPlatformId = 1;
                m_platformIdCached = true;
                goto LABEL_11;
            } else if (boardTypeLen == 1) {
                if (strcmp(oemstring, " ") == 0) {
                    m_dwPlatformId = 1;
                    m_platformIdCached = true;
                    goto LABEL_11;
                }
                if (amPlatformDebugLevel > 0) amiDebugLog("Oemstring length Error!!");
                goto oemstring_bad;
            }
        } else if (strcmp(m_strPlatformId, PLATFORM_RINGWIDE) == 0 && boardTypeLen == 0) {
            m_dwPlatformId = 1;
            m_platformIdCached = true;
            goto LABEL_11;
        }

        if (boardTypeLen < 3) {
            if (amPlatformDebugLevel > 0) amiDebugLog("Oemstring length Error!!");
            goto oemstring_bad;
        }
        if (strncmp(m_strPlatformId, oemstring, 3) != 0) {
            if (amPlatformDebugLevel > 0) amiDebugLog("Oemstring compare Error!!");
            goto oemstring_bad;
        }

        if (boardTypeLen > 3) {
            for (size_t i = 3; i < boardTypeLen; i++) {
                if (oemstring[i] < '0' || oemstring[i] > '9') {
                    if (amPlatformDebugLevel > 0) amiDebugLog("Oemstring format Error!!");
                    goto oemstring_bad;
                }
            }
        }

        m_dwPlatformId = atoi(&oemstring[3]);
        if (!m_dwPlatformId) goto oemstring_bad;
        m_platformIdCached = true;
    }

LABEL_11:
    memcpy(lpPlatform->strPlatformId, m_strPlatformId, 4);
    lpPlatform->platformId = m_dwPlatformId;
    return m_platformIdCached ? AM_PLATFORM_STATUS_OK : AM_PLATFORM_STATUS_ERR_SYS;

oemstring_bad:
    if (!m_platformIdCached) {
        ZeroMemory(m_strPlatformId, sizeof m_strPlatformId);
        m_dwPlatformId = 0;
    }
    goto LABEL_11;
}

int amPlatformGetBiosInfo(AM_PLATFORM_BIOS_INFO *lpInfo) {
    if (lpInfo == NULL) {
        if (amPlatformDebugLevel > 0) amiDebugLog("PARAM Error!!");
        return AM_PLATFORM_STATUS_ERR_INVALID_PARAM;
    }

    CHAR biosVer[32];
    int err = amOemstringGetSBiosVer(biosVer);
    if (err < 0) {
        if (amPlatformDebugLevel > 0) amiDebugLog("amOemstringGetSBiosVer Error!! (%d)", err);
        return AM_PLATFORM_STATUS_ERR_SYS;
    }

    CHAR biosReleaseDate[12];
    err = amOemstringGetSBiosReleaseDate(biosReleaseDate);
    if (err < 0) {
        if (amPlatformDebugLevel > 0)
            amiDebugLog("amOemstringGetSBiosReleaseDate Error!! (%d)", err);
        return AM_PLATFORM_STATUS_ERR_SYS;
    }

    strncpy_s(lpInfo->m_BiosVer, sizeof lpInfo->m_BiosVer, biosVer, 0xffffffff);
    strncpy_s(lpInfo->m_BiosReleaseDate, sizeof lpInfo->m_BiosReleaseDate, biosReleaseDate,
              0xffffffff);
    return AM_PLATFORM_STATUS_OK;
}

int amPlatformGetOsVersion(DWORD *lpVersion) {
    if (lpVersion == NULL) {
        if (amPlatformDebugLevel > 0) amiDebugLog("PARAM Error!!");
        return AM_PLATFORM_STATUS_ERR_INVALID_PARAM;
    }

    static DWORD osVersion = 0;

    if (osVersion == 0) {
        HANDLE hSysver = CreateFileW(L"C:\\System\\SystemVersion.txt", GENERIC_READ, 1, NULL,
                                     OPEN_EXISTING, FILE_SHARE_READ, NULL);
        if (hSysver == INVALID_HANDLE_VALUE) {
            if (amPlatformDebugLevel > 0)
                amiDebugLog("CreateFile Error!! Error Code is %d", GetLastError());
        } else {
            DWORD nRead = 0;
            char buffer[8];

            if (!ReadFile(hSysver, buffer, 8, &nRead, NULL)) {
                if (amPlatformDebugLevel > 0)
                    amiDebugLog("ReadFile Error!! Error Code is %d", GetLastError());
            } else if (nRead != 8) {
                if (amPlatformDebugLevel > 0)
                    amiDebugLog("ReadFile Error!! Error Code is %d", GetLastError());
            } else {
                WORD ver1 = (((buffer[0] - '0') * 1000) + ((buffer[1] - '0') * 100) +
                             (buffer[2] - '0') * 10 + (buffer[3] - '0'));
                BYTE ver2 = (buffer[4] - '0') * 10 + (buffer[5] - '0');
                BYTE ver3 = (buffer[6] - '0') * 10 + (buffer[7] - '0');

                osVersion = (((ver1 << 16) | ver2) << 8) | ver3;
            }
            CloseHandle(hSysver);
        }
    }

    *lpVersion = osVersion;
    return osVersion == 0 ? AM_PLATFORM_STATUS_ERR_SYS : AM_PLATFORM_STATUS_OK;
}

int amPlatformGetMemorySize(PLARGE_INTEGER size) {
    if (size == NULL) {
        if (amPlatformDebugLevel > 0) amiDebugLog("PARAM Error!!");
        return AM_PLATFORM_STATUS_ERR_INVALID_PARAM;
    }

    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof statex;
    if (!GlobalMemoryStatusEx((LPMEMORYSTATUSEX)&statex)) {
        if (amPlatformDebugLevel > 0)
            amiDebugLog("GlobalMemoryStatusEx Error!! (%d)", GetLastError());
        size->QuadPart = 0;
        return AM_PLATFORM_STATUS_ERR_SYS;
    }

    size->QuadPart = statex.ullTotalPhys;
    return AM_PLATFORM_STATUS_OK;
}

static AM_PLATFORM_CMOS_PARAM _cmosParam[5] = {
    // RE
    {
        .index = { { 0x60, 0x00 }, { 0x61, 0x00 } },
        .m_MbrCount = 0,
    },
    // RW 1
    {
        .index = { { 0x3a, 0x05 }, { 0x3a, 0x02 } },
        .m_MbrCount = 1,
    },
    // RW 2
    {
        .index = { { 0x28, 0x00 }, { 0x28, 0x00 } },
        .m_MbrCount = 1,
    },
    // RE 2
    {
        .index = { { 0x60, 0x00 }, { 0x61, 0x00 } },
        .m_MbrCount = 1,
    },
    {
        .index = { { 0x00, 0x00 }, { 0x00, 0x00 } },
        .m_MbrCount = 0,
    },
};
AM_PLATFORM_CMOS_PARAM *amPlatformGetCmosParam(AM_PLATFORM_BOARD_TYPE boardType) {
    if (boardType < 5) return &_cmosParam[boardType];
    return NULL;
}
