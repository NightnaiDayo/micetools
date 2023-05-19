#pragma once
#include <Windows.h>

#include "nfc.h"

typedef DWORD64 KEY_PAIR[2];

void NbgiWriteBNESignedBlock(PNFC_CARD lpCard, BYTE block, BYTE nKey, LPBYTE lpData);

void FelicaMacGenerateCK(DWORD64* ID, KEY_PAIR lpKeys);
void FelicaMacGenerateSK(DWORD64* random, KEY_PAIR ck, KEY_PAIR lpSk);
DWORD64 FelicaMacGenerateMAC(DWORD64* data, KEY_PAIR sk, DWORD64* random);
