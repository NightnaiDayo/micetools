#include "appLauncher.h"

#include <io.h>
#include <memory.h>
#include <string.h>

#include "config.h"

void appLauncherDefaultRegSet() {
    // TODO: This
}

char GAME_EXTENSIONS[3][4] = {
    { 'c', 'o', 'm', '\0' },
    { 'e', 'x', 'e', '\0' },
    { 'b', 'a', 't', '\0' },
};

DWORD __stdcall appLauncherAppThread(appLauncherAppInfo_t *appInfo) {
    if (appInfo == NULL || appInfo->m_appLauncher == NULL) {
        amiDebugLog("param error %p %p", appInfo, appInfo->m_appLauncher);
        return 0xffffffff;
    }

    PROCESS_INFORMATION processInformation;
    STARTUPINFOW startupInfoW;
    STARTUPINFOA startupInfoA;
    char commandLine[64];
    char findPath[64];
    WCHAR workingDirectory[128];
    WCHAR wCommandLine[130];

    intptr_t hFile;
    struct _finddata_t next_file;

    bool skipSpawn = false;
    bool appUser = false;

    switch (appInfo->m_mode) {
        case 1:
            sprintf_s(commandLine, sizeof commandLine, "%s%s", Config.dir.execute,
                      Config.binary.segaboot_r);
            break;
        case 3:
            sprintf_s(commandLine, sizeof commandLine, "%s%s", Config.dir.execute,
                      Config.binary.segaboot_tr);
            break;
        case 5:
            sprintf_s(commandLine, sizeof commandLine, "%s%s", Config.dir.execute,
                      Config.binary.segaboot_d);
            break;
        case 6:
            sprintf_s(commandLine, sizeof commandLine, "%s%s", Config.dir.execute,
                      Config.binary.segaboot);
            break;

        case 2:
            if (appInfo->m_path[0] != '\0') {
                sprintf_s(commandLine, sizeof commandLine, "%s", appInfo->m_path);
                hFile = _findfirst(commandLine, &next_file);
                if (hFile == -1) {
                    amiDebugLog("Error : Process does not exist");
                    skipSpawn = true;
                }
                break;
            }

            sprintf_s(findPath, sizeof findPath, "%s%s.*", "x:\\", "game");
            hFile = _findfirst(findPath, &next_file);
            if (hFile == -1) {
                amiDebugLog("Error : Process does not exist");
                skipSpawn = true;
                break;
            }

            do {
                for (int i = 0; i < 3; i++) {
                    sprintf_s(findPath, 0x40, "%s.%s", "game", GAME_EXTENSIONS[i]);

                    if (strcmp(findPath, next_file.name) == 0) {
                        sprintf_s(commandLine, 0x40, "%s%s", "x:\\", findPath);
                        _findclose(hFile);
                        appUser = true;
                        goto game_found;
                    }
                }
            } while (_findnext(hFile, &next_file) == 0);

            amiDebugLog("Error : Process does not exist");
            sprintf_s(commandLine, sizeof commandLine, "%s%s", Config.dir.execute,
                      Config.binary.segaboot_r);

        game_found:
            break;

        case 4:
            sprintf_s(findPath, 0x40, "%s%s.*", "x:\\", "gametest");
            hFile = _findfirst(findPath, &next_file);
            if (hFile == -1) {
                amiDebugLog("Error : Process does not exist");
                sprintf_s(commandLine, sizeof commandLine, "%s%s", Config.dir.execute,
                          Config.binary.segaboot_r);
                break;
            }

            do {
                for (int i = 0; i < 3; i++) {
                    sprintf_s(findPath, 0x40, "%s.%s", "gametest", GAME_EXTENSIONS[i]);

                    if (strcmp(findPath, next_file.name) == 0) {
                        sprintf_s(commandLine, 0x40, "%s%s", "x:\\", findPath);
                        _findclose(hFile);
                        appUser = true;
                        goto gametest_found;
                    }
                }
            } while (_findnext(hFile, &next_file) == 0);

            amiDebugLog("Error : Process does not exist");
            sprintf_s(commandLine, sizeof commandLine, "%s%s", Config.dir.execute,
                      Config.binary.segaboot_r);
            _findclose(hFile);
            appUser = true;

        gametest_found:
            break;

        default:
            amiDebugLog("Error : Error Process");
            sprintf_s(commandLine, sizeof commandLine, "%s%s", Config.dir.execute,
                      Config.binary.segaboot_r);
    }

    ZeroMemory(&startupInfoA, sizeof startupInfoA);
    startupInfoA.cb = sizeof startupInfoA;
    startupInfoA.dwFlags = 1;
    startupInfoA.wShowWindow = 1;
    ZeroMemory(&startupInfoW, sizeof startupInfoW);
    startupInfoW.cb = sizeof startupInfoW;
    startupInfoW.dwFlags = 1;
    startupInfoW.wShowWindow = 1;

    if (!skipSpawn) {
        BOOL success;
        if (appUser) {
            DWORD dwCreationFlags = 0;

            if (strstr(commandLine, "bat") != NULL) {
                dwCreationFlags = CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS;
                startupInfoW.wShowWindow = 0;
            }

            ZeroMemory(wCommandLine, sizeof wCommandLine);
            ZeroMemory(workingDirectory, sizeof workingDirectory);

            MultiByteToWideChar(0, 0, commandLine, strlen(commandLine), wCommandLine,
                                sizeof wCommandLine / sizeof wCommandLine[0]);
            MultiByteToWideChar(0, 0, Config.dir.game, 4, workingDirectory,
                                sizeof workingDirectory / sizeof workingDirectory[0]);

            wchar_t *username = appInfo->m_appLauncher->m_username;
            wchar_t *password = appInfo->m_appLauncher->m_password;

            if (wcslen(username) == 0)
                MultiByteToWideChar(0, 0, Config.env.username, strlen(Config.env.username),
                                    username, sizeof username / sizeof username[0]);
            if (wcslen(password) == 0)
                MultiByteToWideChar(0, 0, Config.env.password, strlen(Config.env.password),
                                    password, sizeof password / sizeof password[0]);

            amiDebugLog("Creating process: %ls", wCommandLine);
            success = CreateProcessWithLogonW(username, NULL, password, 0, NULL, wCommandLine,
                                              dwCreationFlags, NULL, workingDirectory,
                                              &startupInfoW, &processInformation);
        } else {
            amiDebugLog("Creating process: %s", commandLine);
            success = CreateProcessA(NULL, commandLine, NULL, NULL, FALSE,
                                     CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS, NULL, NULL,
                                     &startupInfoA, &processInformation);
        }

        if (!success) {
            CloseHandle(processInformation.hProcess);
            CloseHandle(processInformation.hThread);
            amiDebugLog("ERROR : Close Process %s", commandLine);
        } else {
            if (WaitForSingleObject(processInformation.hProcess, INFINITE) == WAIT_FAILED) {
                amiDebugLog("WaitForSingleObject Error:%d", GetLastError());
            }
            CloseHandle(processInformation.hProcess);
            CloseHandle(processInformation.hThread);
        }
    }

    pcpa_t *stream = &(appInfo->m_appLauncher->m_pcp);
    e_pcpa_t err;
    if ((err = pcpaInitStream(stream)) != e_pcpa_ok) {
        amiDebugLog("Error pcpaInitStream. Code %d", err);
        return 0xffffffff;
    }
    do {
        err = pcpaOpenClient(stream, "127.0.0.1", 40100, 60000, TIMEOUT_NONE);
        if (err != e_pcpa_ok) pcpaClose(stream);
    } while (err != e_pcpa_ok);

    if (pcpaSetSendPacket(stream, "mxmaster.foreground.active", "0") == NULL) {
        pcpaClose(stream);
        amiDebugLog("Error pcpaSetSendPacket return NULL");
        return 0xffffffff;
    }

    if ((err = pcpaAccessClient(stream, TIMEOUT_NONE)) != e_pcpa_ok) {
        amiDebugLog("Error : Not Access Master Thread. Code %d", err);
        pcpaClose(stream);
        return 0xffffffff;
    }
    pcpaClose(stream);
    return 0;
}

int appLauncherCreateThread(appLauncherAppInfo_t *appInfo, appLauncher_t *appLauncher) {
    if (appLauncher->m_createdThread != 0) {
        amiDebugLog("Error : already created thread");
        return 3;
    }

    unsigned int mode = appInfo->m_mode;
    appLauncher->m_appInfo.m_mode = mode;
    strcpy_s(appLauncher->m_appInfo.m_path, sizeof appLauncher->m_appInfo.m_path, appInfo->m_path);

    if (mode == 1 || mode == 3 || mode == 5 || mode == 6) appLauncherDefaultRegSet();

    HANDLE hThread = CreateThread(NULL, 0, &appLauncherAppThread, &appLauncher->m_appInfo, 0,
                                       &appLauncher->m_threadAddr);

    appLauncher->m_hThread = hThread;
    if (hThread == 0) {
        amiDebugLog("Error : create thread errror");
        return 2;
    }
    appLauncher->m_createdThread = 1;
    return 0;
}