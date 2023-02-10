#include <Windows.h>
#include <setupapi.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../lib/am/amDongle.h"
#include "../lib/am/amSerialId.h"

struct {
    char keychipId[17];
    char keychipSid[12];
    char gameId[5];
    unsigned char systemflag;
    unsigned char modelType;
    unsigned char region;
    char networkAddress[16];
} amlibDongle;

AM_DONGLE_STATUS amlib_init_dongle(void) {
    AM_DONGLE_STATUS err;
    if ((err = amDongleInit()) != AM_DONGLE_STATUS_OK) return err;

    do {
        err = amDongleSetupKeychip();
    } while (err == AM_DONGLE_STATUS_PENDING);
    if (err != AM_DONGLE_STATUS_OK) return err;

    if ((err = amDongleSetupKeychip()) != AM_DONGLE_STATUS_OK) return err;
    if (!amDongleIsAvailable()) return AM_DONGLE_STATUS_NG;
    if ((err = amDongleSetAuthConfig("toolmode")) != AM_DONGLE_STATUS_OK) return err;
    if ((err = amDongleBillingGetKeychipId(amlibDongle.keychipId, AM_DONGLE_BLOCK)) !=
        AM_DONGLE_STATUS_OK)
        return err;
    if (!amSerialIdConvert(amlibDongle.keychipId, amlibDongle.keychipSid))
        return AM_DONGLE_STATUS_NG;
    if ((err = amDongleGetGameId(amlibDongle.gameId, AM_DONGLE_BLOCK)) != AM_DONGLE_STATUS_OK)
        return err;
    if ((err = amDongleGetSystemFlag(&amlibDongle.systemflag, AM_DONGLE_BLOCK)) !=
        AM_DONGLE_STATUS_OK)
        return err;
    if ((err = amDongleGetModelType(&amlibDongle.modelType, AM_DONGLE_BLOCK)) !=
        AM_DONGLE_STATUS_OK)
        return err;
    if ((err = amDongleGetRegion(&amlibDongle.region, AM_DONGLE_BLOCK)) != AM_DONGLE_STATUS_OK)
        return err;

    unsigned int iNetworkAddress;
    if ((err = amDongleGetNetworkAddress(&iNetworkAddress, AM_DONGLE_BLOCK)) != AM_DONGLE_STATUS_OK)
        return err;

    sprintf_s(amlibDongle.networkAddress, sizeof amlibDongle.networkAddress, "%d.%d.%d.%d",
              iNetworkAddress & 0xff, (iNetworkAddress >> 8) & 0xff, (iNetworkAddress >> 16) & 0xff,
              (iNetworkAddress >> 24) & 0xff);

    return AM_DONGLE_STATUS_OK;
}

int main(int argc, char** argv) {
    amDongleDebugLevel = 1;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <infile> <outfile>\n", argv[0]);
        return -1;
    }
    if (!PathFileExistsA(argv[1])) {
        fprintf(stderr, "%s: no such file or directory\n", argv[1]);
        return -1;
    }
    // if (!PathFileExistsA(argv[2])) {
    //     fprintf(stderr, "%s: no such file or directory\n", argv[2]);
    //     return -1;
    // }

    WSADATA wsaData;
    if (WSAStartup(2, &wsaData)) {
        puts("Failed to WSAStartup");
        return -1;
    }

    puts("Waiting for dongle");
    AM_DONGLE_STATUS err;
    err = amlib_init_dongle();
    if (err != AM_DONGLE_STATUS_OK) {
        printf("Failed to init dongle:%d\n", err);
        amDongleExit();
        return -1;
    }

    printf("Dumping using dongle %s, for %s\n", amlibDongle.keychipId, amlibDongle.gameId);

    FILE* inFile;
    FILE* outFile;

    if (fopen_s(&inFile, argv[1], "rb")) {
        puts("Failed to open infile");
        amDongleExit();
        return -1;
    }
    if (fopen_s(&outFile, argv[2], "wb")) {
        puts("Failed to open outfile");
        fclose(inFile);
        amDongleExit();
        return -1;
    }

    fseek(inFile, 0, 2);
    size_t end = ftell(inFile);
    fseek(inFile, 0, 0);

    unsigned char ct[16];
    unsigned char pt[16];
    uint i = 0;
    amDongleSetIv(AM_DONGLE_BLOCK);
    while (1) {
        if (fread_s(ct, sizeof ct, 16, 1, inFile) != 1) break;
        amDongleDecrypt(ct, pt, AM_DONGLE_BLOCK);
        fwrite(pt, 16, 1, outFile);

        printf("\r%d/%d", i, end);
        i += 16;
        if (i % 512 == 0) fflush(outFile);
    }
    puts("");
    fclose(inFile);
    fclose(outFile);
    puts("Done!");

    amDongleExit();
    return 0;
}
