#include "amSram.h"

#include "../ami/ami.h"
#include "../mice/ioctl.h"

AM_LIB_C_HEADER(amSram, AM_SRAM)

AM_SRAM_STATUS amSramCreateMutex(AM_SRAM *device) {
    SECURITY_ATTRIBUTES muxAttrs;
    SECURITY_DESCRIPTOR securityDescriptor;
    SID_IDENTIFIER_AUTHORITY idAuth;

    PSID *pSid = &device->m_sid;
    idAuth.Value[0] = 0;
    idAuth.Value[1] = 0;
    idAuth.Value[2] = 0;
    idAuth.Value[3] = 0;
    idAuth.Value[4] = 0;
    idAuth.Value[5] = 1;

    if (!AllocateAndInitializeSid(&idAuth, '\x01', 0, 0, 0, 0, 0, 0, 0, 0, pSid)) {
        if (amSramDebugLevel > 0)
            amiDebugLog("AllocateAndInitializeSid Error in amSramInit.");
        *pSid = NULL;
        goto amSramCreateMutexError;
    }

    if (!InitializeSecurityDescriptor(&securityDescriptor, 1)) {
        if (amSramDebugLevel > 0)
            amiDebugLog("InitializeSecurityDescriptor Error in amSramInit.");
        goto amSramCreateMutexError;
    }

    if (!SetSecurityDescriptorDacl(&securityDescriptor, TRUE, NULL, FALSE)) {
        if (amSramDebugLevel > 0) {
            amiDebugLog("amSramCreateMutex", 0x43a,
                             "SetSecurityDescriptorDacl Error in amSramInit.\n");
        }
        goto amSramCreateMutexError;
    }

    muxAttrs.lpSecurityDescriptor = &securityDescriptor;
    muxAttrs.nLength = 12;
    muxAttrs.bInheritHandle = FALSE;
    if ((device->m_mutex = CreateMutexA(&muxAttrs, FALSE, "Global\\AM_SRAM_MUTEX")) != NULL) {
        return AM_SRAM_STATUS_OK;
    }

    if (amSramDebugLevel > 0) amiDebugLog("CreateMutexA Error(%ld).", GetLastError());

amSramCreateMutexError:
    if (device->m_mutex != NULL) {
        CloseHandle(device->m_mutex);
        device->m_mutex = NULL;
    }
    if (*pSid != NULL) {
        FreeSid(*pSid);
        *pSid = NULL;
    }
    return AM_SRAM_STATUS_ERR_SYS;
}

AM_SRAM_STATUS amSramInit(void) {
    if (amSram.m_init) return AM_SRAM_STATUS_ERR_ALREADY_INIT;

    amSram.m_readTimeout = -1;
    amSram.m_writeTimeout = -1;
    amSram.Unk20 = -1;

    AM_SRAM_STATUS status = amSramCreateMutex(&amSram);
    if (status != AM_SRAM_STATUS_OK) {
        if (amSramDebugLevel > 0)
            amiDebugLog("amSramCreateMutex Error!! Error Code is %ld!!", status);
        goto amSramInitErrorSys;
    }

    amSram.m_handle = CreateFileW(L"\\\\.\\mxsram", GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                                  FILE_SUPPORTS_REPARSE_POINTS, NULL);
    if (amSram.m_handle == INVALID_HANDLE_VALUE) {
        if (amSramDebugLevel > 0)
            amiDebugLog("CreateFileW Error!! Error Code is %ld!!", GetLastError());
        goto amSramInitErrorSys;
    }

    DWORD bytesReturned;
    DWORD version;
    BOOL s = DeviceIoControl(amSram.m_handle, IOCTL_MXSRAM_PING, NULL, 0, &version, sizeof version,
                             &bytesReturned, NULL);
    if (!s || bytesReturned != sizeof version) {
        if (amSramDebugLevel > 0)
            amiDebugLog("DeviceIoControl Error!! Error Code is %ld!!", GetLastError());
        goto amSramInitErrorSys;
    }

    if ((version & 0xffff) != 1) {
        if (amSramDebugLevel > 0) {
            amiDebugLog(
                "Sram Driver Protocol Mismatched. Detected Driver Version "
                ": 0x%08lx. Pl ease Update Sram Driver or User Program.\n",
                version);
        }
        goto amSramInitErrorSys;
    }

    DISK_GEOMETRY driveGeometry;
    s = DeviceIoControl(amSram.m_handle, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &driveGeometry,
                        sizeof driveGeometry, &bytesReturned, NULL);
    if (!s || bytesReturned != sizeof driveGeometry) {
        if (amSramDebugLevel > 0)
            amiDebugLog("DeviceIoControl Error!! Error Code is %ld!!", GetLastError());
        goto amSramInitErrorSys;
    }

    amSram.m_sectorSize = driveGeometry.BytesPerSector;
    amSram.m_size.QuadPart = driveGeometry.Cylinders.QuadPart * driveGeometry.TracksPerCylinder *
                             driveGeometry.SectorsPerTrack * driveGeometry.BytesPerSector;
    if (amSram.m_size.QuadPart == 0) {
        if (amSramDebugLevel > 0) amiDebugLog("Get Sram Size Error!!");
        goto amSramInitErrorSys;
    }

    DWORD sectorSize;
    s = DeviceIoControl(amSram.m_handle, IOCTL_MXSRAM_GET_SECTOR_SIZE, NULL, 0, &sectorSize,
                        sizeof sectorSize, &bytesReturned, NULL);
    if (s) {
        if (bytesReturned != sizeof sectorSize) {
            if (amSramDebugLevel > 0)
                amiDebugLog("DeviceIoControl Error!! Error Code is %ld!!", GetLastError());
            goto amSramInitErrorSys;
        }
        amSram.m_sectorSize = sectorSize;
    }

    if (amSram.m_sectorSize <= AM_SRAM_MAX_SECTOR_SIZE) {
        amSram.m_init = TRUE;
        return AM_SRAM_STATUS_OK;
    }

    if (amSramDebugLevel)
        amiDebugLog("Internal Error. sector size(%d)is too large.", amSram.m_sectorSize);
    goto amSramInitErrorSys;

amSramInitErrorSys:
    amSram.m_init = TRUE;
    amSramExit();
    return AM_SRAM_STATUS_ERR_SYS;
}

