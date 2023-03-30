#include <Windows.h>
#include <stdio.h>

#include "../lib/mice/mice.h"
#include "locate.h"
#include "spawn.h"

bool debug_wait = false;
int boot_delay = 0;
char exe_name[MAX_PATH + 1] = "";
char commandline[MAX_PATH + 1] = "";

static void print_help(char* exe) {
    log_info(plfBoot, "Usage: %s [-h] [-t] [-b executable.exe] [-d]", exe);
    log_info(plfBoot, "  -h: Print this help message and exit");
    log_info(plfBoot, "  -b: Specify the game binary to use");
    log_info(plfBoot, "  --mice-d: Wait for a debugger to attach when starting");
    log_info(plfBoot, "  --mice.<config>=<setting>: Set launcher options");
    exit(0);
}

static bool parse_cmdline(int argc, char* argv[]) {
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

static DWORD WINAPI MiceMailslotWatcher(HANDLE* pSlot) {
    // Enable colour support in conhost
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hConsole, &dwMode))
        SetConsoleMode(hConsole, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    HANDLE hLogFile = INVALID_HANDLE_VALUE;
    if (MiceConfig.mice.log_to_file) {
        hLogFile = CreateFileA(MiceConfig.mice.log_file, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                               CREATE_ALWAYS, 0, NULL);
        if (hLogFile == INVALID_HANDLE_VALUE) {
            fprintf(stderr, "Failed to initialise logging: open %s failed: %d",
                    MiceConfig.mice.log_file, GetLastError());
            ExitProcess(2);
        }
    }

    OVERLAPPED ov;
    HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    ov.Offset = ov.OffsetHigh = 0;
    ov.hEvent = hEvent;

    char readBuffer[4096];
    DWORD nRead, nBytes;
    while (1) {
        nRead = nBytes = 0;
        ReadFile(*pSlot, readBuffer, sizeof readBuffer, &nRead, &ov);
        GetOverlappedResult(*pSlot, &ov, &nBytes, TRUE);
        DWORD nWrote;
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), readBuffer, nBytes, &nWrote, NULL);

        if (hLogFile != INVALID_HANDLE_VALUE) {
            // The only VT100 sequences being used are colours, so this lazy approach works
            BOOL inVt100 = FALSE;
            for (int i = 0; i < nBytes; i++) {
                if (readBuffer[i] == '\033') {
                    inVt100 = TRUE;
                    continue;
                }
                if (inVt100 && readBuffer[i] == 'm') {
                    inVt100 = FALSE;
                    continue;
                }
                if (!inVt100) WriteFile(hLogFile, &(readBuffer[i]), 1, &nWrote, NULL);
            }
            FlushFileBuffers(hLogFile);
        }
    }
}

static BOOL setup_mailslot(void) {
    DWORD pid = GetCurrentProcessId();
    char slotName[MAX_PATH + 1];
    sprintf_s(slotName, MAX_PATH, "\\\\.\\mailslot\\micelog%d", pid);

    HANDLE* pSlot = malloc(sizeof(HANDLE));
    if (pSlot == NULL) {
        fprintf(stderr, "malloc(pSlot) failed\n");
        return FALSE;
    }
    *pSlot = CreateMailslotA(slotName, 0, MAILSLOT_WAIT_FOREVER, NULL);
    if (*pSlot == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Creation of mailslot failed %d\n", GetLastError());
        return FALSE;
    }

    CreateThread(NULL, 0, MiceMailslotWatcher, pSlot, 0, NULL);

    return TRUE;
}

/**
 * Because logging is via a mailslot, we need to give it a chance to flush messages before
 * terminating. While we could engineer something overly fancy, the only situation this ever occurs
 * is in the main() function just below.
 *
 * I'll let the code speak for itself.
 */
static int terminate(int err) {
    Sleep(100);

    return err;
}

HANDLE hGameProc = INVALID_HANDLE_VALUE;
HANDLE hJob = INVALID_HANDLE_VALUE;
BOOL WINAPI MiceHandlerRoutine(DWORD CtrlType) {
    if (hJob != INVALID_HANDLE_VALUE) CloseHandle(hJob);
    // if (hGameProc != INVALID_HANDLE_VALUE) TerminateProcess(hGameProc, 0);
}

int main(int argc, char* argv[]) {
    load_mice_config();
    if (!setup_mailslot()) return 1;
    setup_logging(GetCurrentProcessId());

    log_info(plfBoot, "Micetools version: %s", MICE_VERSION);

    CHAR workDir[MAX_PATH + 1];
    GetCurrentDirectory(MAX_PATH, workDir);
    log_info(plfBoot, "Current directory: %s", workDir);

    if (!parse_cmdline(argc, argv)) return terminate(0);

    if (exe_name[0] == '\0' && MiceConfig.launcher.game_binary[0] != '\0') {
        snprintf(exe_name, sizeof exe_name, "%s", MiceConfig.launcher.game_binary);
    }
    if (MiceConfig.launcher.wait_for_debugger) debug_wait = true;
    boot_delay = MiceConfig.launcher.startup_delay;

    if (exe_name[0] == '\0') {
        if (!locate_game(exe_name, MAX_PATH + 1)) {
            log_error(plfBoot, "Fatal: Failed to locate a game");
            return terminate(0);
        }
    } else {
        DWORD dwAttrib = GetFileAttributes(exe_name);
        if (dwAttrib == INVALID_FILE_ATTRIBUTES || dwAttrib & FILE_ATTRIBUTE_DIRECTORY) {
            log_error(plfBoot, "Fatal: %s: no such file found", exe_name);
            return terminate(0);
        }
    }

    spawn_pcp_processes();

    log_info(plfBoot, "exec: %s %s", exe_name, commandline);

    char micepath[MAX_PATH + 1];
    if (!locate_library(micepath, MAX_PATH + 1)) {
        log_error(plfBoot, "Fatal: Failed to locate micelib. Check your mice_dll setting!");
        return terminate(0);
    }

    hJob = CreateJobObject(NULL, NULL);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION info;
    info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &info, sizeof info);

    char* extra_injections = MiceConfig.launcher.inject;
    hGameProc = start_and_inject(hJob, exe_name, commandline, micepath, debug_wait, boot_delay,
                                 extra_injections, 0);
    if (!hGameProc) return terminate(-1);

    SetConsoleCtrlHandler(MiceHandlerRoutine, TRUE);

    if (FAILED(WaitForSingleObject(hGameProc, INFINITE))) {
        log_error(plfBoot, "Fatal: WaitForSingleObject failed: %03x", GetLastError());
    } else {
        log_info(plfBoot, "Shutting down");
        CloseHandle(hGameProc);
    }
    return terminate(0);
}
