#include "amEeprom.h"
#pragma comment(lib, "Setupapi.lib")

#include "../../dll/smbus.h"
#include "../ami/ami.h"

AM_LIB_C_HEADER(amEeprom, AM_EEPROM)

HANDLE amEepromCreateDeviceFile(const GUID *guid, LPCSTR resource, DWORD member_index) {
    SP_DEVICE_INTERFACE_DATA interfaceData;
    SP_DEVICE_INTERFACE_DETAIL_DATA_A interfaceDetail[204];

    if (!guid) {
        if (amEepromDebugLevel > 0) amiDebugLog("PARAM Error.");
        return INVALID_HANDLE_VALUE;
    }

    HDEVINFO DeviceInfoSet =
        SetupDiGetClassDevsA(guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (DeviceInfoSet == INVALID_HANDLE_VALUE) {
        if (amEepromDebugLevel > 0)
            amiDebugLog("SetupDiGetClassDevs Error(%ld).", GetLastError());
        return INVALID_HANDLE_VALUE;
    }

    interfaceData.cbSize = 28;
    BOOL s;
    s = SetupDiEnumDeviceInterfaces(DeviceInfoSet, NULL, guid, member_index, &interfaceData);
    if (!s) {
        if (amEepromDebugLevel > 0)
            amiDebugLog("SetupDiEnumDeviceInterfaces Error(%ld).", GetLastError());
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        return INVALID_HANDLE_VALUE;
    }

    interfaceDetail[0].cbSize = 5;
    s = SetupDiGetDeviceInterfaceDetailA(DeviceInfoSet, &interfaceData, interfaceDetail,
                                         sizeof interfaceDetail, NULL, NULL);
    if (!s) {
        if (amEepromDebugLevel > 0)
            amiDebugLog("SetupDiGetDeviceInterfaceDetailA Error(%ld).", GetLastError());
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        return INVALID_HANDLE_VALUE;
    }

    char fileName[260];
    strcpy_s(fileName, sizeof fileName, interfaceDetail[0].DevicePath);

    if (resource != NULL) {
        strcat_s(fileName, 4, "\\");
        strcat_s(fileName, 4, resource);
    }

    HANDLE device =
        CreateFileA(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, FILE_SUPPORTS_GHOSTING, NULL);
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    return device;
}

AM_EEPROM_STATUS amEepromGetDriverVersion(AM_EEPROM *device, LPDWORD version) {
    if (device == NULL || version == NULL) {
        if (amEepromDebugLevel > 0) amiDebugLog("PARAM Error.");
        return AM_EEPROM_STATUS_ERR_INVALID_PARAM;
    }
    if (device->m_superio == INVALID_HANDLE_VALUE) {
        if (amEepromDebugLevel > 0) amiDebugLog("Device not opened.");
        return AM_EEPROM_STATUS_ERR_SYS;
    }

    DWORD bytesReturned;
    DWORD buffer;
    BOOL s = DeviceIoControl(device->m_superio, IOCTL_MXSMBUS_GET_VERSION, NULL, 0, &buffer, 4,
                             &bytesReturned, NULL);

    if (s && bytesReturned == sizeof buffer) {
        *version = buffer;
        return AM_EEPROM_STATUS_OK;
    }

    if (amEepromDebugLevel > 0) amiDebugLog("DeviceIoControl error(%ld).", GetLastError());
    return AM_EEPROM_STATUS_ERR_SYS;
}

AM_EEPROM_STATUS amEepromCreateMutex(AM_EEPROM *device) {
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

    if (!AllocateAndInitializeSid(&idAuth, 1, 0, 0, 0, 0, 0, 0, 0, 0, pSid)) {
        if (amEepromDebugLevel > 0)
            amiDebugLog("AllocateAndInitializeSid Error in amEepromInit.");
        *pSid = NULL;
        goto amEepromCreateMutexError;
    }

    if (!InitializeSecurityDescriptor(&securityDescriptor, 1)) {
        if (amEepromDebugLevel > 0)
            amiDebugLog("InitializeSecurityDescriptor Error in amEepromInit.");
        goto amEepromCreateMutexError;
    }

    if (!SetSecurityDescriptorDacl(&securityDescriptor, TRUE, NULL, FALSE)) {
        if (amEepromDebugLevel > 0)
            amiDebugLog("SetSecurityDescriptorDacl Error in amEepromInit.");
        goto amEepromCreateMutexError;
    }

    muxAttrs.lpSecurityDescriptor = &securityDescriptor;
    muxAttrs.nLength = 12;
    muxAttrs.bInheritHandle = FALSE;
    if ((device->m_mutex = CreateMutexA(&muxAttrs, FALSE, "Global\\AM_EEPROM_MUTEX")) != NULL) {
        return AM_EEPROM_STATUS_OK;
    }

    if (amEepromDebugLevel > 0) amiDebugLog("CreateMutexA Error(%ld).", GetLastError());

amEepromCreateMutexError:
    if (device->m_mutex != NULL) {
        CloseHandle(device->m_mutex);
        device->m_mutex = NULL;
    }
    if (*pSid != NULL) {
        FreeSid(*pSid);
        *pSid = NULL;
    }
    return AM_EEPROM_STATUS_ERR_SYS;
}

AM_EEPROM_STATUS amEepromInit(AM_EEPROM_TIMEOUT *timeout) {
    if (amEeprom.m_init) return AM_EEPROM_STATUS_ERR_ALREADY_INIT;

    amEeprom.m_prt = AM_EEPROM_ADDR;
    amEeprom.m_timeout.ReadTimeout = AM_EEPROM_DEFAULT_TIMEOUT;
    amEeprom.m_timeout.WriteTimeout = AM_EEPROM_DEFAULT_TIMEOUT;

    int ret = amEepromCreateMutex(&amEeprom);
    if (ret != 0) {
        if (amEepromDebugLevel > 0)
            amiDebugLog("amEepromCreateMutex Error!! Error Code is %ld!!", ret);
        ret = AM_EEPROM_STATUS_ERR_SYS;
        goto amEepromInitError;
    }

    if (ret != 0) {
        if (amEepromDebugLevel > 0)
            amiDebugLog("amEepromCreateMutex Error!! Error Code is %ld!!", ret);
        ret = AM_EEPROM_STATUS_ERR_SYS;
        goto amEepromInitError;
    }

    amEeprom.m_superio = amEepromCreateDeviceFile(&MXSMBUS_GUID, 0, 0);
    if (amEeprom.m_superio == INVALID_HANDLE_VALUE) {
        if (amEepromDebugLevel > 0)
            amiDebugLog("amEepromCreateDeviceFile Error in amEepromInit.");
        ret = AM_EEPROM_STATUS_ERR_SYS;
        goto amEepromInitError;
    }

    DWORD driverVersion;
    ret = amEepromGetDriverVersion(&amEeprom, &driverVersion);
    if (ret != 0) {
        amiDebugLog("amEepromGetDriverVerision Error.");
        goto amEepromInitError;
    }

    if ((driverVersion & 0xffff) != 1) {
        if (amEepromDebugLevel > 0)
            amiDebugLog(
                "Unknown SMBUS Driver Protocol(0x%08x). Please update SMBUS driver or user "
                "program.\n",
                driverVersion);

        ret = AM_EEPROM_STATUS_ERR_PROTOCOL_VER;
        goto amEepromInitError;
    }

    if (timeout != NULL) {
        amEeprom.m_timeout.ReadTimeout = timeout->ReadTimeout;
        amEeprom.m_timeout.WriteTimeout = timeout->WriteTimeout;
    }
    amEeprom.m_init = TRUE;
    return AM_EEPROM_STATUS_OK;

amEepromInitError:
    amEeprom.m_init = TRUE;
    amEepromExit();
    return ret;
}

AM_EEPROM_STATUS amEepromExit() {
    if (!amEeprom.m_init) {
        if (amEepromDebugLevel > 0) amiDebugLog("No Init Error!!");
        return AM_EEPROM_STATUS_ERR_NO_INIT;
    }
    if (amEeprom.m_superio != INVALID_HANDLE_VALUE) {
        CloseHandle(amEeprom.m_superio);
        amEeprom.m_superio = INVALID_HANDLE_VALUE;
    }
    if (amEeprom.m_mutex != NULL) {
        CloseHandle(amEeprom.m_mutex);
        amEeprom.m_mutex = NULL;
    }
    if (amEeprom.m_sid != NULL) {
        FreeSid(amEeprom.m_sid);
        amEeprom.m_sid = NULL;
    }
    amEeprom.m_init = FALSE;
    return AM_EEPROM_STATUS_OK;
}

BOOL amEepromI2CReadBlock(AM_EEPROM *device, WORD reg, BYTE nBytes, LPBYTE buffer) {
    if (device == NULL || buffer == NULL || nBytes == 0 || nBytes >= 0x21) {
        return false;
    }

    MXSMBUS_I2C_PACKET packet;
    packet.v_addr = device->m_prt;
    packet.command_code = reg;
    packet.status = 0;
    packet.command = MXSMBUS_CMD_READ_BLOCK;
    packet.nbytes = nBytes;

    DWORD bytesReturned;
    BOOL s = DeviceIoControl(device->m_superio, IOCTL_MXSMBUS_I2C, &packet, sizeof packet, &packet,
                             sizeof packet, &bytesReturned, NULL);
    if (!s || bytesReturned != sizeof packet) {
        return false;
    }

    if (packet.status != 0) {
        if (amEepromDebugLevel > 0)
            amiDebugLog(" .. SMBus read error. prt=0x%02x addr=0x%02x reg=0x%04x",
                            packet.command, packet.v_addr, packet.command_code);
        return false;
    }

    memcpy(buffer, packet.data, nBytes);
    return true;
}

BOOL amEepromI2CWriteBlock(AM_EEPROM *device, WORD reg, BYTE nBytes, LPBYTE buffer) {
    if (device == NULL || buffer == NULL || nBytes == 0 || nBytes >= 0x21) {
        return false;
    }

    MXSMBUS_I2C_PACKET packet;
    packet.v_addr = device->m_prt;
    packet.command_code = reg;
    packet.status = 0;
    packet.command = MXSMBUS_CMD_WRITE_BLOCK;
    packet.nbytes = nBytes;
    memcpy(packet.data, buffer, nBytes);

    DWORD bytesReturned;
    BOOL s = DeviceIoControl(device->m_superio, IOCTL_MXSMBUS_I2C, &packet, sizeof packet, &packet,
                             sizeof packet, &bytesReturned, NULL);
    if (!s || bytesReturned != sizeof packet) {
        return false;
    }

    if (packet.status != 0) {
        if (amEepromDebugLevel > 0)
            amiDebugLog(
                " .. SMBus write error. status=0x%02x, prt=0x%02x addr=0x%02x reg=0x%04x\n",
                packet.status, packet.command, packet.v_addr, packet.command_code);
        return false;
    }

    return true;
}

BOOL amiEepromWait(AM_EEPROM *device, int timeout) {
    amtime_t startTime;
    amtime_t nowTime;

    if (device == NULL) return FALSE;

    MXSMBUS_REQUEST_PACKET packet;
    packet.status = 24;
    packet.command = MXSMBUS_CMD_READ_BYTE;
    packet.v_addr = device->m_prt;
    packet.command_code = 0;
    packet.nbytes = 0;
    packet.data[0] = 0;
    packet.data[1] = 0;

    amiTimerGet(&startTime);
    while (1) {
        amiTimerGet(&nowTime);
        DWORD bytesReturned;
        DeviceIoControl(device->m_superio, IOCTL_MXSMBUS_REQUEST, &packet, sizeof packet, &packet,
                        sizeof packet, &bytesReturned, NULL);
        if (packet.status == 0) return TRUE;

        if (amiTimerDiffUsec(&startTime, &nowTime) > timeout * 1000) return FALSE;
    }
}

AM_EEPROM_STATUS amEepromRead(WORD reg, LPBYTE buf, DWORD length) {
    if (amEeprom.m_init == 0) {
        if (amEepromDebugLevel > 0) amiDebugLog("No Init Error.");
        return AM_EEPROM_STATUS_ERR_NO_INIT;
    }
    if (buf == NULL || length == 0) return AM_EEPROM_STATUS_ERR_INVALID_PARAM;

    DWORD err = WaitForSingleObject(amEeprom.m_mutex, 256);
    if (err == WAIT_FAILED) {
        if (amEepromDebugLevel > 0)
            amiDebugLog("WaitForSingleObject Error(%ld).", GetLastError());
        return AM_EEPROM_STATUS_ERR_SYS;
    }
    if (err == WAIT_TIMEOUT) return AM_EEPROM_STATUS_ERR_GET_MUTEX;

    if (!(err == WAIT_OBJECT_0 || err == WAIT_ABANDONED)) {
        if (amEepromDebugLevel > 0)
            amiDebugLog("WaitForSingleObject Error(%ld). Return Value is %d.", GetLastError(),
                            err);
        return AM_EEPROM_STATUS_ERR_SYS;
    }

    while (length != 0) {
        BYTE readSize = 0x20;
        if ((reg & 0x1f) != 0) readSize = 0x20 - (reg & 0x1f);
        if (length <= readSize) readSize = length & 0xff;

        if (!amiEepromWait(&amEeprom, amEeprom.m_timeout.ReadTimeout))
            return AM_EEPROM_STATUS_ERR_TIMEOUT;

        if (!amEepromI2CReadBlock(&amEeprom, reg, readSize, buf)) return AM_EEPROM_STATUS_ERR_READ;

        buf += readSize;
        reg += readSize;
        length -= readSize;
    }

    if (!ReleaseMutex(amEeprom.m_mutex)) {
        if (amEepromDebugLevel > 0) amiDebugLog("ReleaseMutex Error(%ld).", GetLastError());
        return AM_EEPROM_STATUS_ERR_SYS;
    }

    return AM_EEPROM_STATUS_OK;
}

AM_EEPROM_STATUS amEepromWrite(WORD reg, LPBYTE buf, DWORD length) {
    if (amEeprom.m_init == 0) {
        if (amEepromDebugLevel > 0) amiDebugLog("No Init Error.");
        return AM_EEPROM_STATUS_ERR_NO_INIT;
    }
    if (buf == NULL || length == 0) return AM_EEPROM_STATUS_ERR_INVALID_PARAM;

    DWORD err = WaitForSingleObject(amEeprom.m_mutex, 256);
    if (err == WAIT_FAILED) {
        if (amEepromDebugLevel > 0)
            amiDebugLog("WaitForSingleObject Error(%ld).", GetLastError());
        return AM_EEPROM_STATUS_ERR_SYS;
    }
    if (err == WAIT_TIMEOUT) return AM_EEPROM_STATUS_ERR_GET_MUTEX;

    if (!(err == WAIT_OBJECT_0 || err == WAIT_ABANDONED)) {
        if (amEepromDebugLevel > 0)
            amiDebugLog("WaitForSingleObject Error(%ld). Return Value is %d.", GetLastError(),
                            err);
        return AM_EEPROM_STATUS_ERR_SYS;
    }

    while (length != 0) {
        BYTE writeSize = 0x20;
        if ((reg & 0x1f) != 0) writeSize = 0x20 - (reg & 0x1f);
        if (length <= writeSize) writeSize = length & 0xff;

        if (!amiEepromWait(&amEeprom, amEeprom.m_timeout.WriteTimeout))
            return AM_EEPROM_STATUS_ERR_TIMEOUT;

        if (!amEepromI2CWriteBlock(&amEeprom, reg, writeSize, buf))
            return AM_EEPROM_STATUS_ERR_WRITE;

        buf += writeSize;
        reg += writeSize;
        length -= writeSize;
    }

    if (!ReleaseMutex(amEeprom.m_mutex)) {
        if (amEepromDebugLevel > 0) amiDebugLog("ReleaseMutex Error(%ld).", GetLastError());
        return AM_EEPROM_STATUS_ERR_SYS;
    }

    return AM_EEPROM_STATUS_OK;
}
