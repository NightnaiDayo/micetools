#include "../config.h"
#include "../lib/libpcp/libpcp.h"
#include "callbacks.h"

void mxkPcpPbKeyId(pcpa_t* stream, void* data) { pcpaSetSendPacket(stream, BIL_KEYID, Config.billing_keyid); }
void mxkPcpPbMainId(pcpa_t* stream, void* data) { pcpaSetSendPacket(stream, BIL_MAINID, Config.billing_mainid); }
void mxkPcpPbPlayCount(pcpa_t* stream, void* data) {
    char pc[9];
    snprintf(pc, sizeof pc, "%08X", Config.billing_playcount);
    pcpaSetSendPacket(stream, BIL_PLAYCOUNT, pc);
}
void mxkPcpPbPlayLimit(pcpa_t* stream, void* data) {
    char pl[9];
    snprintf(pl, sizeof pl, "%08X", Config.billing_playlimit);

    char* playlimit = pcpaGetCommand(stream, BIL_PLAYLIMIT);
    pcpaSetSendPacket(stream, BIL_PLAYLIMIT, pl);

    if (!strcmp(playlimit, "?")) return;

    // TODO: This
    pcpaSetSendPacket(stream, "code", "-1");
}
void mxkPcpPbNearfull(pcpa_t* stream, void* data) {
    char nf[9];
    snprintf(nf, sizeof nf, "%08X", Config.billing_nearfull);

    pcpaSetSendPacket(stream, BIL_NEARFULL, nf);
}

#define PUBKEY_BUF_MAX 1024
byte PUBKEY_BUF[PUBKEY_BUF_MAX];
int PUBKEY_LEN = 0;
void mxkPcpPbSignaturePubKey(pcpa_t* stream, void* data) {
    if (PUBKEY_LEN == 0) {
        FILE* fPubkey;
        fopen_s(&fPubkey, Config.billing_pubkey, "r");
        if (fPubkey == NULL)
            PUBKEY_LEN = -1;
        else {
            fseek(fPubkey, 0, SEEK_END);
            PUBKEY_LEN = ftell(fPubkey);
            if (PUBKEY_LEN > PUBKEY_BUF_MAX) PUBKEY_LEN = PUBKEY_BUF_MAX;
            rewind(fPubkey);

            fread(PUBKEY_BUF, PUBKEY_LEN, 1, fPubkey);
            fclose(fPubkey);
        }
    }

    if (PUBKEY_LEN == -1) {
        // TODO: Fix this maybe?
        pcpaSetBinaryMode(stream, binary_mode_none);
        pcpaSetSendPacket(stream, BIL_SIGNATURE, "-1");
        return;
    }

    pcpaSetBinaryMode(stream, binary_mode_send);
    pcpaSetBeforeBinaryModeCallBackFunc(stream, mxkBinaryCallback, NULL);

    BINARY_DATA_LEN = PUBKEY_LEN;
    memcpy(BINARY_DATA, PUBKEY_BUF, PUBKEY_LEN);
    pcpaSetRecvBinaryBuffer(stream, BINARY_DATA, BINARY_DATA_LEN);

    pcpaSetSendPacket(stream, BIL_SIGNATURE, "0");
    pcpaAddSendPacket(stream, "port", "40107");
    char sSize[16];
    snprintf(sSize, 16, "%d", PUBKEY_LEN);
    pcpaAddSendPacket(stream, "size", sSize);
}

#define CA_CERT_BUF_MAX 1024
byte CA_CERTIFICATION[CA_CERT_BUF_MAX];
int CA_CERT_LEN = 0;
void mxkPcpPbCaCertification(pcpa_t* stream, void* data) {
    if (CA_CERT_LEN == 0) {
        FILE* fCert;
        fopen_s(&fCert, Config.billing_cacert, "r");
        if (fCert == NULL)
            CA_CERT_LEN = -1;
        else {
            fseek(fCert, 0, SEEK_END);
            CA_CERT_LEN = ftell(fCert);
            if (CA_CERT_LEN > CA_CERT_BUF_MAX) CA_CERT_LEN = CA_CERT_BUF_MAX;
            rewind(fCert);

            fread(CA_CERTIFICATION, CA_CERT_LEN, 1, fCert);
            fclose(fCert);
        }
    }

    if (CA_CERT_LEN == -1) {
        // TODO: Fix this maybe?
        pcpaSetBinaryMode(stream, binary_mode_none);
        pcpaSetSendPacket(stream, BIL_CACERT, "-1");
        return;
    }

    pcpaSetBinaryMode(stream, binary_mode_send);
    pcpaSetBeforeBinaryModeCallBackFunc(stream, mxkBinaryCallback, NULL);

    BINARY_DATA_LEN = CA_CERT_LEN;
    memcpy(BINARY_DATA, CA_CERTIFICATION, CA_CERT_LEN);
    pcpaSetRecvBinaryBuffer(stream, BINARY_DATA, BINARY_DATA_LEN);

    pcpaSetSendPacket(stream, BIL_CACERT, "0");
    pcpaAddSendPacket(stream, "port", "40107");
    char sSize[16];  // todo: nicer lol
    _itoa_s(CA_CERT_LEN, sSize, sizeof sSize, 10);
    pcpaAddSendPacket(stream, "size", sSize);
}
