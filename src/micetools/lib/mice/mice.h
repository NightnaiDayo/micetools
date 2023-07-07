#pragma once

#include <Windows.h>
#include <stdlib.h>

#include "../am/am.h"
#include "../ami/ami.h"
#include "config.h"
#include "exe.h"
#include "ioctl.h"
#include "ipc.h"
#include "kcf.h"
#include "log.h"
#include "patch.h"
#include "ringbuf.h"

#define SRAM_PATH MiceIpcRelativePath("sram.bin")

BOOL GetSerialNumbers(LPSTR szMainId, LPSTR szKeyId);
