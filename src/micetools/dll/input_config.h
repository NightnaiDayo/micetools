#include "drivers/jvs_boards/jvs.h"

void save_keybind_config();
void load_keybind_config();

extern MICE_JVS _MiceJvsBoards[JVS_IO_MAX];
extern int g_MiceJvsNumBoards;
