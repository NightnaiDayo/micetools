#include <Windows.h>
#include <stdio.h>

#include "../lib/mice/mice.h"
#include "locate.h"
#include "spawn.h"

bool debug_wait = false;
int boot_delay = 0;
char exe_name[MAX_PATH + 1] = "";
char commandline[MAX_PATH + 1] = "";

void print_help(char* exe) {
    log_info(plfBoot, "Usage: %s [-h] [-t] [-b executable.exe] [-d]", exe);
    log_info(plfBoot, "  -h: Print this help message and exit");
    log_info(plfBoot, "  -b: Specify the game binary to use");
    log_info(plfBoot, "  --mice-d: Wait for a debugger to attach when starting");
    log_info(plfBoot, "  --mice.<config>=<setting>: Set launcher options");
    exit(0);
}

bool parse_cmdline(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            print_help(argv[0]);
        } else if (strcmp(argv[i], "-b") == 0) {
            if (i + 1 == argc) print_help(argv[0]);
            char* val = argv[++i];
            memcpy(exe_name, val, strlen(val) + 1);
        } else if (strcmp(argv[i], "--mice-d") == 0) {
            debug_wait = true;
        } else if (strncmp(argv[i], "--mice.", strlen("--mice.")) == 0) {
            char key[256];
            char value[256];
            key[0] = value[0] = '\0';

            char* arg = argv[i] + strlen("--mice.");
            int j = 0;
            for (; j < _countof(key) - 2; j++) {
                if (arg[j] == '\0') break;
                if (arg[j] == '=') {
                    strcpy_s(value, _countof(value), arg + j + 1);
                    break;
                }
                key[j] = arg[j];
            }
            key[j] = '\0';
            bool handled = false;

            for (j = 0; j < _countof(mxspawns); j++) {
                if (strcmp(key, mxspawns[j].name) != 0) continue;
                handled = true;

                if (strcmp(value, "no") == 0)
                    mxspawns[j].mode = SPAWN_NONE;
                else if (strcmp(value, "mx") == 0)
                    mxspawns[j].mode |= SPAWN_REAL;
                else if (strcmp(value, "md") == 0)
                    mxspawns[j].mode |= SPAWN_DUMMY;
                else {
                    log_error(plfBoot, "Unknown spawn mode: %s=%s", key, value);
                    return false;
                }
            }

            if (!handled) {
                log_error(plfBoot, "Unknown launcher config: %s=%s", key, value);
                return false;
            }
        } else {
            if (commandline[0] == 0)
                snprintf(commandline, sizeof commandline, "%s", argv[i]);
            else
                snprintf(commandline, sizeof commandline, "%s %s", commandline, argv[i]);
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
    load_mice_config();
    setup_logging();

    log_info(plfBoot, "Micetools version: %s", MICE_VERSION);

    CHAR workDir[MAX_PATH + 1];
    GetCurrentDirectory(MAX_PATH, workDir);
    log_info(plfBoot, "Current directory: %s", workDir);

    if (!parse_cmdline(argc, argv)) return 0;

    if (exe_name[0] == '\0' && MiceConfig.launcher.game_binary[0] != '\0') {
        snprintf(exe_name, sizeof exe_name, "%s", MiceConfig.launcher.game_binary);
    }
    if (MiceConfig.launcher.wait_for_debugger) debug_wait = true;
    boot_delay = MiceConfig.launcher.startup_delay;

    if (exe_name[0] == '\0') {
        if (!locate_game(exe_name, MAX_PATH + 1)) {
            log_error(plfBoot, "Fatal: Failed to locate a game");
            return 0;
        }
    } else {
        DWORD dwAttrib = GetFileAttributes(exe_name);
        if (dwAttrib == INVALID_FILE_ATTRIBUTES || dwAttrib & FILE_ATTRIBUTE_DIRECTORY) {
            log_error(plfBoot, "Fatal: %s: no such file found", exe_name);
            return 0;
        }
    }

    spawn_pcp_processes();

    log_info(plfBoot, "exec: %s %s", exe_name, commandline);

    char micepath[MAX_PATH + 1];
    if (!locate_library(micepath, MAX_PATH + 1)) {
        log_error(plfBoot, "Fatal: Failed to locate micelib. Check your mice_dll setting!");
        return 0;
    }

    char* extra_injections = MiceConfig.launcher.inject;
    HANDLE game_proc =
        start_and_inject(exe_name, commandline, micepath, debug_wait, boot_delay, extra_injections, 0);
    if (!game_proc) return -1;

    if (FAILED(WaitForSingleObject(game_proc, INFINITE))) {
        log_error(plfBoot, "Fatal: WaitForSingleObject failed: %03x", GetLastError());
    } else {
        log_info(plfBoot, "Shutting down");
        CloseHandle(game_proc);
    }
    return 0;
}
