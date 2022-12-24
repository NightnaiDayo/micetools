#pragma once
#include "../hooks/_hooks.h"
#include "../common.h"
#include "../../lib/am/amEeprom.h"

FnDeviceIoControl mxhwreset_DeviceIoControl;
void setup_mxhwreset();

FnDeviceIoControl mxjvs_DeviceIoControl;
void setup_mxjvs();

FnDeviceIoControl mxsmbus_DeviceIoControl;
void setup_mxsmbus();

FnDeviceIoControl mxsram_DeviceIoControl;
FnSetFilePointer mxsram_SetFilePointer;
FnWriteFile mxsram_WriteFile;
FnReadFile mxsram_ReadFile;
void setup_mxsram();

FnDeviceIoControl mxsuperio_DeviceIoControl;
void setup_mxsuperio();

FnDeviceIoControl columba_DeviceIoControl;
void setup_columba();

FnDeviceIoControl mxparallel_DeviceIoControl;
void setup_mxparallel();
