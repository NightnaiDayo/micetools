#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../../subprojects/inih_dep/ini.h"

config_t MiceConfig = {
#define SECTION(s, comment) .s = {
#define CFG_str(s, n, default, comment) .n = default,
#define CFG_bool(s, n, default, comment) .n = default,
#define CFG_int(s, n, default, comment) .n = default,
#define CFG_hex(s, n, precision, default, comment) .n = 0x##default,
#define ENDSECTION(s) \
    }                 \
    ,
#include "config.def"
    ._keep_linter_happy = true
};

void fprintf_prefix(FILE *file, const char *prefix, const char *text) {
    char* copy = (char*)malloc(strlen(text) + 1);
    memcpy_s(copy, strlen(text) + 1, text, strlen(text) + 1);

    char* next_token;
    char* token = strtok_s(copy, "\n", &next_token);
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

#define CFG_str(s, n, default, comment)                                \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment); \
    fprintf(config_file, "; (string) default = %s\n", default);        \
    fprintf(config_file, "%s = %s\n", #n, default);

#define CFG_bool(s, n, default, comment)                                         \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment);           \
    fprintf(config_file, "; (bool) default = %s\n", default ? "true" : "false"); \
    fprintf(config_file, "%s = %s\n", #n, default ? "true" : "false");

#define CFG_int(s, n, default, comment)                                \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment); \
    fprintf(config_file, "; (int) default = %d\n", default);           \
    fprintf(config_file, "%s = %d\n", #n, default);

#define CFG_hex(s, n, precision, default, comment)                            \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment);        \
    fprintf(config_file, "; (hex) default = %.*X\n", precision, 0x##default); \
    fprintf(config_file, "%s = %.*X\n", #n, precision, 0x##default);

#define SECTION(s, comment)                                            \
    if (!first_section) fprintf(config_file, "\n");                    \
    first_section = false;                                             \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment); \
    fprintf(config_file, "[%s]\n", #s);

#define HEADER(comment)                      \
    if (strlen(comment) != 0) fprintf_prefix(config_file, "; ", comment); \
    first_section = false;

#include "config.def"

    fclose(config_file);
}

int handler(void *user, const char *section, const char *name, const char *value) {
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

#include "config.def"

    return 1;
}

void load_mice_config() {
    if (ini_parse(CONFIG_PATH, handler, &MiceConfig) < 0) {
        make_default_config();
        printf("Can't load '%s', using defaults\n", CONFIG_PATH);
    }
}
