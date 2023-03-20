#include "locate.h"

#include "../lib/mice/mice.h"

const char* KNOWN_GAMES[] = {
    // Preferentially use a decrypted dump if present
    "maimai_dump_.exe",

    // Specific games
    "DOA5A.exe",

    // Generic
    "RingGame.exe",

    // We currently have no hope of running undumped maimai but whatever
    "maimai.exe",
};

bool locate_file(char* path, size_t len, const char* exe) {
    WIN32_FIND_DATA fdFile;
    HANDLE hFind = NULL;

    char work_path[2048];
    snprintf(work_path, sizeof work_path, ".\\*.*");

    if ((hFind = FindFirstFile(work_path, &fdFile)) == INVALID_HANDLE_VALUE) return false;

    do {
        if (!strcmp(fdFile.cFileName, ".") || !strcmp(fdFile.cFileName, "..")) continue;

        if (fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

        if (!strcmp(fdFile.cFileName, exe)) {
            snprintf(path, len, ".\\%s", fdFile.cFileName);
            FindClose(hFind);
            return true;
        }
    } while (FindNextFile(hFind, &fdFile));

    FindClose(hFind);
    return false;
}

bool locate_game(char* path, size_t len) {
    // This is slightly wasteful, but is needed to ensure priority!
    for (size_t i = 0; i < ((sizeof KNOWN_GAMES) / (sizeof KNOWN_GAMES[0])); i++) {
        if (locate_file(path, len, KNOWN_GAMES[i])) {
            return true;
        }
    }
    return false;
}

bool locate_library(char* path, size_t len) {
    if (locate_file(path, len, MiceConfig.launcher.mice_dll)) return true;

    // Don't call DllMain! We don't want to hook ourself!
    HANDLE mice = LoadLibraryExA(MiceConfig.launcher.mice_dll, NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (mice != NULL) {
        GetModuleFileNameA(mice, path, len);
        return true;
    }

    return false;
}
