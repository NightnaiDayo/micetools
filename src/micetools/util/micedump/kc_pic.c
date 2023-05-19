#include <Windows.h>
#include <stdio.h>

#include "../lib/am/amDongle.h"
#include "../lib/am/amSerialId.h"
#include "../lib/mxk/mxk.h"

BOOL miceDumpKCPIC() {
    fprintf(stderr, "Dumping dongle using mxk\n");

    MXK_STATUS status;
    status = mxkInit();
    if (status != MXK_STATUS_OK) {
        amiDebugLog("Failed to mxkInit(): %d", status);
        return FALSE;
    }

    /**
     * The following sequence is required, within 30 seconds, to avoid SBTR!
     *
     * amDongleSetupKeychip:
     *     keychip.appboot.systemflag=?&cache=0
     *     keychip.version=?&cache=0
     *
     * amlib_init_dongle:
     *     keychip.billing.keyid=?&cache=1
     *     keychip.appboot.gameid=?&cache=1
     *     keychip.appboot.systemflag=?&cache=1
     *     keychip.appboot.modeltype=?&cache=1
     *     keychip.appboot.region=?&cache=1
     *     keychip.appboot.networkaddr=?&cache=1
     */

    unsigned char systemFlag;
    unsigned char err;
    mxkAbSystemFlag(MXK_CACHE_BUST, &systemFlag, &err);
    printf("SystemFlag: %02x\n", systemFlag);

    unsigned short version;
    mxkVersion(&version, MXK_CACHE_BUST, &err);
    printf("Version: %04x\n", version);

    puts("Dongle woken!");

    char gameId[4];
    mxkAbGameId(MXK_CACHE_USE, gameId, &err);
    printf("   Game ID: %.*s\n", 4, gameId);

    return TRUE;
}
