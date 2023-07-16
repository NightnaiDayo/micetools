#include "amCmos.h"

#include "../mice/ioctl.h"

AM_LIB_C_HEADER(amCmos, AM_CMOS)

AM_CMOS_STATUS amCmosInit(AM_PLATFORM_CMOS_PARAM *lpParam) {
    if (amCmos.m_bInit != 0) return AM_CMOS_STATUS_ERR_ALREADY_INIT;
    if (lpParam == NULL) return AM_CMOS_STATUS_ERR_INVALID_PARAM;

    HANDLE hMutex = CreateMutexA(NULL, 0, "AM_CMOS_MUTEX");
    if (hMutex == NULL) return AM_CMOS_STATUS_ERR_SYS;

    HANDLE hDriver = CreateFileA("\\\\.\\mxcmos", 0xc0000000, 3, NULL, 3, 0, NULL);
    if (hDriver == INVALID_HANDLE_VALUE) {
        CloseHandle(hMutex);
        return AM_CMOS_STATUS_ERR_OPEN_FAILED;
    }

    amCmos.m_bInit = TRUE;
    amCmos.m_hMutex = hMutex;
    amCmos.m_hDriver = hDriver;

    memcpy(&amCmos.m_Param, lpParam, sizeof *lpParam);

    return AM_CMOS_STATUS_OK;
}

AM_CMOS_STATUS amCmosExit(void) {
    if (amCmos.m_bInit == 0) return AM_CMOS_STATUS_ERR_NO_INIT;

    if (amCmos.m_hMutex != NULL) {
        CloseHandle(amCmos.m_hMutex);
        amCmos.m_hMutex = NULL;
    }
    if (amCmos.m_hDriver != INVALID_HANDLE_VALUE) {
        CloseHandle(amCmos.m_hDriver);
        amCmos.m_hDriver = INVALID_HANDLE_VALUE;
    }

    amCmos.m_bInit = FALSE;
    return AM_CMOS_STATUS_OK;
}

AM_CMOS_STATUS amCmosReadByteInRearpart(BYTE addr, LPBYTE lpValue) {
    if (amCmos.m_bInit == 0) return AM_CMOS_STATUS_ERR_NO_INIT;
    if (lpValue == NULL || addr > 0x7f) return AM_CMOS_STATUS_ERR_INVALID_PARAM;

    if (WaitForSingleObject(amCmos.m_hMutex, 0) != WAIT_OBJECT_0) return AM_CMOS_STATUS_ERR_LOCK;

    BYTE request[2];
    request[0] = addr;
    request[1] = 0;

    DWORD nBytesReturned;
    BOOL bErr = DeviceIoControl(amCmos.m_hDriver, IOCTL_MXCMOS_READ, request, sizeof request,
                                lpValue, 1, &nBytesReturned, NULL);

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
AM_CMOS_STATUS amCmosWriteByteInRearpart(BYTE addr, BYTE value) {
    if (amCmos.m_bInit == 0) return AM_CMOS_STATUS_ERR_NO_INIT;
    if (addr > 0x7f) return AM_CMOS_STATUS_ERR_INVALID_PARAM;

    if (WaitForSingleObject(amCmos.m_hMutex, 0) != WAIT_OBJECT_0) return AM_CMOS_STATUS_ERR_LOCK;

    BYTE request[2];
    request[0] = addr;
    request[1] = value;

    DWORD nBytesReturned;
    BOOL bErr = DeviceIoControl(amCmos.m_hDriver, IOCTL_MXCMOS_READ, request, sizeof request,
                                NULL, 0, &nBytesReturned, NULL);

    if (!ReleaseMutex(amCmos.m_hMutex)) {
        if (amCmosDebugLevel > 0)
            amiDebugLog("ReleaseMutex Error!! Error Code is %ld!!", GetLastError());
        return AM_CMOS_STATUS_ERR_SYS;
    }
    return bErr ? AM_CMOS_STATUS_OK : AM_CMOS_STATUS_ERR_SYS;
}

AM_CMOS_STATUS amCmosSetPartition(BYTE addr, BYTE bitOffset, BYTE mbrCount, BYTE partition) {
    if (!amCmos.m_bInit) return AM_CMOS_STATUS_ERR_NO_INIT;
    if (partition > 4) return AM_CMOS_STATUS_ERR_INVALID_PARAM;
    if (addr == 0 || bitOffset > 8) return AM_CMOS_STATUS_NG;

    BYTE bValue;
    AM_CMOS_STATUS err = amCmosReadByteInRearpart(addr, &bValue);
    if (amCmosReadByteInRearpart(addr, &bValue)) {
        if (amCmosDebugLevel >= 1) amiDebugLog("Error: amCmosReadByteInRearpart()");
        return err;
    }

    if (partition < mbrCount) partition += 5;

    /*
    parition range: 0, 1, 2, 3, 4, [5]
    mbrCount values: 0, 1

    -> if partition == 0 && mbrCount == 1 ::-> partition = 5

    value range: 0, 1, 2, 3, 4

    bO | mask
    0  | 11111000  <--
    1  | 11110001
    2  | 11100011
    3  | 11000111
    4  | 10001111
    5  | 00011111  <--
    6  | 00111111
    7  | 01111111

    bO | value
    0  | 00 01 02 03 04  <--
    1  | 00 02 04 06 08
    2  | 00 04 08 0c 10
    3  | 00 08 10 18 20
    4  | 00 10 20 30 40
    5  | 00 20 40 60 80  <--
    6  | 00 40 80 c0 00
    7  | 00 80 00 80 00
    */

    byte value = partition - mbrCount;
    byte mask = 0b11111000;
    for (int uVar4 = bitOffset; uVar4 != 0; uVar4--) {
        mask = (mask << 1) | 1;
        value <<= 1;
    }
    return amCmosWriteByteInRearpart(addr, (bValue & mask) | value);
}

AM_CMOS_STATUS amCmosSetPartition_0(byte partition) {
    return amCmosSetPartition(amCmos.m_Param.index[0][0], amCmos.m_Param.index[0][1],
                              amCmos.m_Param.m_MbrCount, partition);
}
AM_CMOS_STATUS amCmosSetPartition_1(byte partition) {
    return amCmosSetPartition(amCmos.m_Param.index[1][0], amCmos.m_Param.index[1][1],
                              amCmos.m_Param.m_MbrCount, partition);
}
