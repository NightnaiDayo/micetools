#include <stdbool.h>

#define CONFIG_PATH "config.ini"

typedef struct config {
#define CFG_str(s, n, default, comment) char* s##_##n;
#define CFG_bool(s, n, default, comment) bool s##_##n;
#define CFG_int(s, n, default, comment) int s##_##n;
#define CFG_hex(s, n, precision, default, comment) int s##_##n;
#include "config.def"
    bool _keep_linter_happy;
} config_t;

extern config_t Config;

void mxkMakeDefaultConfig();
void mxkLoadConfig();
