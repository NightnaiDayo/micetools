#include "mxk.h"

byte SERVER_STATE = 0;
pcpa_t PCP;
pcpa_cb_table_t CALLBACK_FUNCTION_BUFFER[40];

byte BINARY_DATA[4096];
size_t BINARY_DATA_LEN;

void mxkBinaryCallback(pcpa_t* stream, void* data) { pcpaSetSendBinaryBuffer(stream, BINARY_DATA, BINARY_DATA_LEN); }

int mxkInit() {
    // Enable colour
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hConsole, &dwMode)) SetConsoleMode(hConsole, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    WSADATA wsaData;
    int err = WSAStartup(2, &wsaData);
    SERVER_STATE = 1;
    return err;
}

void log_callback(struct pcpa* stream, void* data) {
    FILE* log_file = fopen("pcp.log", "a");
    if (log_file != NULL) {
        fprintf(log_file, "%s\n", (char*)data);
        fclose(log_file);
    }
}

e_pcpa_t mxkPcpStreamInit() {
    e_pcpa_t err;

    PCP.before_cb = log_callback;

    err = pcpaInitStream(&PCP);
    if (err != e_pcpa_ok) {
        printf("pcpaInitStream Error. Code:%d\n", err);
        return err;
    }

    err = pcpaSetCallbackFuncBuffer(&PCP, CALLBACK_FUNCTION_BUFFER,
                                    (sizeof CALLBACK_FUNCTION_BUFFER) / (sizeof CALLBACK_FUNCTION_BUFFER[0]));
    if (err != e_pcpa_ok) {
        printf("pcpaSetCallBackFuncBuffer Error. Code:%d\n", err);
        return err;
    }

    // Misc
    pcpaSetCallbackFunc(&PCP, KC_VERSION, mxkPcpVersion, NULL);
    pcpaSetCallbackFunc(&PCP, KC_STATUS, mxkPcpStatus, NULL);
    // Crypto
    pcpaSetCallbackFunc(&PCP, DS_COMPUTE, mxkPcpDsCompute, NULL);
    pcpaSetCallbackFunc(&PCP, SSD_PROOF, mxkPcpSsdProof, NULL);
    pcpaSetCallbackFunc(&PCP, SSD_HOSTPROOF, mxkPcpSsdHostProof, NULL);
    pcpaSetCallbackFunc(&PCP, KC_ENCRYPT, mkxPcpEncrypt, NULL);
    pcpaSetCallbackFunc(&PCP, KC_DECRYPT, mxkPcpDecrypt, NULL);
    pcpaSetCallbackFunc(&PCP, KC_SETIV, mxkPcpSetIv, NULL);
    // Appboot
    pcpaSetCallbackFunc(&PCP, AB_GAMEID, mxkPcpAbGameId, NULL);
    pcpaSetCallbackFunc(&PCP, AB_SYSTEMFLAG, mxkPcpAbSystemFlag, NULL);
    pcpaSetCallbackFunc(&PCP, AB_MODELTYPE, mxkPcpAbModelType, NULL);
    pcpaSetCallbackFunc(&PCP, AB_FORMATTYPE, mxkPcpAbFormatType, NULL);
    pcpaSetCallbackFunc(&PCP, AB_REGION, mxkPcpAbRegion, NULL);
    pcpaSetCallbackFunc(&PCP, AB_PLATFORMID, mxkPcpAbPlatformId, NULL);
    pcpaSetCallbackFunc(&PCP, AB_NETWORKADDRESS, mxkPcpAbNetworkAddress, NULL);
    pcpaSetCallbackFunc(&PCP, AB_DVD, mxkPcpAbDvd, NULL);
    pcpaSetCallbackFunc(&PCP, AB_SEED, mxkPcpAbSeed, NULL);
    // Billing
    pcpaSetCallbackFunc(&PCP, BIL_KEYID, mxkPcpPbKeyId, NULL);
    pcpaSetCallbackFunc(&PCP, BIL_MAINID, mxkPcpPbMainId, NULL);
    pcpaSetCallbackFunc(&PCP, BIL_PLAYCOUNT, mxkPcpPbPlayCount, NULL);
    pcpaSetCallbackFunc(&PCP, BIL_PLAYLIMIT, mxkPcpPbPlayLimit, NULL);
    pcpaSetCallbackFunc(&PCP, BIL_NEARFUL, mxkPcpPbNearfull, NULL);
    pcpaSetCallbackFunc(&PCP, BIL_SIGNATURE, mxkPcpPbSignaturePubKey, NULL);
    pcpaSetCallbackFunc(&PCP, BIL_CACERT, mxkPcpPbCaCertification, NULL);
    // Tracedata
    pcpaSetCallbackFunc(&PCP, TRA_RESTORE, mxkPcpTdRestore, NULL);
    pcpaSetCallbackFunc(&PCP, TRA_PUT, mxkPcpTdPut, NULL);
    pcpaSetCallbackFunc(&PCP, TRA_GET, mxkPcpTdGet, NULL);
    pcpaSetCallbackFunc(&PCP, TRA_LOGICALERASE, mxkPcpTdLogicalErase, NULL);
    pcpaSetCallbackFunc(&PCP, TRA_SECTOREERASE, mxkPcpTdSectorErase, NULL);
    // Storage
    pcpaSetCallbackFunc(&PCP, KC_EEPROM, mxkPcpEeprom, NULL);
    pcpaSetCallbackFunc(&PCP, KC_NVRAM0, mxkPcpNvram, NULL);
    pcpaSetCallbackFunc(&PCP, KC_NVRAM1, mxkPcpNvram, NULL);
    pcpaSetCallbackFunc(&PCP, KC_NVRAM2, mxkPcpNvram, NULL);
    pcpaSetCallbackFunc(&PCP, KC_NVRAM3, mxkPcpNvram, NULL);
    pcpaSetCallbackFunc(&PCP, KC_NVRAM4, mxkPcpNvram, NULL);
    pcpaSetCallbackFunc(&PCP, KC_NVRAM5, mxkPcpNvram, NULL);
    pcpaSetCallbackFunc(&PCP, KC_NVRAM6, mxkPcpNvram, NULL);
    pcpaSetCallbackFunc(&PCP, KC_NVRAM7, mxkPcpNvram, NULL);
    pcpaSetCallbackFunc(&PCP, KC_NVRAM8, mxkPcpNvram, NULL);
    pcpaSetCallbackFunc(&PCP, KC_NVRAM9, mxkPcpNvram, NULL);

    long text_port = Config.pcp_control_port;
    if (text_port > 0xffff) {
        puts("PCP control port invalid");
        exit(-1);
    }
    long binary_port = Config.pcp_binary_port;
    if (binary_port > 0xffff) {
        puts("PCP binary port invalid");
        exit(-1);
    }
    int open_mode = Config.pcp_bind_global ? OPEN_MODE_GLOBAL : OPEN_MODE_1;

    err = pcpaOpenServerWithBinary(&PCP, open_mode, text_port & 0xffff, binary_port & 0xffff, 300000);
    if (err != e_pcpa_ok && err != e_pcpa_to) {
        printf("pcpaOpenServerWithBinary Error. Code %d\n", err);
        return e_pcpa_not_open;
    }
    if (open_mode == OPEN_MODE_GLOBAL)
        printf("Listening on 0.0.0.0:%d (:%d)\n", text_port & 0xffff, binary_port & 0xffff);
    else
        printf("Listening on 127.0.0.1:%d (:%d)\n", text_port & 0xffff, binary_port & 0xffff);
    return e_pcpa_ok;
}

#define TICK_MS 16
// #define PRINT_DEBUG
#ifdef PRINT_DEBUG
// Larger TICK_MS for testing
#undef TICK_MS
#define TICK_MS 100
#endif

e_pcpa_t mxkPcpServer() {
    int err;
    if (SERVER_STATE == 1) {
        err = mxkPcpStreamInit();
        if (err == 0) {
            SERVER_STATE = 2;
            return err;
        }
    } else {
        if (SERVER_STATE != 2) {
            return (SERVER_STATE == 0) ? e_pcpa_cannot_open : e_pcpa_not_open;
        }
        err = pcpaServer(&PCP, TICK_MS);
        if (err == e_pcpa_to || err == e_pcpa_closed) err = e_pcpa_ok;

        if (err) {
            printf("Error pcpaServer. Code %d\n", err);
            pcpaClose(&PCP);
            SERVER_STATE = 1;
        }
    }

#ifdef PRINT_DEBUG
    puts("\033[H\033[J\033[H");
    pcpaPrint(&PCP);
    puts("\033[J");
#endif

    return err;
}