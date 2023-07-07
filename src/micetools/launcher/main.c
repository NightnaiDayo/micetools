#include <Windows.h>
#include <stdio.h>

#include "../lib/mice/ipc.h"
#include "../lib/mice/mice.h"
#include "../lib/util/pid.h"
#include "locate.h"
#include "spawn.h"

bool debug_wait = false;
int boot_delay = 0;
char exe_name[MAX_PATH + 1] = "";

BYTE _MiceLauncherNext = 2;
HANDLE _MiceLauncherNextEvent = INVALID_HANDLE_VALUE;

size_t nCommandLine = 0;
char* commandLine = NULL;

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
        if (strcmp(argv[i], "--mice-h") == 0) {
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
            nCommandLine += 1 + strlen(argv[i]) + 1;
            if (commandLine == NULL) {
                commandLine = malloc(nCommandLine);
                commandLine[0] = '\0';
            } else {
                commandLine = realloc(commandLine, nCommandLine);
            }
            strcat_s(commandLine, nCommandLine, " ");
            strcat_s(commandLine, nCommandLine, argv[i]);
            commandLine[nCommandLine - 1] = '\0';
        }
    }
    return true;
}

static DWORD WINAPI MiceIpcMessageWatcher(PVOID _) {
    // Enable colour support in conhost
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hConsole, &dwMode))
        SetConsoleMode(hConsole, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    HANDLE hLogFile = INVALID_HANDLE_VALUE;
    if (MiceConfig.mice.log_to_file) {
        hLogFile = CreateFileA(MiceConfig.mice.log_file, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
                               CREATE_ALWAYS, 0, NULL);
        if (hLogFile == INVALID_HANDLE_VALUE) {
            fprintf(stderr, "Failed to initialise logging: open %s failed: %d",
                    MiceConfig.mice.log_file, GetLastError());
            ExitProcess(2);
        }
    }

    while (1) {
        MICE_IPC_MESSAGE message;
        MiceIpcPopMessage(&message, TRUE);

        switch (message.m_Type) {
            case MICE_IPC_TYPE_LOG:
                // printf("%s", message.m_Message);

                DWORD nWrote;
                WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), message.m_Message, message.m_Size,
                          &nWrote, NULL);

                if (hLogFile != INVALID_HANDLE_VALUE) {
                    // The only VT100 sequences being used are colours, so this lazy approach works
                    BOOL inVt100 = FALSE;
                    for (unsigned int i = 0; i < message.m_Size; i++) {
                        if (message.m_Message[i] == '\033') {
                            inVt100 = TRUE;
                            continue;
                        }
                        if (inVt100 && message.m_Message[i] == 'm') {
                            inVt100 = FALSE;
                            continue;
                        }
                        if (!inVt100)
                            WriteFile(hLogFile, &(message.m_Message[i]), 1, &nWrote, NULL);
                    }
                    // FlushFileBuffers(hLogFile);
                }
                // FlushFileBuffers(GetStdHandle(STD_OUTPUT_HANDLE));

                break;
        }
    }
}

/**
 * Because logging is via IPC, we need to give it a chance to flush messages before
 * terminating. While we could engineer something overly fancy, the only situation
 * this ever occurs is in the main() function just below.
 *
 * I'll let the code speak for itself.
 */
static int terminate(int err, char* szMessage) {
    printf("Mice termination occuring... %d\n", err);
    log_info(plfBoot, "Terminating");
    Sleep(1000);

    extern unsigned char g_szFault[255 + 1];
    extern BYTE g_nFault;

    if (g_nFault) {
        MessageBeep(MB_ICONERROR);
        MessageBoxA(NULL, (LPCSTR)g_szFault, "System Fault",
                    MB_OK | MB_SETFOREGROUND | MB_SYSTEMMODAL);
    } else if (szMessage) {
        MessageBeep(MB_ICONERROR);
        MessageBoxA(NULL, szMessage, "Boot Failed", MB_OK | MB_SETFOREGROUND | MB_SYSTEMMODAL);
    }
    return err;
}

