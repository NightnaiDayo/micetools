#include "_hooks.h"

void hook_all() {
    if (MiceConfig.hooks.logging) hook_logging();
    if (MiceConfig.hooks.gui) {
        hook_gui();
        setup_hud_gui();
    }
    if (MiceConfig.hooks.setupapi) hook_setupapi();
    if (MiceConfig.hooks.commio) hook_commio();
    if (MiceConfig.hooks.io) hook_io();
    if (MiceConfig.hooks.processes) hook_processes();
    if (MiceConfig.hooks.network) hook_network();
    if (MiceConfig.hooks.time) hook_time();
    if (MiceConfig.hooks.registry) hook_registry();
    if (MiceConfig.hooks.drives) hook_drives();
    if (MiceConfig.hooks.system) hook_system();
}
