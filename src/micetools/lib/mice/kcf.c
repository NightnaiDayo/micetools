#include "kcf.h"

#include <Windows.h>
#include <stdio.h>

inline static void scramble(LPBYTE data, DWORD a, DWORD b, DWORD c, DWORD d) {
    BYTE temp;

    temp = data[a];
    data[a] = data[b];
    data[b] = temp;

    temp = data[c];
    data[c] = data[d];
    data[d] = temp;
}

BOOL MiceParseKcf(LPCSTR lpFileName, PAM_KCF pKcf) {
    // Load in scrambled data
    HANDLE hFile =
        CreateFileA(lpFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    DWORD nNumberOfBytesRead;
    if (!ReadFile(hFile, pKcf, sizeof *pKcf, &nNumberOfBytesRead, NULL)) {
        CloseHandle(hFile);
        return FALSE;
    }
    CloseHandle(hFile);
    if (nNumberOfBytesRead != sizeof *pKcf) {
        return FALSE;
    }

    // Unscramble
    scramble(pKcf->m_Seed, 1, 8, 12, 15);
    scramble(pKcf->m_Key, 0, 4, 2, 14);
    scramble(pKcf->m_Iv, 0, 11, 5, 15);

    return TRUE;
}
