#include "mxk.h"

AM_KCF mxkKcfConfig;

byte SERVER_STATE = 0;
pcpa_t PCP;
pcpa_cb_table_t CALLBACK_FUNCTION_BUFFER[40];

byte BINARY_DATA[4096];
size_t BINARY_DATA_LEN;

void mxkBinaryCallback(pcpa_t *stream, void *data) {
    pcpaSetSendBinaryBuffer(stream, BINARY_DATA, BINARY_DATA_LEN);
}

int mxkInit() {
    // Enable colour
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hConsole, &dwMode))
        SetConsoleMode(hConsole, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    WSADATA wsaData;
    int err = WSAStartup(2, &wsaData);
    SERVER_STATE = 1;
    return err;
}

void log_callback(struct pcpa *stream, char *data, size_t nData) {
    FILE *log_file;
    fopen_s(&log_file, "pcp.log", "a");

    for (uint i = 0; i < stream->recv_data.cmd_count; i++) {
        if (i != 0) fprintf(stderr, "&");
        fprintf(stderr, "%s=%s", stream->recv_data.strings + stream->recv_data.keywords[i],
                stream->recv_data.strings + stream->recv_data.values[i]);

        if (log_file != NULL) {
            if (i != 0) fprintf(log_file, "&");
            fprintf(log_file, "%s=%s", stream->recv_data.strings + stream->recv_data.keywords[i],
                    stream->recv_data.strings + stream->recv_data.values[i]);
        }
    }
    fprintf(stderr, "\n");

    if (log_file != NULL) {
        fprintf(log_file, "\n");
        fclose(log_file);
    }
}

e_pcpa_t mxkPcpStreamInit() {
    e_pcpa_t err;

    err = pcpaInitStream(&PCP);
    if (err != e_pcpa_ok) {
        amiDebugLog("pcpaInitStream Error. Code:%d", err);
        return err;
    }
    PCP.before_cb = log_callback;

    err = pcpaSetCallbackFuncBuffer(
        &PCP, CALLBACK_FUNCTION_BUFFER,
        (sizeof CALLBACK_FUNCTION_BUFFER) / (sizeof CALLBACK_FUNCTION_BUFFER[0]));
    if (err != e_pcpa_ok) {
        amiDebugLog("pcpaSetCallBackFuncBuffer Error. Code:%d", err);
        return err;
    }

    // Misc
    pcpaSetCallbackFunc(&PCP, KC_VERSION, mxkPcpVersion, NULL);
    pcpaSetCallbackFunc(&PCP, KC_STATUS, mxkPcpStatus, NULL);
    // Crypto
    pcpaSetCallbackFunc(&PCP, DS_COMPUTE, mxkPcpDsCompute, NULL);
    pcpaSetCallbackFunc(&PCP, SSD_PROOF, mxkPcpSsdProof, NULL);
    pcpaSetCallbackFunc(&PCP, SSD_HOSTPROOF, mxkPcpSsdHostProof, NULL);
    pcpaSetCallbackFunc(&PCP, KC_ENCRYPT, mxkPcpEncrypt, NULL);
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
    pcpaSetCallbackFunc(&PCP, BIL_NEARFULL, mxkPcpPbNearfull, NULL);
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
        amiDebugLog("PCP control port invalid");
        exit(-1);
    }
    long binary_port = Config.pcp_binary_port;
    if (binary_port > 0xffff) {
        amiDebugLog("PCP binary port invalid");
        exit(-1);
    }
    int open_mode = Config.pcp_bind_global ? OPEN_MODE_GLOBAL : OPEN_MODE_LOCAL;

    err =
        pcpaOpenServerWithBinary(&PCP, open_mode, text_port & 0xffff, binary_port & 0xffff, 300000);
    if (err != e_pcpa_ok && err != e_pcpa_to) {
        amiDebugLog("pcpaOpenServerWithBinary Error. Code %d", err);
        return e_pcpa_not_open;
    }
    if (open_mode == OPEN_MODE_GLOBAL)
        amiDebugLog("Listening on 0.0.0.0:%d (:%d)", text_port & 0xffff, binary_port & 0xffff);
    else
        amiDebugLog("Listening on 127.0.0.1:%d (:%d)", text_port & 0xffff, binary_port & 0xffff);
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
            amiDebugLog("Error pcpaServer. Code %d", err);
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

static LPCSTR kcfSearchPaths[] = {
    "S:\\config.kcf",
    "C:\\system\\device\\config.kcf",
};

int mxkMain(void) {
    DWORD dwAttrib = GetFileAttributes(CONFIG_PATH);
    if (dwAttrib == INVALID_FILE_ATTRIBUTES || (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
        mxkMakeDefaultConfig();

    mxkLoadConfig();

    ZeroMemory(&mxkKcfConfig, sizeof mxkKcfConfig);
    BOOL kcfLoaded = false;
    if (Config.config_kcf_path && Config.config_kcf_path[0]) {
        kcfLoaded = MiceParseKcf(Config.config_kcf_path, &mxkKcfConfig);
    }
    if (!kcfLoaded) {
        for (int i = 0; i < _countof(kcfSearchPaths); i++) {
            kcfLoaded = MiceParseKcf(kcfSearchPaths[i], &mxkKcfConfig);
            if (kcfLoaded) break;
        }
    }

    if (!kcfLoaded) {
        amiDebugLog("Failed to parse kcf!");
    } else {
        amiDebugLog("Loaded KCF for:");
        amiDebugLog("Game ID    : %.4s", mxkKcfConfig.m_Header.m_GameId);
        amiDebugLog("Platform   : %.3s", mxkKcfConfig.m_Header.m_PlatformId);
        amiDebugLog("System Fag : %02x", mxkKcfConfig.m_Header.m_SystemFlag);
    }

    int err = mxkInit();
    if (err != 0) {
        amiDebugLog("Error mxkInit. Code %d", err);
        return -1;
    }

    while (1) {
        err = mxkPcpServer();
        if (err != e_pcpp_ok) amiDebugLog("Server tick: %d", err);
    }
}