HANDLE hJob = INVALID_HANDLE_VALUE;
BOOL WINAPI MiceHandlerRoutine(DWORD CtrlType) {
    if (hJob != INVALID_HANDLE_VALUE) CloseHandle(hJob);
    ExitProcess(terminate(0, NULL));
}

static BOOL StartGame(LPPROCESS_INFORMATION lpPi, LPCSTR args) {
    // Clear out the next flag. If the game closes without setting this, we don't want to restart
    // the game.
    _MiceLauncherNext = 0;

    hJob = CreateJobObject(NULL, NULL);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION info;
    info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &info, sizeof info);

    size_t nFullCommandLine = strlen(exe_name) + 1;
    if (args != NULL && strlen(args) > 0) nFullCommandLine += 1 + strlen(args);
    if (nCommandLine > 0) nFullCommandLine += 1 + nCommandLine;
    char* fullCommandLine = malloc(nFullCommandLine);

    if (args != NULL && nCommandLine > 0) {
        sprintf_s(fullCommandLine, nFullCommandLine, "%s %s %s", exe_name, args, commandLine);
    } else if (args == NULL && nCommandLine > 0) {
        sprintf_s(fullCommandLine, nFullCommandLine, "%s %s", exe_name, commandLine);
    } else if (args != NULL) {
        sprintf_s(fullCommandLine, nFullCommandLine, "%s %s", exe_name, args);
    } else {
        sprintf_s(fullCommandLine, nFullCommandLine, "%s", exe_name);
    }

    char* extra_injections = MiceConfig.launcher.inject;
    if (!start_and_inject(hJob, exe_name, fullCommandLine, _miceIpcData->m_MiceDll, debug_wait,
                          boot_delay, extra_injections, 0, lpPi)) {
        free(fullCommandLine);
        return FALSE;
    }
    free(fullCommandLine);
    return TRUE;
}

static BOOL _GameIdSearcher(char* lpBasePath, char* lpGameId) {
    WIN32_FIND_DATAA findFileData;

    CHAR szSearchPath[MAX_PATH + 1];
    sprintf_s(szSearchPath, _countof(szSearchPath), "%s\\*", lpBasePath);

    // Search for a table in the CWD
    HANDLE hFind = FindFirstFileA(szSearchPath, &findFileData);
    do {
        if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0)
            continue;

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            sprintf_s(szSearchPath, _countof(szSearchPath), "%s\\%s", lpBasePath,
                      findFileData.cFileName);
            if (_GameIdSearcher(szSearchPath, lpGameId)) {
                FindClose(hFind);
                return TRUE;
            }
        } else {
            if (strlen(findFileData.cFileName) == strlen("SAAA_Table.dat") &&
                PathMatchSpecA(findFileData.cFileName, "S*_Table.dat")) {
                // maimai games have a copy of the R&D table; ignore it.
                if (strncmp(findFileData.cFileName, "SBRZ", 4) == 0) continue;
                lpGameId[0] = findFileData.cFileName[0];
                lpGameId[1] = findFileData.cFileName[1];
                lpGameId[2] = findFileData.cFileName[2];
                lpGameId[3] = findFileData.cFileName[3];

                FindClose(hFind);
                return TRUE;
            }
            if (strlen(findFileData.cFileName) == strlen("SAAA_banner.bmp") &&
                PathMatchSpecA(findFileData.cFileName, "S*_banner.bmp")) {
                lpGameId[0] = findFileData.cFileName[0];
                lpGameId[1] = findFileData.cFileName[1];
                lpGameId[2] = findFileData.cFileName[2];
                lpGameId[3] = findFileData.cFileName[3];

                FindClose(hFind);
                return TRUE;
            }
            if (strlen(findFileData.cFileName) == strlen("SAAA_main.wmv") &&
                PathMatchSpecA(findFileData.cFileName, "S*_main.wmv")) {
                lpGameId[0] = findFileData.cFileName[0];
                lpGameId[1] = findFileData.cFileName[1];
                lpGameId[2] = findFileData.cFileName[2];
                lpGameId[3] = findFileData.cFileName[3];

                FindClose(hFind);
                return TRUE;
            }
        }
    } while (FindNextFile(hFind, &findFileData));
    FindClose(hFind);
    return FALSE;
}

