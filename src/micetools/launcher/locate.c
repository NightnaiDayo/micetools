#include "locate.h"

#include "../lib/mice/mice.h"

const char* KNOWN_GAMES[] = {
    // Ideally, we're running from the X: root
    "maimai\\maimai_dump_.exe",
    "maimai\\maimai_dump.exe",
    "maimai\\Game.exe",
    // Preferentially use a decrypted dump if present
    "maimai_dump_.exe",
    "maimai_dump.exe",
    "Game.exe",

    // Specific games
    "DOA5A.exe",

    // Generic
    "RingGame.exe",

    // We currently have no hope of running undumped maimai but whatever
    "maimai.exe",
};

bool locate_game(char* path, size_t len) {
    for (size_t i = 0; i < _countof(KNOWN_GAMES); i++) {
        if (PathFileExistsA(KNOWN_GAMES[i])) {
            snprintf(path, len, ".\\%s", KNOWN_GAMES[i]);
            return true;
        }
    }
    return false;
}
