#include "../com.h"
#include "../files.h"
#include <initguid.h>

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

DEFINE_GUID(MXSMBUS_GUID, 0x5C49E1FE, 0x3FEC, 0x4B8D, 0xA4, 0xB5, 0x76, 0xBE, 0x70, 0x25, 0xD8, 0x42);
DEFINE_GUID(PLATFORM_GUID, 0x86E0D1E0, 0x8089, 0x11D0, 0x9C, 0xE4, 0x08, 0x00, 0x3e, 0x30, 0x1F, 0x73);
