#include <Windows.h>

DWORD64 MiceDesEncrypt(DWORD64 nData, DWORD64 nKey);
DWORD64 MiceDesDecrypt(DWORD64 nData, DWORD64 nKey);
DWORD64 Mice3Des_3K(DWORD64 nData, DWORD64 lpKeys[3]);
DWORD64 Mice3Des_2K(DWORD64 nData, DWORD64 lpKeys[2]);
DWORD64 Mice3Des_1K(DWORD64 nData, DWORD64 nKey);
