#include "amDipsw.h"

#include <setupapi.h>

AM_LIB_C_HEADER(amDipsw, AM_DIPSW)

int amDipswCreateMutex(AM_DIPSW *this) {
    SECURITY_ATTRIBUTES securityAttributes;
    SECURITY_DESCRIPTOR securityDescriptor;
    SID_IDENTIFIER_AUTHORITY sid;

    PSID *ppSid = &this->m_Sid;
    sid.Value[0] = '\0';
    sid.Value[1] = '\0';
    sid.Value[2] = '\0';
    sid.Value[3] = '\0';
    sid.Value[4] = '\0';
    sid.Value[5] = '\x01';
    if (!AllocateAndInitializeSid(&sid, '\x01', 0, 0, 0, 0, 0, 0, 0, 0, ppSid)) {
        if (amDipswDebugLevel > 0) amiDebugLog("AllocateAndInitializeSid Error in amDipswInit.");
        *ppSid = NULL;
        goto fail_cleanup;
    }
    if (!InitializeSecurityDescriptor(&securityDescriptor, 1)) {
        if (amDipswDebugLevel > 0)
            amiDebugLog("InitializeSecurityDescriptor Error in amDipswInit.");

        goto fail_cleanup;
    }
    if (!SetSecurityDescriptorDacl(&securityDescriptor, 1, NULL, 0)) {
        if (amDipswDebugLevel > 0) amiDebugLog("SetSecurityDescriptorDacl Error in amDipswInit.");
        goto fail_cleanup;
    }
    securityAttributes.lpSecurityDescriptor = &securityDescriptor;
    securityAttributes.nLength = 0xc;
    securityAttributes.bInheritHandle = 0;

    HANDLE mutex = CreateMutexA(&securityAttributes, 0, "Global\\AM_PCA9535_MUTEX");
    this->m_Mutex = mutex;
    if (mutex != NULL) return 0;
    if (amDipswDebugLevel > 0) amiDebugLog("CreateMutexA Error(%ld).", GetLastError());

fail_cleanup:
    if (this->m_Mutex != NULL) {
        CloseHandle(this->m_Mutex);
        this->m_Mutex = NULL;
    }
    if (*ppSid != NULL) {
        FreeSid(*ppSid);
        *ppSid = NULL;
    }
    return -5;
}

HANDLE amDipswCreateDeviceFile(GUID *guid, DWORD memberIndex, char *name) {
    HDEVINFO deviceInfoSet;
    BOOL BVar3;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    SP_DEVICE_INTERFACE_DETAIL_DATA_A deviceInterfaceDetailData[204];

    if (guid == NULL) {
        if (amDipswDebugLevel > 0) amiDebugLog("PARAM Error.");

        return INVALID_HANDLE_VALUE;
    }
    deviceInfoSet = SetupDiGetClassDevsA(guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        if (amDipswDebugLevel > 0) amiDebugLog("SetupDiGetClassDevs Error(%ld).", GetLastError());
        return INVALID_HANDLE_VALUE;
    }

    char szFilename[260];
    ZeroMemory(szFilename, 4);
    deviceInterfaceData.cbSize = 0x1c;
    if (!SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, guid, memberIndex, &deviceInterfaceData)) {
        if (amDipswDebugLevel > 0)
            amiDebugLog("SetupDiEnumDeviceInterfaces Error(%ld).", GetLastError());
    } else {
        deviceInterfaceDetailData[0].cbSize = 5;
        if (SetupDiGetDeviceInterfaceDetailA(deviceInfoSet, &deviceInterfaceData,
                                             deviceInterfaceDetailData, 0x400, NULL, NULL)) {
            strcpy_s(szFilename, 4, deviceInterfaceDetailData[0].DevicePath);
            if (name != NULL) {
                strcat_s(szFilename, 4, "\\");
                strcat_s(szFilename, 4, name);
            }
            SetupDiDestroyDeviceInfoList(deviceInfoSet);
            return CreateFileA(szFilename, GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                               FILE_SUPPORTS_GHOSTING, NULL);
        }
        if (amDipswDebugLevel > 0) amiDebugLog("SetupDiGetClassDevs Error(%ld).", GetLastError());
    }
    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return INVALID_HANDLE_VALUE;
}

void amDipswExit(void);
void amDipswGetDriverVerision(void);
void amDipswInit(void);
void amDipswModifyByte(void);
void amDipswModifyByteEx(void);
void amDipswModifyByteInternal(void);
void amDipswModifyByteInternalEx(void);
void amDipswMxsmbusStatusCheck(void);
void amDipswReadByte(void);
void amDipswReadByteInternal(void);
void amDipswReadByteInternalEx(void);
void amDipswRequestGetDipsw(void);
void amDipswRequestGetPushsw(void);
void amDipswRequestReadByte(void);
void amDipswRequestSetJvsIoctl(void);
void amDipswRequestSetLed(void);
void amDipswResponse(void);
void amDipswResponseInternalEx(void);
void amDipswWriteByte(void);
void amDipswWriteByteInternal(void);
void amDipswWriteByteInternalEx(void);
