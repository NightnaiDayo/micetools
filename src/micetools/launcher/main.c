#include <Windows.h>
#include <stdio.h>

#include "../lib/mice/mice.h"
#include "locate.h"

const char* VERSION = "0.0-pre";

bool boot_delay = false;
bool gametest = false;
char exe_name[MAX_PATH + 1] = "";

void print_help(char* exe) {
    log_info(BOOT_LOGGER, "Usage: %s [-h] [-t] [-b executable.exe] [-d]", exe);
    log_info(BOOT_LOGGER, "  -h: Print this help message and exit");
    log_info(BOOT_LOGGER, "  -t: Start the game in test mode");
    log_info(BOOT_LOGGER, "  -b: Specify the game binary to use");
    log_info(BOOT_LOGGER, "  -d: Wait for a debugger to attach when starting");
    exit(0);
}

void parse_cmdline(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            print_help(argv[0]);
        } else if (strcmp(argv[i], "-b") == 0) {
            if (i + 1 == argc) print_help(argv[0]);
            strcpy(exe_name, argv[++i]);
        } else if (strcmp(argv[i], "-d") == 0) {
            boot_delay = true;
        } else if (strcmp(argv[i], "-t") == 0) {
            gametest = true;
        }
    }
}

int main(int argc, char* argv[]) {
    setup_logging();

    log_info(BOOT_LOGGER, "Micetools version: %s", VERSION);

    parse_cmdline(argc, argv);

    if (exe_name[0] == '\0') {
        if (!locate_game(exe_name, MAX_PATH + 1)) {
            log_error(BOOT_LOGGER, "Failed to locate a game");
            return 0;
        }
    } else {
        DWORD dwAttrib = GetFileAttributes(exe_name);
        if (dwAttrib == INVALID_FILE_ATTRIBUTES || dwAttrib & FILE_ATTRIBUTE_DIRECTORY) {
            log_error(BOOT_LOGGER, "%s: no such file found", exe_name);
        }
    }

    char* cmdline = gametest ? "gametest" : "";
    log_info(BOOT_LOGGER, "%s %s", exe_name, cmdline);

    char micepath[MAX_PATH + 1];
    if (!locate_library(micepath, MAX_PATH + 1)) {
        log_error(BOOT_LOGGER, "Failed to locate micelib");
        return 0;
    }

    HANDLE game_proc = start_and_inject(exe_name, cmdline, micepath, boot_delay);
    if (!game_proc) return -1;

    if (FAILED(WaitForSingleObject(game_proc, INFINITE))) {
        log_error(BOOT_LOGGER, "WaitForSingleObject failed: %03x", GetLastError());
    } else {
        log_info(BOOT_LOGGER, "Shutting down");
        CloseHandle(game_proc);
    }
    return 0;
}
