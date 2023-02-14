#pragma once

#include <stdint.h>
#include <stdbool.h>

#define CONFIG_PATH "mice.ini"

typedef struct config {
#define SECTION(s, comment) struct {
#define CFG_str(s, n, default, comment) char* n;
#define CFG_bool(s, n, default, comment) bool n;
#define CFG_int(s, n, default, comment) int n;
#define CFG_hex(s, n, precision, default, comment) int n;
#define CFG_ipv4(s, n, a, b, c, d, comment) unsigned int n;
#define ENDSECTION(s) } s;
#include "config.def"
    bool _keep_linter_happy;
} config_t;

extern config_t MiceConfig;

void make_default_config();
void save_current_config();
void load_mice_config();
