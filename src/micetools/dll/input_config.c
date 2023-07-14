#include "common.h"

int g_MiceJvsNumBoards = 0;
MICE_JVS _MiceJvsBoards[JVS_IO_MAX];

void save_keybind_config() {
    HANDLE hFile = CreateFileA(KEYBINDS_PATH, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE, NULL,
                               CREATE_ALWAYS, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Failed to open keybinds file: %d", GetLastError());
        return;
    }

    SetFilePointer(hFile, 8, NULL, 0);

    DWORD nWrote;
    DWORD totalSize = 0;
    WriteFile(hFile, &g_MiceJvsNumBoards, sizeof g_MiceJvsNumBoards, &nWrote, NULL);
    totalSize += nWrote;

    for (int board = 0; board < g_MiceJvsNumBoards; board++) {
        WriteFile(hFile, &_MiceJvsBoards[board].m_Players, sizeof _MiceJvsBoards[board].m_Players,
                  &nWrote, NULL);
        totalSize += nWrote;
        WriteFile(hFile, &_MiceJvsBoards[board].m_ButtonsPerPlayer,
                  sizeof _MiceJvsBoards[board].m_ButtonsPerPlayer, &nWrote, NULL);
        totalSize += nWrote;
        WriteFile(hFile, &_MiceJvsBoards[board].m_Coins, sizeof _MiceJvsBoards[board].m_Coins,
                  &nWrote, NULL);
        totalSize += nWrote;
        WriteFile(hFile, &_MiceJvsBoards[board].m_NumButtons,
                  sizeof _MiceJvsBoards[board].m_NumButtons, &nWrote, NULL);
        totalSize += nWrote;

        WriteFile(hFile, _MiceJvsBoards[board].m_Bindings,
                  sizeof *_MiceJvsBoards[board].m_Bindings * _MiceJvsBoards[board].m_NumButtons,
                  &nWrote, NULL);
        totalSize += nWrote;
    }

    SetFilePointer(hFile, 0, NULL, 0);
    WriteFile(hFile, &totalSize, sizeof totalSize, &nWrote, NULL);

    amiCrc32RInit();
    DWORD readLeft = totalSize;
    DWORD crc32 = 0;
    SetFilePointer(hFile, 8, NULL, 0);
    while (readLeft) {
        BYTE readBuf[0x1000];
        DWORD toRead = sizeof readBuf;
        if (readLeft < toRead) toRead = readLeft;
        DWORD nRead;
        ReadFile(hFile, readBuf, toRead, &nRead, NULL);

        if (nRead == 0) {
            fprintf(stderr, "Binding saving failed %d\n", GetLastError());
            CloseHandle(hFile);
            return;
        }
        crc32 = amiCrc32RCalc(nRead, readBuf, crc32);
        readLeft -= nRead;
    }
    SetFilePointer(hFile, 4, NULL, 0);
    WriteFile(hFile, &crc32, sizeof crc32, &nWrote, NULL);

    CloseHandle(hFile);
}
void load_keybind_config() {
    HANDLE hFile =
        CreateFileA(KEYBINDS_PATH, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    DWORD nRead;
    DWORD nSize = 0;
    ReadFile(hFile, &nSize, sizeof nSize, &nRead, NULL);

    SetFilePointer(hFile, 8, NULL, 0);
    amiCrc32RInit();
    DWORD crc32 = 0;
    while (nSize) {
        BYTE readBuf[0x1000];
        DWORD toRead = sizeof readBuf;
        if (nSize < toRead) toRead = nSize;
        ReadFile(hFile, readBuf, toRead, &nRead, NULL);

        if (nRead == 0) {
            log_error(plfBoot, "Binding reading failed\n");
            CloseHandle(hFile);
            return;
        }
        crc32 = amiCrc32RCalc(nRead, readBuf, crc32);
        nSize -= nRead;
    }

    DWORD savedCrc;
    SetFilePointer(hFile, 4, NULL, 0);
    ReadFile(hFile, &savedCrc, sizeof savedCrc, &nRead, NULL);
    if (savedCrc != crc32) {
        log_error(plfBoot, "Binding file CRC failed (%08x!=%08x)\n", savedCrc, crc32);
        CloseHandle(hFile);
        return;
    }

    ReadFile(hFile, &g_MiceJvsNumBoards, sizeof g_MiceJvsNumBoards, &nRead, NULL);

    for (int board = 0; board < g_MiceJvsNumBoards; board++) {
        ReadFile(hFile, &_MiceJvsBoards[board].m_Players, sizeof _MiceJvsBoards[board].m_Players,
                 &nRead, NULL);
        ReadFile(hFile, &_MiceJvsBoards[board].m_ButtonsPerPlayer,
                 sizeof _MiceJvsBoards[board].m_ButtonsPerPlayer, &nRead, NULL);
        ReadFile(hFile, &_MiceJvsBoards[board].m_Coins, sizeof _MiceJvsBoards[board].m_Coins,
                 &nRead, NULL);
        ReadFile(hFile, &_MiceJvsBoards[board].m_NumButtons,
                 sizeof _MiceJvsBoards[board].m_NumButtons, &nRead, NULL);

        free(_MiceJvsBoards[board].m_Bindings);
        _MiceJvsBoards[board].m_Bindings =
            malloc(sizeof *_MiceJvsBoards[board].m_Bindings * _MiceJvsBoards[board].m_NumButtons);
        ReadFile(hFile, _MiceJvsBoards[board].m_Bindings,
                 sizeof *_MiceJvsBoards[board].m_Bindings * _MiceJvsBoards[board].m_NumButtons,
                 &nRead, NULL);

        free(_MiceJvsBoards[board].m_ButtonStates);
        _MiceJvsBoards[board].m_ButtonStates = malloc(sizeof *_MiceJvsBoards[board].m_ButtonStates *
                                                      _MiceJvsBoards[board].m_NumButtons);
    }

    CloseHandle(hFile);
}
