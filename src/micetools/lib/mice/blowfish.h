#include <Windows.h>

void BlowfishEncrypt(LPDWORD lpLeft, LPDWORD lpRight);
void BlowfishDecrypt(LPDWORD lpLeft, LPDWORD lpRight);
void BlowfishSetKey(LPBYTE lpKey, DWORD nKeyLen);
