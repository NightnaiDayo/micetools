#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

char path[MAX_PATH * 1000];
int oldmain(int argc, char** argv) {
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
    BOOL ret = GetVolumeInformationA("C:\\", NULL, 0, &volumeSerialNumber, NULL, NULL, NULL, 0);
    printf("volumeSerialNumber: %08x\n", volumeSerialNumber);

    // Exhaustive call
    CHAR volumeNameBuffer[MAX_PATH];
    DWORD maximumComponentLength;
    DWORD fileSystemFlags;
    CHAR fileSystemName[MAX_PATH];
    ret = GetVolumeInformationA("C:\\", volumeNameBuffer, sizeof volumeNameBuffer,
                                &volumeSerialNumber, &maximumComponentLength, &fileSystemFlags,
                                fileSystemName, sizeof fileSystemName);

    printf("volumeNameBuffer: %s\n", volumeNameBuffer);
    printf("volumeSerialNumber: %08x\n", volumeSerialNumber);
    printf("maximumComponentLength: %08x\n", maximumComponentLength);
    printf("fileSystemFlags: %08x\n", fileSystemFlags);
    printf("fileSystemName: %s\n", fileSystemName);

    return 0;
}

#define RING

#ifdef RING
#define A2P \
    "C:\\Documents and Settings\\All Users\\Application Data/boost_interprocess/ALLNetComA2P"
#else
#define A2P "G:\\MegaSync\\SDEY_1.99\\maimai\\dev\\c\\ProgramData/boost_interprocess/ALLNetComA2P"
#endif

int main(int argc, char** argv) {
    unsigned char buf[1];
    char buf2[4];
    DWORD temp;

    HANDLE hFile = CreateFileA(A2P, GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Failed to open A2P %03x\n", GetLastError());
        return -1;
    }

    SetFilePointer(hFile, 4, NULL, 0);
    buf[0] = '\1';
    WriteFile(hFile, buf, sizeof buf, &temp, NULL);

    // buf[0] = '\2'; // game start
    buf[0] = '\3';  // game test
    SetFilePointer(hFile, 0, NULL, 0);
    WriteFile(hFile, buf, sizeof buf, &temp, NULL);

    SetFilePointer(hFile, 8, NULL, 0);
    buf2[0] = '1';
    buf2[0] = '.';
    buf2[0] = '9';
    buf2[0] = '9';
    WriteFile(hFile, buf2, sizeof buf2, &temp, NULL);

    CloseHandle(hFile);
}