AM_SRAM_STATUS amSramExit(void) {
    if (!amSram.m_init) {
        if (amSramDebugLevel > 0) amiDebugLog("No Init Error!!");
        return AM_SRAM_STATUS_ERR_NO_INIT;
    }

    if (amSram.m_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(amSram.m_handle);
        amSram.m_handle = INVALID_HANDLE_VALUE;
    }
    if (amSram.m_mutex != NULL) {
        CloseHandle(amSram.m_mutex);
        amSram.m_mutex = NULL;
    }
    if (amSram.m_sid != NULL) {
        FreeSid(amSram.m_sid);
        amSram.m_sid = NULL;
    }
    amSram.m_init = FALSE;
    return AM_SRAM_STATUS_OK;
}

AM_SRAM_STATUS amSramReadInternal(AM_SRAM *device, LONG addressLow, LONG addressHigh, LPVOID buffer,
                                  DWORD nBytes) {
    if (SetFilePointer(device->m_handle, addressLow, &addressHigh, 0) == INVALID_SET_FILE_POINTER) {
        DWORD lastErr = GetLastError();
        if (lastErr != 0) {
            if (amSramDebugLevel > 0) amiDebugLog("SetFilePointer Error(%d).", lastErr);

            return AM_SRAM_STATUS_ERR_SYS;
        }
    }

    DWORD bytesRead;
    BOOL s = ReadFile(device->m_handle, buffer, nBytes, &bytesRead, NULL);
    if (!s || nBytes != bytesRead) {
        if (amSramDebugLevel > 0) amiDebugLog("ReadFile Error(%d).", GetLastError());
        return AM_SRAM_STATUS_ERR_READ;
    }

    return AM_SRAM_STATUS_OK;
}

AM_SRAM_STATUS amSramWriteInternal(AM_SRAM *device, LONG addressLow, LONG addressHigh,
                                   LPCVOID buffer, DWORD nBytes) {
    if (SetFilePointer(device->m_handle, addressLow, &addressHigh, 0) == INVALID_SET_FILE_POINTER) {
        DWORD lastErr = GetLastError();
        if (lastErr != 0) {
            if (amSramDebugLevel > 0) amiDebugLog("SetFilePointer Error(%d).", lastErr);

            return AM_SRAM_STATUS_ERR_SYS;
        }
    }

    DWORD bytesRead;
    BOOL s = WriteFile(device->m_handle, buffer, nBytes, &bytesRead, NULL);
    if (!s || nBytes != bytesRead) {
        if (amSramDebugLevel > 0) amiDebugLog("WriteFile Error(%d).", GetLastError());
        return AM_SRAM_STATUS_ERR_WRITE;
    }

    return AM_SRAM_STATUS_OK;
}

