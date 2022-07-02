#include <Windows.h>
#include <stdio.h>

#include "../lib/mice/mice.h"
#include "locate.h"

const char* VERSION = "0.0-pre";

bool boot_delay = false;
bool gametest = false;
char exe_name[MAX_PATH + 1] = "";

void print_help(char* exe) {
    fprintf(stderr, "Usage: %s [-h] [-t] [-b executable.exe] [-d]\n", exe);
    fprintf(stderr, "  -h: Print this help message and exit\n");
    fprintf(stderr, "  -t: Start the game in test mode\n");
    fprintf(stderr, "  -b: Specify the game binary to use\n");
    fprintf(stderr, "  -d: Wait for a debugger to attach when starting\n");
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
        } else if (strcmp(argv[i], "-d") == 0) {
            boot_delay = true;
        } else if (strcmp(argv[i], "-t") == 0) {
            gametest = true;
        }
    }
}

int main(int argc, char* argv[]) {
    fprintf(stderr, "Micetools version: %s\n", VERSION);

    parse_cmdline(argc, argv);

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

    char* cmdline = gametest ? ". gametest" : "";
    fprintf(stderr, "exec: %s %s\n", exe_name, cmdline);

    char micepath[MAX_PATH + 1];
    if (!locate_library(micepath, MAX_PATH + 1)) {
        fprintf(stderr, "Fatal: Failed to locate micelib\n");
        return 0;
    }

    HANDLE game_proc = start_and_inject(exe_name, cmdline, micepath, boot_delay);
    if (!game_proc) return -1;

    if (FAILED(WaitForSingleObject(game_proc, INFINITE))) {
        fprintf(stderr, "Fatal: WaitForSingleObject failed: %03x\n", GetLastError());
    } else {
        fprintf(stderr, "Shutting down\n");
        CloseHandle(game_proc);
    }
    return 0;
}
