#include <Windows.h>

#include "../../dll/smbus.h"

DEFINE_GUID(MXSMBUS_GUID, 0x5C49E1FE, 0x3FEC, 0x4B8D, 0xA4, 0xB5, 0x76, 0xBE, 0x70, 0x25, 0xD8, 0x42);
DEFINE_GUID(PLATFORM_GUID, 0x86E0D1E0, 0x8089, 0x11D0, 0x9C, 0xE4, 0x08, 0x00, 0x3e, 0x30, 0x1F, 0x73);

HANDLE amEepromCreateDeviceFile(const GUID *guid, LPCSTR resource, DWORD member_index);

BOOL amEepromReadBlock(HANDLE mxsmbus, BYTE reg, BYTE len, BYTE *data);
BOOL amEepromWriteBlock(HANDLE mxsmbus, BYTE reg, BYTE len, BYTE *data);

void amEepromRepairChecksum(BYTE* data);
