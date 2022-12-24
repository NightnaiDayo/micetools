#include <Windows.h>
#include <stdio.h>

#include "../lib/mice/mice.h"
#include "locate.h"

const char* VERSION = "0.0-pre";

bool debug_wait = false;
int boot_delay = 0;
bool gametest = false;
bool designviewer = false;
bool spriteviewer = false;
bool noisetest = false;
char exe_name[MAX_PATH + 1] = "";
char commandline[MAX_PATH + 1] = "";

void print_help(char* exe) {
    fprintf(stderr, "Usage: %s [-h] [-t] [-b executable.exe] [-d]\n", exe);
    fprintf(stderr, "  -h: Print this help message and exit\n");
    fprintf(stderr, "  -b: Specify the game binary to use\n");
    fprintf(stderr, "  -d: Wait for a debugger to attach when starting\n");
    fprintf(stderr, "  -t: Start the game in test mode\n");
    fprintf(stderr, "  -dv: Start the game in design viewer mode\n");
    fprintf(stderr, "  -sv: Start the game in sprite viewer mode\n");
    fprintf(stderr, "  -nt: Start the game in noisetest mode\n");
    exit(0);
}

void parse_cmdline(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            print_help(argv[0]);
        } else if (strcmp(argv[i], "-b") == 0) {
            if (i + 1 == argc) print_help(argv[0]);
            char* val = argv[++i];
            memcpy(exe_name, val, strlen(val) + 1);
        } else if (strcmp(argv[i], "--mice-d") == 0) {
            debug_wait = true;
        } else if (strcmp(argv[i], "-mice-t") == 0) {
            gametest = true;
        } else if (strcmp(argv[i], "-dv") == 0) {
            designviewer = true;
        } else if (strcmp(argv[i], "-sv") == 0) {
            spriteviewer = true;
        } else if (strcmp(argv[i], "-nt") == 0) {
            noisetest = true;
        } else {
            if (commandline[0] == 0)
                snprintf(commandline, sizeof commandline, "%s", argv[i]);
            else
                snprintf(commandline, sizeof commandline, "%s %s", commandline, argv[i]);
        }
    }
}

int main(int argc, char* argv[]) {
    load_mice_config();

    fprintf(stderr, "Micetools version: %s\n", VERSION);

    parse_cmdline(argc, argv);

    if (exe_name[0] == '\0' && MiceConfig.launcher.game_binary[0] != '\0') {
        snprintf(exe_name, sizeof exe_name, "%s", MiceConfig.launcher.game_binary);
    }
    if (MiceConfig.launcher.wait_for_debugger) debug_wait = true;
    boot_delay = MiceConfig.launcher.startup_delay;

    if (exe_name[0] == '\0') {
        if (!locate_game(exe_name, MAX_PATH + 1)) {
            fprintf(stderr, "Fatal: Failed to locate a game\n");
            return 0;
        }
    } else {
        DWORD dwAttrib = GetFileAttributes(exe_name);
        if (dwAttrib == INVALID_FILE_ATTRIBUTES || dwAttrib & FILE_ATTRIBUTE_DIRECTORY) {
            fprintf(stderr, "Fatal: %s: no such file found\n", exe_name);
            return 0;
        }
    }

    char* cmdline_mode = gametest       ? ". gametest"
                         : designviewer ? ". designviewer"
                         : spriteviewer ? ". spriteviewer"
                         : noisetest    ? ". noisetest"
                                        : "";
    char cmdline[MAX_PATH + 1];
    if (commandline[0] == 0)
        snprintf(cmdline, sizeof cmdline, "%s", cmdline_mode);
    else
        snprintf(cmdline, sizeof cmdline, "%s %s", cmdline_mode, commandline);
    fprintf(stderr, "exec: %s %s\n", exe_name, cmdline);

    char micepath[MAX_PATH + 1];
    if (!locate_library(micepath, MAX_PATH + 1)) {
        fprintf(stderr, "Fatal: Failed to locate micelib. Check your mice_dll setting!\n");
        return 0;
    }

    char* extra_injections = MiceConfig.launcher.inject;
    HANDLE game_proc =
        start_and_inject(exe_name, cmdline, micepath, debug_wait, boot_delay, extra_injections);
    if (!game_proc) return -1;

    if (FAILED(WaitForSingleObject(game_proc, INFINITE))) {
        fprintf(stderr, "Fatal: WaitForSingleObject failed: %03x\n", GetLastError());
    } else {
        fprintf(stderr, "Shutting down\n");
        CloseHandle(game_proc);
    }
    return 0;
}
