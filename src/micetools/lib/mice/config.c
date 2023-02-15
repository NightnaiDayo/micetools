#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../../subprojects/inih_dep/ini.h"
#include "../../dll/devices/smb_pca9535.h"
#include "../../dll/key_config.h"

config_t MiceConfig = {
#define SECTION(s, comment) .s = {
#define CFG_str(s, n, default, comment) .n = default,
#define CFG_bool(s, n, default, comment) .n = default,
#define CFG_int(s, n, default, comment) .n = default,
#define CFG_hex(s, n, precision, default, comment) .n = 0x##default,
#define CFG_ipv4(s, n, a, b, c, d, comment) .n = (a << 24) | (b << 16) | (c << 8) | d,
#define ENDSECTION(s) \
    }                 \
    ,
#include "config.def"
    ._keep_linter_happy = true
};

void fprintf_prefix(FILE *file, const char *prefix, const char *text) {
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

void make_default_config() {
    FILE *config_file;
    fopen_s(&config_file, CONFIG_PATH, "w");
    if (config_file == NULL) {
        puts("Failed to create config file!");
        return;
    };
    int first_section = true;

#define CFG_str(s, n, default, comment)                                   \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment); \
    fprintf(config_file, "%s = %s ;(string)\n", #n, default);

#define CFG_bool(s, n, default, comment)                                  \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment); \
    fprintf(config_file, "%s = %s ;(bool)\n", #n, default ? "true" : "false");

#define CFG_int(s, n, default, comment)                                   \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment); \
    fprintf(config_file, "%s = %d ;(int)\n", #n, default);

#define CFG_hex(s, n, precision, default, comment)                                               \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment);                        \
    fprintf(config_file, "%s = %.*X ;(hex, %d byte%s)\n", #n, precision, 0x##default, precision, \
            precision == 1 ? "" : "s");

#define CFG_ipv4(s, n, a, b, c, d, comment)                               \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment); \
    fprintf(config_file, "%s = %hhu.%hhu.%hhu.%hhu ;(ipv4)\n", #n, a, b, c, d);

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

char keybindBuffer[8192];
void save_current_config() {
    // Dump keybind settings
    int i = 0;
    for (int board = 0; board < MiceConfig.keys.board_count; board++) {
        i += snprintf(keybindBuffer + i, _countof(keybindBuffer) - i, "%d,",
                      jvsKeybindings[board].test);
        for (int n = 0; n < JVS_BUTTON_PAIR_COUNT * 2; n++) {
            i += snprintf(keybindBuffer + i, _countof(keybindBuffer) - i, "%d,",
                          jvsKeybindings[board].buttons[n]);
            i += snprintf(keybindBuffer + i, _countof(keybindBuffer) - i, "%d,",
                          jvsKeybindings[board].invert[n]);
        }
    }
    keybindBuffer[i] = '\0';
    free(MiceConfig.keys.keys);
    MiceConfig.keys.keys = strdup(keybindBuffer);

    FILE *config_file;
    fopen_s(&config_file, CONFIG_PATH, "w");
    if (config_file == NULL) {
        puts("Failed to create config file!");
        return;
    };
    int first_section = true;

#define CFG_str(s, n, default, comment)                                   \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment); \
    fprintf(config_file, "%s = %s ;(string)\n", #n, MiceConfig.s.n);

#define CFG_bool(s, n, default, comment)                                  \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment); \
    fprintf(config_file, "%s = %s ;(bool)\n", #n, MiceConfig.s.n ? "true" : "false");

#define CFG_int(s, n, default, comment)                                   \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment); \
    fprintf(config_file, "%s = %d ;(int)\n", #n, MiceConfig.s.n);

#define CFG_hex(s, n, precision, default, comment)                                       \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment);                \
    fprintf(config_file, "%s = %.*X ;(hex, %d byte%s)\n", #n, precision, MiceConfig.s.n, \
            precision, precision == 1 ? "" : "s");

#define CFG_ipv4(s, n, a, b, c, d, comment)                                                \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment);                  \
    fprintf(config_file, "%s = %hhu.%hhu.%hhu.%hhu ;(ipv4)\n", #n, (MiceConfig.s.n >> 24), \
            ((MiceConfig.s.n >> 16) & 0xff), ((MiceConfig.s.n >> 8) & 0xff),               \
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
#define CFG_ipv4(s, n, a, b, c, d, comment)                                          \
    else if (_stricmp(section, #s) == 0 && _stricmp(name, #n) == 0) {                \
        cfg->s.n = strtol(value, &end, 16);                                          \
        unsigned char ip_a, ip_b, ip_c, ip_d;                                        \
        if (sscanf_s(value, "%hhu.%hhu.%hhu.%hhu", &ip_a, &ip_b, &ip_c, &ip_d) == 4) \
            cfg->s.n = (ip_a << 24) | (ip_b << 16) | (ip_c << 8) | ip_d;             \
        else                                                                         \
            cfg->s.n = (a << 24) | (b << 16) | (c << 8) | d;                         \
    }

#include "config.def"

    return 1;
}

const unsigned int RES_W[8] = { 640, 640, 1024, 1024, 1280, 1280, 1360, 1920 };
const unsigned int RES_H[8] = { 480, 480, 600, 768, 720, 1024, 768, 1080 };
void load_mice_config() {
    if (ini_parse(CONFIG_PATH, handler, &MiceConfig) < 0) {
        make_default_config();
        printf("Can't load '%s', using defaults\n", CONFIG_PATH);
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

    if (MiceConfig.keys.board_count < 0) MiceConfig.keys.board_count = 0;
    if (MiceConfig.keys.board_count > JVS_IO_MAX) MiceConfig.keys.board_count = JVS_IO_MAX;

    char *text = MiceConfig.keys.keys;
    if (text) {
        char *copy = (char *)malloc(strlen(text) + 1);
        memcpy_s(copy, strlen(text) + 1, text, strlen(text) + 1);

        char *next_token;
        char *token = strtok_s(copy, ",", &next_token);
        int board = 0;
        int state = 0;
        int n = 0;
        while (token != NULL) {
            int val = atoi(token);
            if (state == 0) {
                jvsKeybindings[board].test = val;
                state = 1;
            } else {
                if (state == 1) {
                    jvsKeybindings[board].buttons[n] = val;
                    state = 2;
                } else {
                    jvsKeybindings[board].invert[n] = val;
                    state = 1;
                    if (n++ == JVS_BUTTON_PAIR_COUNT * 2) {
                        n = 0;
                        state = 0;
                        if (board++ == JVS_IO_MAX) break;
                    }
                }
            }
            token = strtok_s(NULL, ",", &next_token);
        }
        free(copy);
    }
}
