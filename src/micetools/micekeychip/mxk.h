#include <stdio.h>

#include "../../../subprojects/inih_dep/ini.h"
#include "../lib/libpcp/libpcp.h"
#include "../lib/mice/kcf.h"
#include "callbacks/callbacks.h"
#include "config.h"

extern AM_KCF mxkKcfConfig;

void mxkBinaryCallback(pcpa_t* stream, void* data);
int mxkInit();
e_pcpa_t mxkPcpStreamInit();
e_pcpa_t mxkPcpServer();
int mxkMain(void);
