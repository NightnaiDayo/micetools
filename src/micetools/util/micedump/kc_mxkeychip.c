#include <Windows.h>
#include <stdio.h>

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

    sprintf(amlibDongle.networkAddress, "%d.%d.%d.%d", iNetworkAddress & 0xff,
            (iNetworkAddress >> 8) & 0xff, (iNetworkAddress >> 16) & 0xff,
            (iNetworkAddress >> 24) & 0xff);

    return AM_DONGLE_STATUS_OK;
}

void miceDumpKCMxkeychip(void) {
    amDongleDebugLevel = 1;

    fprintf(stderr, "Dumping dongle using %s\n", amDongleVersion);

    WSADATA wsaData;
    if (WSAStartup(2, &wsaData)) {
        puts("Failed to WSAStartup");
        return;
    }

    AM_DONGLE_STATUS err;
    err = amlib_init_dongle();
    if (err != AM_DONGLE_STATUS_OK) {
        printf("Failed to init dongle:%d\n", err);
        amDongleExit();
        return;
    }

    printf("Keychip ID: %s\n", amlibDongle.keychipId);
    printf("       SID: %s\n", amlibDongle.keychipSid);
    printf("   Game ID: %s\n", amlibDongle.gameId);
    printf("Systemflag: %02x\n", amlibDongle.systemflag);
    printf("Model type: %02x\n", amlibDongle.modelType);
    printf("    Region: %02x\n", amlibDongle.region);
    printf("  Net addr: %s\n", amlibDongle.networkAddress);

    unsigned short version;
    amDongleGetPicVersion(&version, AM_DONGLE_BLOCK);
    printf("   Version: %04x\n", version);
    amDongleGetVersion(&version, AM_DONGLE_BLOCK);
    printf("N2 Version: %04x\n", version);

    unsigned int billingData;
    amDongleBillingGetPlayCount(&billingData, AM_DONGLE_BLOCK);
    printf("Play Count: %08x\n", billingData);
    amDongleBillingGetPlayLimit(&billingData, AM_DONGLE_BLOCK);
    printf("Play Limit: %08x\n", billingData);
    amDongleBillingGetNearfull(&billingData, AM_DONGLE_BLOCK);
    printf("  Nearfull: %08x\n", billingData);

    amDongleExit();
}
