#include <Windows.h>
//
#include <Ntddscsi.h>
#include <stdio.h>
#define _CRT_RAND_S
#include <stdlib.h>
#include <winioctl.h>

#include "../dll/hooks/drive/irb.h"
#include "../lib/ami/amiDebug.h"

#define ATA_VENDOR_HOST_PROOF 0xC1
#define ATA_VENDOR_PROOF_DEVICE 0xC2
#define ATA_IDENTIFY_DEVICE 0xEC
#define ATA_SECURITY_UNLOCK 0xF2
#define ATA_SECURITY_FREEZE_LOCK 0xF5

#pragma pack(push, 1)
typedef struct {
    UCHAR bFeaturesReg;
    UCHAR bSectorCountReg;
    UCHAR bSectorNumberReg;
    UCHAR bCylLowReg;
    UCHAR bCylHighReg;
    UCHAR bDriveHeadReg;
    UCHAR bCommandReg;
    UCHAR bReserved;
} ATA_TASK_FILE_OUT;
typedef struct {
    UCHAR bErrorReg;
    UCHAR bSectorCountReg;
    UCHAR bSectorNumberReg;
    UCHAR bCylLowReg;
    UCHAR bCylHighReg;
    UCHAR bDriveHeadReg;
    UCHAR bStatusReg;
    UCHAR bReserved;
} ATA_TASK_FILE_IN;
typedef struct _ATA_PASS_THROUGH_EX_512 {
    ATA_PASS_THROUGH_EX Ata;
    UCHAR Data[512];
} ATA_PASS_THROUGH_EX_512, *PATA_PASS_THROUGH_EX_512;
typedef struct _ATA_PASS_THROUGH_EX_256 {
    ATA_PASS_THROUGH_EX Ata;
    UCHAR Data[256];
} ATA_PASS_THROUGH_EX_256, *PATA_PASS_THROUGH_EX_256;
#pragma pack(pop)

#define TASK_FILE_OUT(Ata) (*((ATA_TASK_FILE_OUT *)(void *)(&Ata.CurrentTaskFile)))
#define TASK_FILE_IN(Ata) (*((ATA_TASK_FILE_IN *)(void *)(&Ata.CurrentTaskFile)))

typedef enum {
    mseOk = 0,
    mseNg = -1,
    mseInvalidParam = -2,
    mseNoInit = -3,
    mseAlreadyInit = -4,
    mseDiskError = -5,
    mseAtaDeviceError = -6,
} mxk_ssd_err_t;

HANDLE PD0_HANDLE = INVALID_HANDLE_VALUE;
BOOL PD0_HAS_INIT = FALSE;

mxk_ssd_err_t mxkGetSystemDiskNumber(DWORD *disk) {
    // TODO: This
    if (disk != NULL) *disk = 1;
    return mseOk;
}

mxk_ssd_err_t mxkSsdInit(void) {
    if (PD0_HAS_INIT) return mseAlreadyInit;

    DWORD disk;
    mxk_ssd_err_t err = mxkGetSystemDiskNumber(&disk);
    if (err != mseOk) return mseDiskError;

    char path[32] = { 0 };
    sprintf_s(path, sizeof path, "\\\\.\\PhysicalDrive%d", disk);
    amiDebugLog("Using disk: %s", path);
    PD0_HANDLE = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL, OPEN_EXISTING, 0, NULL);
    if (PD0_HANDLE == INVALID_HANDLE_VALUE) {
        amiDebugLog("CreateFile Error : Error No = %d", GetLastError());
        return mseDiskError;
    }

    PD0_HAS_INIT = TRUE;
    return mseOk;
}
mxk_ssd_err_t mxkSsdExit(void) {
    if (!PD0_HAS_INIT) return mseNoInit;

    if (PD0_HANDLE != INVALID_HANDLE_VALUE) CloseHandle(PD0_HANDLE);
    PD0_HAS_INIT = FALSE;
    return mseOk;
}