BOOL MiceGuessGameId(char* lpGameId) {
    if (_GameIdSearcher(".", lpGameId)) return TRUE;

    // TODO: Some more lookups. Exe name is probably not an awful fallback

    return FALSE;
}

BOOL g_bIsInDllMain = FALSE;
int main(int argc, char* argv[]) {
#if 0
    // char buffer[] = "41A800003F000000000000006801";
    char* buffer = argv[1];
    char local_10[4];

    sscanf_s(buffer, "%08x", local_10, sizeof local_10);
    float measure_whole = *(float*)(void*)&local_10;
    printf("measure_whole: %f\n", measure_whole);
    sscanf_s(buffer + 8, "%08x", local_10, sizeof local_10);
    float measure_fraction = *(float*)(void*)&local_10;
    printf("measure_fraction: %f\n", measure_fraction);
    sscanf_s(buffer + 16, "%08x", local_10, sizeof local_10);
    float duration = *(float*)(void*)&local_10;
    printf("duration: %f\n", duration);
    sscanf_s(buffer + 24, "%01x", local_10, sizeof local_10);
    unsigned char location = local_10[0];
    printf("location: %d\n", location);

    sscanf_s(buffer + 25, "%01x", local_10, sizeof local_10);
    unsigned char type = local_10[0];

    printf("Type: %d\n", local_10[0]);
    if (local_10[0] < '\0') {
        type = type ^ 0x80;
        puts("<0");
        // field_0x42 = 1;
    }
    if ((type & 0x40) != 0) {
        puts("&0x40");
        type = type ^ 0x40;
        // field15_0x18 = 0x40000000;
    }
    if ((type & 0x20) != 0) {
        puts("&0x20");
        type = type ^ 0x20;
        // field15_0x18 = 0x40800000;
    }
    printf("TypeO: %01x\n", type);

    return 0;
#endif

    load_mice_config();

    if (!MiceIpcSetup(TRUE)) return 1;
    _miceIpcData->m_LauncherIsReady = TRUE;
    char gameId[4];
    if (MiceGuessGameId(gameId)) memcpy(_miceIpcData->m_GameId, gameId, 4);

    CreateThread(NULL, 0, MiceIpcMessageWatcher, NULL, 0, NULL);

    setup_logging();

    log_info(plfBoot, "Micetools version: %s", MICE_VERSION);
    if (_miceIpcData->m_GameId[0] == '\0') {
        if (MiceConfig.devices.do_auto) {
            log_error(
                plfBoot,
                "Failed to identify game. Please edit [devices].do_auto to false and restart.");
            log_error(plfBoot, "Please also report this.");
            return terminate(-1, "Failed to identify game.");
        }
        log_warning(plfBoot, "Failed to identify game, but proceeding regardless.");
    } else {
        log_info(plfBoot, "Booting game: %.*s", 4, _miceIpcData->m_GameId);
    }

    CHAR szPath[MAX_PATH + 1];
    GetCurrentDirectory(MAX_PATH, szPath);
    szPath[sizeof szPath - 1] = '\0';
    log_info(plfBoot, "Current directory: %s", szPath);

    if (!parse_cmdline(argc, argv)) {
        puts("parse_cmdline failed");
        return terminate(-2, "parse_cmdline failed");
    }

    if (exe_name[0] == '\0' && MiceConfig.launcher.game_binary[0] != '\0') {
        snprintf(exe_name, sizeof exe_name, "%s", MiceConfig.launcher.game_binary);
    }
    if (MiceConfig.launcher.wait_for_debugger) debug_wait = true;
    boot_delay = MiceConfig.launcher.startup_delay;

    if (exe_name[0] == '\0') {
        if (!locate_game(exe_name, MAX_PATH + 1)) {
            log_error(plfBoot, "Fatal: Failed to locate a game");
            return terminate(-3, "Failed to locate a game");
        }
    } else {
        DWORD dwAttrib = GetFileAttributes(exe_name);
        if (dwAttrib == INVALID_FILE_ATTRIBUTES || dwAttrib & FILE_ATTRIBUTE_DIRECTORY) {
            log_error(plfBoot, "Fatal: %s: no such file found", exe_name);
            return terminate(-4, "Specified executable not locatable");
        }
    }

    int i;
    // Build IPC.m_PathPrefix
    {
        _miceIpcData->m_PathPrefix[0] = '\0';
        for (i = 0; i < sizeof _miceIpcData->m_PathPrefix; i++) {
            _miceIpcData->m_PathPrefix[i] = exe_name[i];
            if (exe_name[i] == '\0') break;
        }
        // Find the final slash in the path
        for (; i > 0; i--) {
            if (_miceIpcData->m_PathPrefix[i] == '/' || _miceIpcData->m_PathPrefix[i] == '\\')
                break;
        }
        _miceIpcData->m_PathPrefix[i] = '\0';
    }

    strcpy_s(_miceIpcData->m_MiceBase, sizeof _miceIpcData->m_MiceBase, szPath);

    if (!SearchPathA(NULL, MiceConfig.launcher.mice_dll, NULL, sizeof _miceIpcData->m_MiceDll,
                     _miceIpcData->m_MiceDll, NULL)) {
        log_error(plfBoot, "Unable to locate micelib. Check your mice_dll setting!");
        return terminate(-5, "Unable to locate micelib");
    }
    _miceIpcData->m_MiceDll[sizeof _miceIpcData->m_MiceDll - 1] = '\0';

    _MiceLauncherNextEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    SetConsoleCtrlHandler(MiceHandlerRoutine, TRUE);

    spawn_pcp_processes();

    log_info(plfBoot, "exec: %s %s", exe_name, commandLine);

    while (1) {
        if (hJob != INVALID_HANDLE_VALUE) {
            CloseHandle(hJob);
            hJob = INVALID_HANDLE_VALUE;
        }

        PROCESS_INFORMATION pi;
        if (_MiceLauncherNext == 2) {
            if (!StartGame(&pi, NULL))
                return terminate(-6, "Start failed. Check console for errors.");
        } else if (_MiceLauncherNext == 3 || _MiceLauncherNext == 4) {
            // TODO: Boot mode 3 is actually system test. We could do something here?
            if (!StartGame(&pi, "gametest"))
                return terminate(-7, "Gametest failed. Check console for errors.");
        } else {
            log_info(plfBoot, "Boot mode %d not supported. Exiting.", _MiceLauncherNext);
            return terminate(-8, NULL);
        }

        HANDLE objects[2];
        objects[0] = pi.hProcess;
        objects[1] = _MiceLauncherNextEvent;
        DWORD waited = WaitForMultipleObjects(_countof(objects), objects, FALSE, INFINITE);
        if (FAILED(waited)) {
            log_error(plfBoot, "Fatal: WaitForSingleObject failed: %03x", GetLastError());
            return terminate(-9, "WFSO failed");
        }
        if (waited == WAIT_OBJECT_0) {
            DWORD exitCode;
            if (GetExitCodeProcess(pi.hProcess, &exitCode))
                log_info(plfBoot, "Shutting down (ret:%d)", exitCode);
            else
                log_info(plfBoot, "Shutting down (failed to get exit code)");
            CloseHandle(pi.hProcess);
            continue;
            // return terminate(-10);
        } else if (waited == WAIT_OBJECT_0 + 1) {
            continue;
        } else {
            log_error(plfBoot, "Unknown wait object retured: %d", waited);
            return terminate(-11, "Unknown wait object!");
        }
    }
}
