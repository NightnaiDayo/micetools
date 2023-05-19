#include "amCmos.h"

AM_LIB_C_HEADER(amCmos, AM_CMOS)

AM_CMOS_STATUS amCmosInit(AM_CMOS_CONFIG *lpConfig) {
    if (amCmos.m_bInit != 0) return AM_CMOS_STATUS_ERR_ALREADY_INIT;
    if (lpConfig == NULL) return AM_CMOS_STATUS_ERR_INVALID_PARAM;

    HANDLE hMutex = CreateMutexA(NULL, 0, "AM_CMOS_MUTEX");
    if (hMutex == NULL) return AM_CMOS_STATUS_ERR_SYS;

    HANDLE hCmos = CreateFileA("\\\\.\\mxcmos", 0xc0000000, 3, NULL, 3, 0, NULL);
    if (hCmos == INVALID_HANDLE_VALUE) {
        CloseHandle(hMutex);
        return AM_CMOS_STATUS_ERR_OPEN_FAILED;
    }

    amCmos.m_bInit = TRUE;
    amCmos.m_hMutex = hMutex;
    amCmos.m_hCmos = hCmos;

    memcpy(&amCmos.m_Config, lpConfig, sizeof *lpConfig);

    return AM_CMOS_STATUS_OK;
}

AM_CMOS_STATUS amCmosExit(void) {
    if (amCmos.m_bInit == 0) return AM_CMOS_STATUS_ERR_NO_INIT;

    if (amCmos.m_hMutex != NULL) {
        CloseHandle(amCmos.m_hMutex);
        amCmos.m_hMutex = NULL;
    }
    if (amCmos.m_hCmos != INVALID_HANDLE_VALUE) {
        CloseHandle(amCmos.m_hCmos);
        amCmos.m_hCmos = INVALID_HANDLE_VALUE;
    }

    amCmos.m_bInit = FALSE;
    return AM_CMOS_STATUS_OK;
}

AM_CMOS_STATUS amCmosReadByteInRearpart(BYTE nIndex, LPBYTE lpData) {
    if (amCmos.m_bInit == 0) return AM_CMOS_STATUS_ERR_NO_INIT;

    if (lpData == NULL || nIndex > 0x7f) return AM_CMOS_STATUS_ERR_INVALID_PARAM;

    if (WaitForSingleObject(amCmos.m_hMutex, 0) != WAIT_OBJECT_0) return AM_CMOS_STATUS_ERR_LOCK;

    BYTE request[2];
    request[0] = nIndex;
    request[1] = 0;

    DWORD nBytesReturned;
    BOOL bErr = DeviceIoControl(amCmos.m_hCmos, IOCTL_MXCMOS_READ, request, sizeof request, lpData,
                                1, &nBytesReturned, NULL);

    int iRet = 0;
    if (!bErr) {
        iRet = AM_CMOS_STATUS_ERR_SYS;
    } else {
        iRet = nBytesReturned == 1 ? AM_CMOS_STATUS_OK : AM_CMOS_STATUS_ERR_SYS;
    }

    if (!ReleaseMutex(amCmos.m_hMutex)) {
        if (amCmosDebugLevel > 0)
            amiDebugLog("ReleaseMutex Error!! Error Code is %ld!!", GetLastError());
        return AM_CMOS_STATUS_ERR_SYS;
    }
    return iRet;
}
