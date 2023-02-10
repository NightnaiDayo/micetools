#include <stdio.h>
#include <Windows.h>

char path[MAX_PATH * 1000];
int main(int argc, char** argv) {
    // printf("%d", QueryDosDeviceA(NULL, path, sizeof path));
    // printf(" %03x\n", GetLastError());

    // char* pPath = path;
    // while (1) {
    //     size_t len = strlen(pPath);
    //     if (!len) break;
    //     puts(pPath);
    //     pPath += len + 1;
    // }

    DWORD volumeSerialNumber;

    // Crackproof-style call
    BOOL ret = GetVolumeInformationA(
        "C:\\",
        NULL,
        0,
        &volumeSerialNumber,
        NULL,
        NULL,
        NULL,
        0
    );
    printf("volumeSerialNumber: %08x\n");

    // Exhaustive call
    CHAR volumeNameBuffer[MAX_PATH];
    DWORD maximumComponentLength;
    DWORD fileSystemFlags;
    CHAR fileSystemName[MAX_PATH];
    ret = GetVolumeInformationA(
        "C:\\",
        volumeNameBuffer,
        sizeof volumeNameBuffer,
        &volumeSerialNumber,
        &maximumComponentLength,
        &fileSystemFlags,
        fileSystemName,
        sizeof fileSystemName
    );

    printf("volumeNameBuffer: %s\n", volumeNameBuffer);
    printf("volumeSerialNumber: %08x\n", volumeSerialNumber);
    printf("maximumComponentLength: %08x\n", maximumComponentLength);
    printf("fileSystemFlags: %08x\n", fileSystemFlags);
    printf("fileSystemName: %s\n", fileSystemName);

    return 0;
}