mxk_ssd_err_t mxkAtaPioOutCommand(HANDLE hDrive, PATA_PASS_THROUGH_EX pAtaBuffer, UCHAR *data) {
    ATA_PASS_THROUGH_EX response;
    ATA_PASS_THROUGH_EX_512 request;

    ZeroMemory(&request, sizeof request);
    memcpy(&request.Ata, pAtaBuffer, sizeof request.Ata);

    DWORD sendBytes;
    if (data == NULL) {
        sendBytes = sizeof request.Ata;
    } else {
        memcpy(request.Data, data, sizeof request.Data);
        request.Ata.DataTransferLength = sizeof request.Data;
        sendBytes = sizeof request;
    }

    request.Ata.Length = sizeof request.Ata;
    request.Ata.DataBufferOffset = sizeof request.Ata;
    request.Ata.AtaFlags = ATA_FLAGS_DATA_OUT;
    request.Ata.TimeOutValue = 35;

    DWORD dwBytesReturned = 0;
    BOOL bResult = DeviceIoControl(hDrive, IOCTL_ATA_PASS_THROUGH, &request, sendBytes, &response,
                                   sizeof response, &dwBytesReturned, NULL);

    memcpy(pAtaBuffer, &response, sizeof response);

    if (!bResult || (TASK_FILE_IN(response).bStatusReg & 1) != 0) {
        amiDebugLog("Error DeviceIoControl() : %d/%02x:%02x", GetLastError(),
                    TASK_FILE_IN(response).bStatusReg, TASK_FILE_IN(response).bErrorReg);
        return mseAtaDeviceError;
    }
    return mseOk;
}

mxk_ssd_err_t mxkAtaPioInCommand(HANDLE hDrive, PATA_PASS_THROUGH_EX pAtaBuffer, UCHAR *dataOut) {
    ATA_PASS_THROUGH_EX request;
    ATA_PASS_THROUGH_EX_512 response;

    ZeroMemory(&response.Ata, sizeof response.Ata);
    memcpy(&request, pAtaBuffer, sizeof request);

    request.Length = sizeof request;
    request.DataBufferOffset = sizeof request;
    request.AtaFlags = ATA_FLAGS_DATA_IN;
    request.TimeOutValue = 5;

    DWORD dwBytesReturned = 0;
    BOOL bResult = DeviceIoControl(hDrive, IOCTL_ATA_PASS_THROUGH, &request, sizeof request,
                                   &response, sizeof response, &dwBytesReturned, NULL);

    memcpy(pAtaBuffer, &response, sizeof response.Ata);

    if (!bResult || (TASK_FILE_IN(response.Ata).bStatusReg & 1) != 0) {
        amiDebugLog("Error DeviceIoControl() : %d", GetLastError());
        return mseAtaDeviceError;
    }

    memcpy(dataOut, response.Data, sizeof response.Data);
    return mseOk;
}

mxk_ssd_err_t mxkSsdDeviceProofSet(UCHAR *data) {
    ATA_PASS_THROUGH_EX request;
    UCHAR localData[512];

    if (!PD0_HAS_INIT) return mseNoInit;
    if (data == NULL) return mseInvalidParam;

    ZeroMemory(&request, sizeof request);
    memcpy(localData, data, 128);

    TASK_FILE_OUT(request).bCommandReg = ATA_VENDOR_PROOF_DEVICE;
    TASK_FILE_OUT(request).bFeaturesReg = 17;

    return mxkAtaPioOutCommand(PD0_HANDLE, &request, localData);
}

mxk_ssd_err_t mxkSsdDeviceProofGet(UCHAR *data) {
    ATA_PASS_THROUGH_EX request;
    UCHAR buffer[512];

    if (!PD0_HAS_INIT) return mseNoInit;
    if (data == NULL) return mseInvalidParam;

    ZeroMemory(&request, sizeof request);
    TASK_FILE_OUT(request).bCommandReg = ATA_VENDOR_PROOF_DEVICE;
    TASK_FILE_OUT(request).bFeaturesReg = 18;
    request.DataTransferLength = sizeof buffer;

    mxk_ssd_err_t err = mxkAtaPioInCommand(PD0_HANDLE, &request, buffer);
    if (err != 0) return err;
    memcpy(data, buffer, 128);
    return mseOk;
}

