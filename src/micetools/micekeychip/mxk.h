#include <stdio.h>

#include "../../../subprojects/inih_dep/ini.h"
#include "../lib/libpcp/libpcp.h"
#include "callbacks/callbacks.h"
#include "config.h"

void mxkBinaryCallback(pcpa_t* stream, void* data);
int mxkInit();
e_pcpa_t mxkPcpStreamInit();
e_pcpa_t mxkPcpServer();
int mxkMain(void);
