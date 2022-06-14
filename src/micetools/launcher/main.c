#include <Windows.h>
#include <stdio.h>

#include "../lib/mice/mice.h"
#include "locate.h"

const char* VERSION = "0.0-pre";

int main(int argc, CHAR** argv) {
    setup_logging();

    log_info(BOOT_LOGGER, "Micetools version: %s", VERSION);

    char path[MAX_PATH + 1];
    if (!locate_game(path, MAX_PATH + 1)) {
        log_error(BOOT_LOGGER, "Failed to locate a game");
        return 0;
    }

    char* cmdline = "";

    log_info(BOOT_LOGGER, "%s %s", path, cmdline);

    char micepath[MAX_PATH + 1];
    if (!locate_library(micepath, MAX_PATH + 1)) {
        log_error(BOOT_LOGGER, "Failed to locate micelib");
        return 0;
    }

    HANDLE game_proc = start_and_inject(path, cmdline, micepath);
    if (!game_proc) return -1;

    if (FAILED(WaitForSingleObject(game_proc, INFINITE))) {
        log_error(BOOT_LOGGER, "WaitForSingleObject failed: %03x", GetLastError());
    } else {
        log_info(BOOT_LOGGER, "Shutting down");
        CloseHandle(game_proc);
    }
    return 0;
}