#define SSD_PROOF_NBYTES 16
int mxkSsdProofDevice(UCHAR *seed, UCHAR *response, UCHAR *code) {
    UCHAR workBuf[128];

    if (response == NULL || seed == NULL) {
        if (code != NULL) *code = 54;
        return 3;
    }
    if (code == NULL) return 3;

    *code = 0;
    memcpy_s(workBuf, sizeof workBuf, seed, SSD_PROOF_NBYTES);

    unsigned int randVal;
    for (int i = SSD_PROOF_NBYTES; i < sizeof workBuf; i++) {
        rand_s(&randVal);
        workBuf[i] = randVal & 0xFF;
    }

    mxk_ssd_err_t err;
    err = mxkSsdDeviceProofSet(workBuf);
    if (err != mseOk) {
        amiDebugLog("Error mxkSsdDeviceProofSet()!!! Code:%d", err);
        *code = 2;
        return 1;
    }

    ZeroMemory(workBuf, sizeof workBuf);
    err = mxkSsdDeviceProofGet(workBuf);
    if (err != mseOk) {
        amiDebugLog("Error mxkSsdDeviceProofGet()!!! Code:%d", err);
        *code = 2;
        return 1;
    }

    memcpy_s(response, SSD_PROOF_NBYTES, workBuf, SSD_PROOF_NBYTES);
    return 0;
}

mxk_ssd_err_t mxkSsdSecurityUnlock(UCHAR *password, DWORD nPassword, BOOL userPassword) {
    ATA_PASS_THROUGH_EX request;
    UCHAR localData[512];

    if (!PD0_HAS_INIT) return mseNoInit;
    if (password == NULL) return mseInvalidParam;

    ZeroMemory(&request, sizeof request);
    ZeroMemory(localData, sizeof localData);
    if (userPassword)
        localData[0] = 0x00;
    else
        localData[0] = 0x01;
    memcpy_s(localData + 2, sizeof localData, password, nPassword);

    TASK_FILE_OUT(request).bCommandReg = ATA_SECURITY_UNLOCK;

    return mxkAtaPioOutCommand(PD0_HANDLE, &request, localData);
}

mxk_ssd_err_t mxkSsdIdentify(PIDENTIFY_DEVICE_DATA response) {
    if (!PD0_HAS_INIT) return mseNoInit;
    if (response == NULL) return mseInvalidParam;

    ATA_PASS_THROUGH_EX_256 ata = { 0 };
    ata.Ata.Length = sizeof ata.Ata;
    ata.Ata.AtaFlags = ATA_FLAGS_DATA_IN | ATA_FLAGS_DRDY_REQUIRED;
    ata.Ata.DataTransferLength = sizeof ata.Data;
    ata.Ata.DataBufferOffset = sizeof ata.Ata;
    ata.Ata.TimeOutValue = 10;
    ata.Ata.CurrentTaskFile[6] = ATA_IDENTIFY_DEVICE;
    ata.Ata.CurrentTaskFile[7] = 0;

    DWORD dwBytesReturned = 0;
    BOOL bResult = DeviceIoControl(PD0_HANDLE, IOCTL_ATA_PASS_THROUGH, &ata, sizeof(ata), &ata,
                                   sizeof(ata), &dwBytesReturned, NULL);

    if (!bResult) {
        amiDebugLog("Error DeviceIoControl() : %d", GetLastError());
        return mseAtaDeviceError;
    }

    memcpy(response, ata.Data, sizeof ata.Data);
    return mseOk;
}

void printf_word_string(UCHAR *wordString, DWORD nShorts) {
    while (nShorts--) {
        printf("%c", wordString[1]);
        printf("%c", wordString[0]);
        wordString += 2;
    }
}

