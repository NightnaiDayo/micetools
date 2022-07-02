#include "../config.h"
#include "../lib/libpcp/libpcp.h"
#include "callbacks.h"

void mxkPcpAbGameId(pcpa_t* stream, void* data) { pcpaSetSendPacket(stream, AB_GAMEID, Config.appboot_gameid); }
void mxkPcpAbSystemFlag(pcpa_t* stream, void* data) {
    char sf[3];
    snprintf(sf, 3, "%02X", Config.appboot_systemflag);
    pcpaSetSendPacket(stream, AB_SYSTEMFLAG, sf);
}
void mxkPcpAbModelType(pcpa_t* stream, void* data) {
    char mt[16];
    snprintf(mt, 16, "%02X", Config.appboot_modeltype);
    pcpaSetSendPacket(stream, AB_MODELTYPE, mt);
}
void mxkPcpAbFormatType(pcpa_t* stream, void* data) {
    char ft[16];
    snprintf(ft, 16, "%02X", Config.appboot_formattype);
    pcpaSetSendPacket(stream, AB_FORMATTYPE, ft);
}
void mxkPcpAbRegion(pcpa_t* stream, void* data) {
    char rg[16];
    snprintf(rg, 16, "%02X", Config.appboot_region);
    pcpaSetSendPacket(stream, AB_REGION, rg);
}
void mxkPcpAbPlatformId(pcpa_t* stream, void* data) {
    pcpaSetSendPacket(stream, AB_PLATFORMID, Config.appboot_platformid);
}
void mxkPcpAbNetworkAddress(pcpa_t* stream, void* data) {
    pcpaSetSendPacket(stream, AB_NETWORKADDRESS, Config.appboot_network);
}
void mxkPcpAbDvd(pcpa_t* stream, void* data) {
    char dvd[16];
    snprintf(dvd, 16, "%02X", Config.appboot_dvdflag);
    pcpaSetSendPacket(stream, AB_DVD, dvd);
}

#define SEED_BUF_MAX 1024
byte SEED_BUF[SEED_BUF_MAX];
int SEED_BUF_LEN = 0;
void mxkPcpAbSeed(pcpa_t* stream, void* data) {
    if (SEED_BUF_LEN == 0) {
        FILE* fCert;
        fopen_s(&fCert, Config.appboot_seed, "r");
        if (fCert == NULL)
            SEED_BUF_LEN = -1;
        else {
            fseek(fCert, 0, SEEK_END);
            SEED_BUF_LEN = ftell(fCert);
            if (SEED_BUF_LEN > SEED_BUF_MAX) SEED_BUF_LEN = SEED_BUF_MAX;
            rewind(fCert);

            fread(SEED_BUF, SEED_BUF_LEN, 1, fCert);
            fclose(fCert);
        }
    }

    if (SEED_BUF_LEN == -1) {
        // TODO: Fix this maybe?
        pcpaSetBinaryMode(stream, binary_mode_none);
        pcpaSetSendPacket(stream, AB_SEED, "-1");
        return;
    }

    pcpaSetBinaryMode(stream, binary_mode_send);
    pcpaSetBeforeBinaryModeCallBackFunc(stream, mxkBinaryCallback, NULL);

    BINARY_DATA_LEN = SEED_BUF_LEN;
    memcpy(BINARY_DATA, SEED_BUF, SEED_BUF_LEN);
    pcpaSetRecvBinaryBuffer(stream, BINARY_DATA, BINARY_DATA_LEN);

    pcpaSetSendPacket(stream, AB_SEED, "0");
    pcpaAddSendPacket(stream, "port", "40107");
    char sSize[16];  // todo: nicer lol
    _itoa(SEED_BUF_LEN, sSize, 10);
    pcpaAddSendPacket(stream, "size", sSize);
}
