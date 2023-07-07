#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../../subprojects/inih_dep/ini.h"
#include "../../dll/devices/smb_pca9535.h"
#include "../../dll/drivers/jvs_boards/jvs.h"
#include "../../dll/hooks/files.h"
#include "../../dll/key_config.h"

MICE_JVS _MiceJvsBoards[JVS_IO_MAX];

config_t MiceConfig = {
#define SECTION(s, comment) .s = {
#define CFG_str(s, n, default, comment) .n = default,
#define CFG_bool(s, n, default, comment) .n = default,
#define CFG_int(s, n, default, comment) .n = default,
#define CFG_hex(s, n, precision, default, comment) .n = 0x##default,
#define CFG_ipv4(s, n, a, b, c, d, comment)                                   \
    .n = (unsigned int)(((unsigned char)a << 24) | ((unsigned char)b << 16) | \
                        ((unsigned char)c << 8) | (unsigned char)d),
#define ENDSECTION(s) \
    }                 \
    ,
#include "config.def"
    ._keep_linter_happy = true
};

static void fprintf_prefix(FILE *file, const char *prefix, const char *text) {
    char *copy = (char *)malloc(strlen(text) + 1);
    memcpy_s(copy, strlen(text) + 1, text, strlen(text) + 1);

    char *next_token;
    char *token = strtok_s(copy, "\n", &next_token);
    while (token != NULL) {
        fprintf(file, "%s%s\n", prefix, token);
        token = strtok_s(NULL, "\n", &next_token);
    }

    free(copy);
}

