#include "_hooks.h"

void hook_all() {
    hook_logging();
    hook_gui();
    hook_setupapi();
    hook_commio();
    hook_io();
    hook_processes();
    hook_network();
    hook_time();
    hook_registry();
    hook_drives();
}