void printIdentity(PIDENTIFY_DEVICE_DATA identity) {
    puts("[ Identification ]");
    printf("Serial number : ");
    printf_word_string(identity->SerialNumber, 10);
    printf("\nModel number  : ");
    printf_word_string(identity->ModelNumber, 20);
    puts("\n[ Security ]");
    printf("    Supported : %s\n", identity->SecurityStatus.SecuritySupported ? "YES" : "NO");
    printf("      Enabled : %s\n", identity->SecurityStatus.SecurityEnabled ? "YES" : "NO");
    printf("       Frozen : %s\n", identity->SecurityStatus.SecurityFrozen ? "YES" : "NO");
    printf("       Locked : %s\n", identity->SecurityStatus.SecurityLocked ? "YES" : "NO");
    printf("    # Expired : %s\n", identity->SecurityStatus.SecurityCountExpired ? "YES" : "NO");
    printf("  Master Pass : %s\n", identity->SecurityStatus.SecurityLevel ? "Maximum" : "High");
}

int main(int argc, char **argv) {
    puts("mice GBDisk utility tool.");

    printf("Opening drive\n");

    mxk_ssd_err_t err;
    err = mxkSsdInit();
    if (err != mseOk) {
        printf("Failed to open SSD: %d\n", err);
        return 1;
    }

    puts("Requesting identity");
    IDENTIFY_DEVICE_DATA identity;
    err = mxkSsdIdentify(&identity);
    if (err != mseOk) {
        printf("Failed to identify: %d\n", err);
        mxkSsdExit();
        return 1;
    }
    printIdentity(&identity);
    if (!identity.SecurityStatus.SecurityLocked) {
        puts("Security is not enabled. Nothing to do!");
        mxkSsdExit();
        return 0;
    }
    if (identity.SecurityStatus.SecurityFrozen) {
        puts("Security is frozen. We have no hope of unlocking this drive!");
        mxkSsdExit();
        return 2;
    }

    puts("Sending security unlock");
    UCHAR password[32] = {
        0x72, 0x42, 0x52, 0x5A, 0xBA, 0x52, 0x6A, 0x5A, 0xEA, 0x72, 0x62,
        0x78, 0xCA, 0x42, 0xDA, 0x4A, 0x2A, 0x22, 0x3A, 0x2A, 0x0A, 0x22,
        0x1A, 0x2A, 0x6A, 0x02, 0x7A, 0x0A, 0x5C, 0xCE, 0x4A, 0x0A,
    };
    err = mxkSsdSecurityUnlock(password, sizeof password, TRUE);
    printf("Security unlock result: %d\n", err);

    mxkSsdExit();
    return 0;
}

// int main() {
//     HANDLE hDevice = CreateFile("\\\\.\\PhysicalDrive1", GENERIC_READ | GENERIC_WRITE,
//                                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0,
//                                 NULL);
//     if (hDevice == INVALID_HANDLE_VALUE) {
//         printf("Error: could not open device\n");
//         return 1;
//     }

// ATA_PASS_THROUGH_EX_256 ata = { 0 };
// ata.Ata.Length = sizeof ata.Ata;
// ata.Ata.AtaFlags = ATA_FLAGS_DATA_IN | ATA_FLAGS_DRDY_REQUIRED;
// ata.Ata.DataTransferLength = 256;
// ata.Ata.DataBufferOffset = sizeof ata.Ata;
// ata.Ata.TimeOutValue = 10;
// ata.Ata.CurrentTaskFile[6] = ATA_IDENTIFY_DEVICE;
// ata.Ata.CurrentTaskFile[7] = 0;

// DWORD dwBytesReturned = 0;
// BOOL bResult = DeviceIoControl(hDevice, IOCTL_ATA_PASS_THROUGH, &ata, sizeof(ata), &ata,
//                                sizeof(ata), &dwBytesReturned, NULL);
// if (!bResult) {
//     printf("Error: could not send IDENTIFY DEVICE request %d\n", GetLastError());
//     CloseHandle(hDevice);
//     return 1;
// }

// for (int i = 0; i < 256; i++) {
//     printf(" %02x", ata.Data[i]);
//     if (i % 32 == 31) {
//         printf("    ");
//         for (int j = i - 31; j <= i; j++) {
//             if (10 <= ata.Data[j] && ata.Data[j] < 128)
//                 printf("%c", ata.Data[j]);
//             else
//                 printf(".");
//         }
//         puts("");
//     }
// }
// }