void save_keybinds() {
    HANDLE hFile = CreateFileA(KEYBINDS_PATH, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE, NULL,
                               CREATE_ALWAYS, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Failed to open keybinds file: %d", GetLastError());
        return;
    }

    SetFilePointer(hFile, 8, NULL, 0);

    DWORD nWrote;
    DWORD totalSize = 0;
    WriteFile(hFile, &MiceConfig.keys.board_count, sizeof MiceConfig.keys.board_count, &nWrote,
              NULL);
    totalSize += nWrote;

    for (int board = 0; board < MiceConfig.keys.board_count; board++) {
        WriteFile(hFile, &_MiceJvsBoards->m_Players, sizeof _MiceJvsBoards->m_Players, &nWrote,
                  NULL);
        totalSize += nWrote;
        WriteFile(hFile, &_MiceJvsBoards->m_ButtonsPerPlayer,
                  sizeof _MiceJvsBoards->m_ButtonsPerPlayer, &nWrote, NULL);
        totalSize += nWrote;
        WriteFile(hFile, &_MiceJvsBoards->m_Coins, sizeof _MiceJvsBoards->m_Coins, &nWrote, NULL);
        totalSize += nWrote;
        WriteFile(hFile, &_MiceJvsBoards->m_NumButtons, sizeof _MiceJvsBoards->m_NumButtons,
                  &nWrote, NULL);
        totalSize += nWrote;

        WriteFile(hFile, _MiceJvsBoards[board].m_Bindings,
                  sizeof *_MiceJvsBoards[board].m_Bindings * _MiceJvsBoards->m_NumButtons, &nWrote,
                  NULL);
        totalSize += nWrote;
    }

    SetFilePointer(hFile, 0, NULL, 0);
    WriteFile(hFile, &totalSize, sizeof totalSize, &nWrote, NULL);

    amiCrc32RInit();
    DWORD readLeft = totalSize;
    DWORD crc32 = 0;
    SetFilePointer(hFile, 8, NULL, 0);
    while (readLeft) {
        BYTE readBuf[0x1000];
        DWORD toRead = sizeof readBuf;
        if (readLeft < toRead) toRead = readLeft;
        DWORD nRead;
        ReadFile(hFile, readBuf, toRead, &nRead, NULL);

        if (nRead == 0) {
            fprintf(stderr, "Binding saving failed %d\n", GetLastError());
            CloseHandle(hFile);
            return;
        }
        crc32 = amiCrc32RCalc(nRead, readBuf, crc32);
        readLeft -= nRead;
    }
    SetFilePointer(hFile, 4, NULL, 0);
    WriteFile(hFile, &crc32, sizeof crc32, &nWrote, NULL);

    CloseHandle(hFile);
}
void load_keybind_config() {
    HANDLE hFile =
        CreateFileA(KEYBINDS_PATH, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    DWORD nRead;
    DWORD nSize = 0;
    ReadFile(hFile, &nSize, sizeof nSize, &nRead, NULL);

    SetFilePointer(hFile, 8, NULL, 0);
    amiCrc32RInit();
    DWORD crc32 = 0;
    while (nSize) {
        BYTE readBuf[0x1000];
        DWORD toRead = sizeof readBuf;
        if (nSize < toRead) toRead = nSize;
        ReadFile(hFile, readBuf, toRead, &nRead, NULL);

        if (nRead == 0) {
            fprintf(stderr, "Binding reading failed\n");
            CloseHandle(hFile);
            return;
        }
        crc32 = amiCrc32RCalc(nRead, readBuf, crc32);
        nSize -= nRead;
    }

    DWORD savedCrc;
    SetFilePointer(hFile, 4, NULL, 0);
    ReadFile(hFile, &savedCrc, sizeof savedCrc, &nRead, NULL);
    if (savedCrc != crc32) {
        fprintf(stderr, "Binding file CRC failed (%08x!=%08x)\n", savedCrc, crc32);
        CloseHandle(hFile);
        return;
    }

    ReadFile(hFile, &MiceConfig.keys.board_count, sizeof MiceConfig.keys.board_count, &nRead, NULL);

    for (int board = 0; board < MiceConfig.keys.board_count; board++) {
        ReadFile(hFile, &_MiceJvsBoards->m_Players, sizeof _MiceJvsBoards->m_Players, &nRead, NULL);
        ReadFile(hFile, &_MiceJvsBoards->m_ButtonsPerPlayer,
                 sizeof _MiceJvsBoards->m_ButtonsPerPlayer, &nRead, NULL);
        ReadFile(hFile, &_MiceJvsBoards->m_Coins, sizeof _MiceJvsBoards->m_Coins, &nRead, NULL);
        ReadFile(hFile, &_MiceJvsBoards->m_NumButtons, sizeof _MiceJvsBoards->m_NumButtons, &nRead,
                 NULL);

        free(_MiceJvsBoards[board].m_Bindings);
        _MiceJvsBoards[board].m_Bindings =
            malloc(sizeof *_MiceJvsBoards[board].m_Bindings * _MiceJvsBoards->m_NumButtons);
        ReadFile(hFile, _MiceJvsBoards[board].m_Bindings,
                 sizeof *_MiceJvsBoards[board].m_Bindings * _MiceJvsBoards->m_NumButtons, &nRead,
                 NULL);

        free(_MiceJvsBoards[board].m_ButtonStates);
        _MiceJvsBoards[board].m_ButtonStates =
            malloc(sizeof *_MiceJvsBoards[board].m_ButtonStates * _MiceJvsBoards->m_NumButtons);
    }

    CloseHandle(hFile);
}

char keybindBuffer[2 * JVS_IO_MAX * (sizeof _MiceJvsBoards[0].m_Bindings[0]) * 32];
void save_current_config(bool writeDefault) {
    CreateDirectoryA("mice", NULL);  // TODO: A touch nicer?

    save_keybinds();

    FILE *config_file;
    fopen_s(&config_file, CONFIG_PATH, "w");
    if (config_file == NULL) {
        puts("Failed to create config file!");
        return;
    }
    int first_section = true;

#define CFG_str(s, n, default, comment)                                                         \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment);                       \
    fprintf(config_file, "; (string) default = %s\n", writeDefault ? MiceConfig.s.n : default); \
    fprintf(config_file, "%s = %s\n", #n, MiceConfig.s.n);

#define CFG_bool(s, n, default, comment)                                   \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment);  \
    fprintf(config_file, "; (bool) default = %s\n",                        \
            (writeDefault ? MiceConfig.s.n : default) ? "true" : "false"); \
    fprintf(config_file, "%s = %s\n", #n, MiceConfig.s.n ? "true" : "false");

#define CFG_int(s, n, default, comment)                                                      \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment);                    \
    fprintf(config_file, "; (int) default = %d\n", writeDefault ? MiceConfig.s.n : default); \
    fprintf(config_file, "%s = %d\n", #n, MiceConfig.s.n);

#define CFG_hex(s, n, precision, default, comment)                                              \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment);                       \
    fprintf(config_file, "; (hex, %d byte%s) default = %.*X \n", precision,                     \
            precision == 1 ? "" : "s", precision, writeDefault ? MiceConfig.s.n : 0x##default); \
    fprintf(config_file, "%s = %.*X\n", #n, precision, MiceConfig.s.n);

#define CFG_ipv4(s, n, a, b, c, d, comment)                                        \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment);          \
    fprintf(config_file, "; (ipv4) default = %hhu.%hhu.%hhu.%hhu ;(ipv4)\n",       \
            writeDefault ? MiceConfig.s.n >> 24 : a,                               \
            writeDefault ? (MiceConfig.s.n >> 16) & 0xff : b,                      \
            writeDefault ? (MiceConfig.s.n >> 8) & 0xff : c,                       \
            writeDefault ? (MiceConfig.s.n & 0xff) : d);                           \
    fprintf(config_file, "%s = %hhu.%hhu.%hhu.%hhu\n", #n, (MiceConfig.s.n >> 24), \
            ((MiceConfig.s.n >> 16) & 0xff), ((MiceConfig.s.n >> 8) & 0xff),       \
            (MiceConfig.s.n & 0xff));

#define SECTION(s, comment)                                               \
    if (!first_section) fprintf(config_file, "\n");                       \
    first_section = false;                                                \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment); \
    fprintf(config_file, "[%s]\n", #s);

#define COMMENT(comment) \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment);

#include "config.def"

    fclose(config_file);
}

static int handler(void *user, const char *section, const char *name, const char *value) {
    config_t *cfg = (config_t *)user;

    char *end;

    if (false)
        ;
#define CFG_str(s, n, default, comment) \
    else if (_stricmp(section, #s) == 0 && _stricmp(name, #n) == 0) cfg->s.n = _strdup(value);
#define CFG_bool(s, n, default, comment)                                       \
    else if (_stricmp(section, #s) == 0 && _stricmp(name, #n) == 0) cfg->s.n = \
        strcmp(value, "true") == 0;
#define CFG_int(s, n, default, comment)                                          \
    else if (_stricmp(section, #s) == 0 && _stricmp(name, #n) == 0) {            \
        cfg->s.n = strtol(value, &end, 10);                                      \
        if (end == value || *end != '\0' || errno == ERANGE) cfg->s.n = default; \
    }
#define CFG_hex(s, n, precision, default, comment)                                   \
    else if (_stricmp(section, #s) == 0 && _stricmp(name, #n) == 0) {                \
        cfg->s.n = strtol(value, &end, 16);                                          \
        if (end == value || *end != '\0' || errno == ERANGE) cfg->s.n = 0x##default; \
    }
#define CFG_ipv4(s, n, a, b, c, d, comment)                                                 \
    else if (_stricmp(section, #s) == 0 && _stricmp(name, #n) == 0) {                       \
        cfg->s.n = strtol(value, &end, 16);                                                 \
        unsigned char ip_a, ip_b, ip_c, ip_d;                                               \
        if (sscanf_s(value, "%hhu.%hhu.%hhu.%hhu", &ip_a, &ip_b, &ip_c, &ip_d) == 4)        \
            cfg->s.n = (unsigned int)((ip_a << 24) | (ip_b << 16) | (ip_c << 8) | ip_d);    \
        else                                                                                \
            cfg->s.n = (unsigned int)(((unsigned char)a << 24) | ((unsigned char)b << 16) | \
                                      ((unsigned char)c << 8) | (unsigned char)d);          \
    }

#include "config.def"

    return 1;
}

const unsigned int RES_W[8] = { 640, 640, 1024, 1024, 1280, 1280, 1360, 1920 };
const unsigned int RES_H[8] = { 480, 480, 600, 768, 720, 1024, 768, 1080 };
void load_mice_config() {
    if (ini_parse(CONFIG_PATH, handler, &MiceConfig) < 0) {
        printf("Loading config defaults\n");

        if (MiceConfig.sysconf.serial) free(MiceConfig.sysconf.serial);
        MiceConfig.sysconf.serial = malloc(17);
        if (MiceConfig.sysconf.keyid) free(MiceConfig.sysconf.keyid);
        MiceConfig.sysconf.keyid = malloc(17);
        GetSerialNumbers(MiceConfig.sysconf.serial, MiceConfig.sysconf.keyid);

        save_current_config(true);
    }

    if (!MiceConfig.sysconf.serial || strlen(MiceConfig.sysconf.serial) == 0) {
        if (MiceConfig.sysconf.serial) free(MiceConfig.sysconf.serial);
        MiceConfig.sysconf.serial = malloc(17);
        GetSerialNumbers(MiceConfig.sysconf.serial, NULL);
    }
    if (!MiceConfig.sysconf.keyid || strlen(MiceConfig.sysconf.keyid) == 0) {
        if (MiceConfig.sysconf.keyid) free(MiceConfig.sysconf.keyid);
        MiceConfig.sysconf.keyid = malloc(17);
        GetSerialNumbers(NULL, MiceConfig.sysconf.keyid);
    }

    if (MiceConfig.window.dipsw) {
        MiceConfig.window.w = RES_W[(MiceConfig.sysconf.dipsw >> 4) & 0b111];
        MiceConfig.window.h = RES_H[(MiceConfig.sysconf.dipsw >> 4) & 0b111];
    } else {
        MiceConfig.sysconf.dipsw &= 0b1'000'1111;

        if (MiceConfig.window.w == 1920 && MiceConfig.window.h == 1080)
            MiceConfig.sysconf.dipsw |= DIPSW_RES_1920x1080;
        else if (MiceConfig.window.w == 1360 && MiceConfig.window.h == 768)
            MiceConfig.sysconf.dipsw |= DIPSW_RES_1360x768;
        else if (MiceConfig.window.w == 1280 && MiceConfig.window.h == 1024)
            MiceConfig.sysconf.dipsw |= DIPSW_RES_1280x1024;
        else if (MiceConfig.window.w == 1280 && MiceConfig.window.h == 720)
            MiceConfig.sysconf.dipsw |= DIPSW_RES_1280x720;
        else if (MiceConfig.window.w == 1024 && MiceConfig.window.h == 768)
            MiceConfig.sysconf.dipsw |= DIPSW_RES_1024x768;
        else if (MiceConfig.window.w == 1024 && MiceConfig.window.h == 600)
            MiceConfig.sysconf.dipsw |= DIPSW_RES_1024x600;
        else if (MiceConfig.window.w == 640 && MiceConfig.window.h == 480)
            MiceConfig.sysconf.dipsw |= DIPSW_RES_640x480;
    }
}
