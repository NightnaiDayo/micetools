#pragma once

#include <Windows.h>
#include <Winsock2.h>
#include <initguid.h>
#include <windns.h>
#pragma comment(lib, "Shlwapi.lib")
#include <shlwapi.h>
#pragma comment(lib, "d3d9.lib")
#include <d3d9.h>
#pragma comment(lib, "comctl32.lib")
#include <Iphlpapi.h>
#include <commctrl.h>
#include <memory.h>
#include <setupapi.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../lib/mice/mice.h"
#include "./util/_util.h"
#include "micefs.h"
#include "input_config.h"

#define size2int(x) ((int)((x) & 0x7fffffff))
#define size2uint(x) ((unsigned int)((x) & 0xffffffff))

void _MiceGotGameId(char game_id[4]);
BOOL MiceGuessGameId(char* lpGameId);

extern WCHAR exeName[MAX_PATH + 1];
extern size_t imageOffset;

extern BOOL g_bIsInDllMain;
