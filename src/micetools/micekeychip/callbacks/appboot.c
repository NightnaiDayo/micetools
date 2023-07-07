#include "../lib/libpcp/libpcp.h"
#include "callbacks.h"

void mxkPcpAbGameId(pcpa_t* stream, void* data) {
    char gameId[5];
    memcpy_s(gameId, 5, mxkKcfConfig.m_Header.m_GameId, 4);
    gameId[4] = '\0';
    pcpaSetSendPacket(stream, AB_GAMEID, gameId);
}
void mxkPcpAbSystemFlag(pcpa_t* stream, void* data) {
    char sf[3];
    sprintf_s(sf, 3, "%02X", mxkKcfConfig.m_Header.m_SystemFlag);
    pcpaSetSendPacket(stream, AB_SYSTEMFLAG, sf);
}
void mxkPcpAbModelType(pcpa_t* stream, void* data) {
    char mt[3];
    sprintf_s(mt, 3, "%02X", mxkKcfConfig.m_Header.m_ModelType);
    pcpaSetSendPacket(stream, AB_MODELTYPE, mt);
}
void mxkPcpAbFormatType(pcpa_t* stream, void* data) {
    char ft[3];
    sprintf_s(ft, 3, "%02X", mxkKcfConfig.m_Header.m_Format);
    pcpaSetSendPacket(stream, AB_FORMATTYPE, ft);
}
void mxkPcpAbRegion(pcpa_t* stream, void* data) {
    char rg[3];
    sprintf_s(rg, 3, "%02X", mxkKcfConfig.m_Header.m_Region);
    pcpaSetSendPacket(stream, AB_REGION, rg);
}
void mxkPcpAbPlatformId(pcpa_t* stream, void* data) {
    char platformId[4];
    memcpy_s(platformId, 4, mxkKcfConfig.m_Header.m_PlatformId, 3);
    platformId[3] = '\0';
    pcpaSetSendPacket(stream, AB_PLATFORMID, platformId);
}
void mxkPcpAbNetworkAddress(pcpa_t* stream, void* data) {
    char networkAddr[16];
    sprintf_s(networkAddr, sizeof networkAddr, "%d.%d.%d.%d",
              (mxkKcfConfig.m_Header.m_NetworkAddr >> 0) & 0xff,
              (mxkKcfConfig.m_Header.m_NetworkAddr >> 8) & 0xff,
              (mxkKcfConfig.m_Header.m_NetworkAddr >> 16) & 0xff,
              (mxkKcfConfig.m_Header.m_NetworkAddr >> 24) & 0xff);

    pcpaSetSendPacket(stream, AB_NETWORKADDRESS, networkAddr);
}
void mxkPcpAbDvd(pcpa_t* stream, void* data) {
    char dvd[3];
    sprintf_s(dvd, 3, "%02X", mxkKcfConfig.m_Header.m_DvdFlag);
    pcpaSetSendPacket(stream, AB_DVD, dvd);
}

// #define SEED_BUF_MAX 1024
// byte SEED_BUF[SEED_BUF_MAX];
// int SEED_BUF_LEN = 0;
void mxkPcpAbSeed(pcpa_t* stream, void* data) {
    // if (SEED_BUF_LEN == 0) {
    //     FILE* fCert;
    //     fopen_s(&fCert, Config.appboot_seed, "r");
    //     if (fCert == NULL)
    //         SEED_BUF_LEN = -1;
    //     else {
    //         fseek(fCert, 0, SEEK_END);
    //         SEED_BUF_LEN = ftell(fCert);
    //         if (SEED_BUF_LEN > SEED_BUF_MAX) SEED_BUF_LEN = SEED_BUF_MAX;
    //         rewind(fCert);

    //         fread(SEED_BUF, SEED_BUF_LEN, 1, fCert);
    //         fclose(fCert);
    //     }
    // }

    // if (SEED_BUF_LEN == -1) {
    //     // TODO: Fix this maybe?
    //     pcpaSetBinaryMode(stream, binary_mode_none);
    //     pcpaSetSendPacket(stream, AB_SEED, "-1");
    //     return;
    // }

    pcpaSetBinaryMode(stream, binary_mode_send);
    pcpaSetBeforeBinaryModeCallBackFunc(stream, mxkBinaryCallback, NULL);

    BINARY_DATA_LEN = sizeof mxkKcfConfig.m_Seed;
    memcpy(BINARY_DATA, mxkKcfConfig.m_Seed, sizeof mxkKcfConfig.m_Seed);
    pcpaSetRecvBinaryBuffer(stream, BINARY_DATA, BINARY_DATA_LEN);

    pcpaSetSendPacket(stream, AB_SEED, "0");
    pcpaAddSendPacket(stream, "port", "40107");

    char sSize[16];
    sprintf_s(sSize, sizeof sSize, "%zd", sizeof mxkKcfConfig.m_Seed);
    pcpaAddSendPacket(stream, "size", sSize);
}