AM_SRAM_STATUS amSramRead(DWORD src, LPVOID pDst, DWORD size) {
    if (!amSram.m_init) {
        if (amSramDebugLevel > 0) amiDebugLog("Not initialized.");
        return AM_SRAM_STATUS_ERR_NO_INIT;
    }

    if (pDst == NULL || !size || size % amSram.m_sectorSize || src % amSram.m_sectorSize) {
        if (amSramDebugLevel > 0)
            amiDebugLog("Illegal parameter(s). Src=0x%08x pDst=0x%p Size=0x%08x", src, pDst,
                            size);
        return AM_SRAM_STATUS_ERR_INVALID_PARAM;
    }

    if (src > amSram.m_size.QuadPart - amSram.m_sectorSize || src + size > amSram.m_size.QuadPart) {
        if (amSramDebugLevel > 0)
            amiDebugLog("Address range is invalid. Src=0x%08x Size=0x%08x", src, size);

        return AM_SRAM_STATUS_ERR_DOMAIN;
    }

    DWORD err = WaitForSingleObject(amSram.m_mutex, amSram.m_readTimeout);
    if (err == WAIT_FAILED) {
        if (amSramDebugLevel > 0)
            amiDebugLog("WaitForSingleObject Error(%ld).", GetLastError());
        return AM_SRAM_STATUS_ERR_SYS;
    }
    if (err == WAIT_TIMEOUT) return AM_SRAM_STATUS_ERR_GET_MUTEX;

    if (!(err == WAIT_OBJECT_0 || err == WAIT_ABANDONED)) {
        if (amSramDebugLevel > 0)
            amiDebugLog("WaitForSingleObject Error(%ld). Return Value is %d.", GetLastError(),
                            err);
        return AM_SRAM_STATUS_ERR_SYS;
    }

    AM_SRAM_STATUS status = amSramReadInternal(&amSram, src, 0, pDst, size);
    if (!ReleaseMutex(amSram.m_mutex)) {
        if (amSramDebugLevel > 0) amiDebugLog("ReleaseMutex Error(%ld).", GetLastError());
        return AM_SRAM_STATUS_ERR_SYS;
    }
    return status;
}

AM_SRAM_STATUS amSramWrite(LPCVOID pSrc, DWORD dst, DWORD size) {
    if (!amSram.m_init) {
        if (amSramDebugLevel > 0) amiDebugLog("Not initialized.");
        return AM_SRAM_STATUS_ERR_NO_INIT;
    }

    if (pSrc == NULL || !size || size % amSram.m_sectorSize || dst % amSram.m_sectorSize) {
        if (amSramDebugLevel > 0)
            amiDebugLog("Illegal parameter(s). pSrc=0x%p Dst=0x%08x Size=0x%08x", pSrc, dst,
                            size);
        return AM_SRAM_STATUS_ERR_INVALID_PARAM;
    }

    if (dst > amSram.m_size.QuadPart - amSram.m_sectorSize || dst + size > amSram.m_size.QuadPart) {
        if (amSramDebugLevel > 0)
            amiDebugLog("Address range is invalid. Dst=0x%08x Size=0x%08x", dst, size);

        return AM_SRAM_STATUS_ERR_DOMAIN;
    }

    DWORD err = WaitForSingleObject(amSram.m_mutex, amSram.m_readTimeout);
    if (err == WAIT_FAILED) {
        if (amSramDebugLevel > 0)
            amiDebugLog("WaitForSingleObject Error(%ld).", GetLastError());
        return AM_SRAM_STATUS_ERR_SYS;
    }
    if (err == WAIT_TIMEOUT) return AM_SRAM_STATUS_ERR_GET_MUTEX;

    if (!(err == WAIT_OBJECT_0 || err == WAIT_ABANDONED)) {
        if (amSramDebugLevel > 0)
            amiDebugLog("WaitForSingleObject Error(%ld). Return Value is %d.", GetLastError(),
                            err);
        return AM_SRAM_STATUS_ERR_SYS;
    }

    AM_SRAM_STATUS status = amSramWriteInternal(&amSram, dst, 0, pSrc, size);
    if (!ReleaseMutex(amSram.m_mutex)) {
        if (amSramDebugLevel > 0) amiDebugLog("ReleaseMutex Error(%ld).", GetLastError());
        return AM_SRAM_STATUS_ERR_SYS;
    }
    return status;
}
