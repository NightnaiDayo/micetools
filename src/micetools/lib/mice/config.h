#pragma once

#include <stdint.h>
#include <stdbool.h>

#define CONFIG_PATH MiceIpcRelativePath("config.ini")
#define KEYBINDS_PATH MiceIpcRelativePath("keybinds.bin")

#define MICE_PLATFORM_RINGWIDE 0
#define MICE_PLATFORM_RINGEDGE1 1  // Explictly RingEdge1, to avoid confusion
#define MICE_PLATFORM_RINGEDGE2 2

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

void save_current_config(bool writeDefault);
void load_mice_config();
void load_keybind_config();
void save_keybind_config();
