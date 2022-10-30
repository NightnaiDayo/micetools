#include <Windows.h>
#include <shellapi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ewfapi.h"

typedef enum {
    MxError_Unknown = -1,
    MxError_None = 0,

    MxError_Ewf = 501,
    MxError_Version = 502,
    MxError_NoKeyFile = 503,
    MxError_DiskAccessPrivilege = 504,
    MxError_SystemMount = 505,

    MxError_MxMaster = 507,

    MxError_SBR = 511,
    MxError_SysKeyDelete = 512,
} MxError_t;
typedef enum {
    Init = 0,
    CheckEwf = 1,
    EwfDisabled = 2,
    GetSBR = 3,
    NormalBoot = 4,
    SystemUpdate2 = 5,
    SystemUpdate3 = 6,
    Done = 7,
} StartupStep_t;

#define VERSION_BUFFER 256

#define VERSION_FILE "C:\\System\\SystemVersion.txt"
#define DEFAULT_VERSION_FILE "C:\\System\\DefaultSystemVersion.txt"

#define SYSTEM_KEY_FILE "Z:\\SystemKeyFile"
#define OS_UPDATE_KEY_FILE "Z:\\UpdateKeyFile"
#define SYSTEM_VOLUME "C:\\System\\Execute\\System"
#define SYSTEM_MOUNT "S:"

#define NEXT_HOP_CWD SYSTEM_MOUNT "\\"
#define NEXT_HOP_MXMASTER NEXT_HOP_CWD "mxmaster.exe"

#define CACLS_REMPERM_EXECUTE "cacls c:\\system\\execute /t /e /p AppUser:N"
#define CACLS_REMPERM_Z "cacls z:\\ /t /e /p appuser:N"
#define CACLS_REMPERM_S "cacls s:\\ /t /e /p appuser:N"

// Rather than implement these sums in code, we're just going to use the
// presummed versions, because we have nothing to gain by obfuscating this.
// These two sum to TC_PASSWORD
#define TC_PASSWORD_1 "3d550a335d2557055d2b3d1444153348"
#define TC_PASSWORD_2 "36105d2e0b3c1b5f1336365f335a3f1c"
#define TC_PASSWORD "segahardpassword"
// These two sum to SYSTEM_KEYFILE_ADS
#define SYSTEM_KEYFILE_1 "041443255b204a3f054d1b4c352a22292f211b29440e1237591f2e49375d093967614d"
#define SYSTEM_KEYFILE_2 "3f26192e1e532a26680f2a2c3039534b363b2923082c41421a5537241408700d020b18"
#define SYSTEM_KEYFILE_ADS "C:\\System\\Execute\\DLL:SystemKeyFile"
// These two sum to OS_UPDATE_ADS
#define OS_UPDATE_KEYFILE_1 "4019582c56283006300b40042110143d603b3d3b1d012b5c0d5423152235163f624a43"
#define OS_UPDATE_KEYFILE_2 "03210427234b445f3d51057444536137052107112f392a14570d515029306307072222"
#define OS_UPDATE_KEYFILE_ADS "C:\\System\\Execute\\DLL:UpdateKeyFile"

int LOG_EN_PLATFORM = 0;

#define MX_LOG(...) log(__FUNCTION__, __LINE__, __VA_ARGS__)

void log(char* function, unsigned int line_number, char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("%s: Line%d  ", function, line_number);
    vprintf(fmt, args);
    va_end(args);
}

// TODO: These two
bool InitGraphics() { return true; }
bool PopulateEventInfo() { return true; }

// TODO: These three
void AppendAsterisk(void){};
void setStrForStep2(void){};
void setStrForStep3(void){};

int osuExecProcess(char* command) {
    PROCESS_INFORMATION processInformation = { 0 };
    STARTUPINFOA startupInfo = {
        .cb = 68,
        .dwFlags = STARTF_USESHOWWINDOW,
        .wShowWindow = SW_HIDE,
    };

    if (command == NULL) return -150;

    BOOL spanwed = CreateProcessA(NULL, command, NULL, NULL, 0, 0, NULL, NULL, &startupInfo, &processInformation);
    DWORD wait = WaitForSingleObject(processInformation.hProcess, INFINITE);

    int ret;
    if (!spanwed) {
        ret = -140;
    } else if (wait == WAIT_OBJECT_0) {
        DWORD exitCode;
        GetExitCodeProcess(processInformation.hProcess, &exitCode);
        ret = exitCode == 0 ? 0 : -141;
    } else {
        ret = -141;
    }

    CloseHandle(processInformation.hProcess);
    CloseHandle(processInformation.hThread);

    return ret;
}

/* Read a version from a text file */
bool GetVersionText(char* version, char* path) {
    char buffer[VERSION_BUFFER];
    FILE* file;

    memset(buffer, 0, sizeof buffer);
    if (fopen_s(&file, path, "r") || file == NULL) {
        MX_LOG("Error : GetVersionText error path = %s\n", path);
        return false;
    }

    fgets(buffer, sizeof buffer, file);
    fclose(file);
    strncpy_s(version, VERSION_BUFFER, buffer, VERSION_BUFFER);
    return true;
}
/* Get the currently running version of the system */
bool GetCurrentVersion(char* version) { return GetVersionText(version, VERSION_FILE); }
/* Get the default version of the system */
bool GetDefaultVersion(char* version) { return GetVersionText(version, DEFAULT_VERSION_FILE); }

/* Mount the system volume */
bool MountSystemVolume(void) {
    char command[256];
    snprintf(command, sizeof command, "TrueCrypt /p %s /k %s /v %s /l %s /w /s /q", TC_PASSWORD, SYSTEM_KEY_FILE,
             SYSTEM_VOLUME, SYSTEM_MOUNT);

    int ret = osuExecProcess(command);
    if (ret == 0) return true;
    MX_LOG("Error : osuExecProcess error. ret = %d\n", ret);
    return false;
};
/* Try to mount the system volume with up to 100 retries */
bool TryMountSystemVolume(void) {
    for (int i = 0; i < 100; i++) {
        if (MountSystemVolume()) return true;
        Sleep(200);
    }
    return false;
}

/* Copies the file at src to dst */
bool GetKeyFile(LPCSTR src, LPCSTR dst) {
    HANDLE file_1 = CreateFileA(src, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file_1 == INVALID_HANDLE_VALUE) {
        MX_LOG("CreaterFile Src error\n");
        return false;
    }

    HANDLE file_2 = CreateFileA(dst, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_2 == INVALID_HANDLE_VALUE) {
        CloseHandle(file_1);
        MX_LOG("Error : CreateFile Dst error\n");
        return false;
    }

    CHAR readBuffer[260];
    DWORD bytesRead;

    if (!ReadFile(file_1, readBuffer, 255, &bytesRead, NULL)) {
        CloseHandle(file_1);
        CloseHandle(file_2);
        MX_LOG("Error : ReadFile error\n");
        return false;
    }
    readBuffer[bytesRead] = '\0';

    DWORD written;
    bool ret = WriteFile(file_2, readBuffer, strlen(readBuffer), &written, NULL) != 0;
    if (!ret) MX_LOG("Error : WriteFile error\n");
    CloseHandle(file_1);
    CloseHandle(file_2);
    return ret;
}

bool GetSystemKeyFile() { return GetKeyFile(SYSTEM_KEYFILE_ADS, SYSTEM_KEY_FILE); };
bool GetOOUpdateKeyFile() { return GetKeyFile(OS_UPDATE_KEYFILE_ADS, OS_UPDATE_KEY_FILE); };
bool SetDiskAccessPrivilege() {
    int i;
    for (i = 0; i < 100; i++) {
        if (osuExecProcess(CACLS_REMPERM_EXECUTE) != -140) break;
        Sleep(200);
    }
    if (i == 100) {
        MX_LOG("Error : osuExecProcess error\n");
        return false;
    }

    for (i = 0; i < 100; i++) {
        if (osuExecProcess(CACLS_REMPERM_Z) != -140) break;
        Sleep(200);
    }
    if (i == 100) {
        MX_LOG("Error : osuExecProcess error\n");
        return false;
    }

    for (i = 0; i < 100; i++) {
        if (osuExecProcess(CACLS_REMPERM_S) != -140) break;
        Sleep(200);
    }
    if (i == 100) {
        MX_LOG("Error : osuExecProcess error\n");
        return false;
    }

    return true;
};
bool ExecuteMxMaster() {
    HINSTANCE ret = ShellExecuteA(NULL, NULL, NEXT_HOP_MXMASTER, NULL, NEXT_HOP_CWD, 0);
    if ((int)ret < 33) {
        MX_LOG("Error : ShellExecute ret = %d\n", ret);
        return false;
    }
    return true;
};
bool MxInitStartup(int* diskNumber) { return false; }
bool CheckEwfState(EWF_STATE* state) {
    // TODO: Consider a better implementation of this!

    *state = EWF_ENABLED;  // Nothing to do, boot the system
    // *state = EWF_DISABLED;  // We're in the middle of an update, so finish

    return true;
}
bool GetSBRSlotOSState(int diskNumber, int* updateState) { return false; };
bool SystemUpdateStep2(int diskNumber) { return false; }
bool SystemUpdateStep3(int diskNumber) { return false; }

MxError_t NormalBootStep(void) {
    AppendAsterisk();
    if (!GetSystemKeyFile()) return MxError_NoKeyFile;
    AppendAsterisk();
    if (!TryMountSystemVolume()) return MxError_SystemMount;
    AppendAsterisk();
    if (!DeleteFileA(SYSTEM_KEY_FILE)) return MxError_SysKeyDelete;
    AppendAsterisk();
    if (!SetDiskAccessPrivilege()) return MxError_DiskAccessPrivilege;
    AppendAsterisk();
    if (!ExecuteMxMaster()) return MxError_MxMaster;
    return MxError_None;
}

int DoStartup() {
    StartupStep_t step = Init;
    MxError_t error = MxError_None;
    int diskNumber = 0;

    while (step != Done) {
        printf("Step: %d (%d)\n", step, error);

        switch (step) {
            case Init:
                if (!MxInitStartup(&diskNumber))
                    step = CheckEwf;
                else
                    step = Done;
                break;
            case CheckEwf: {
                AppendAsterisk();
                EWF_STATE state;
                if (!CheckEwfState(&state)) {
                    error = MxError_Ewf;
                    step = Done;
                    break;
                }

                if (state == EWF_ENABLED)
                    step = NormalBoot;
                else
                    step = EwfDisabled;

                break;
            }
            case EwfDisabled: {
                char versionString[VERSION_BUFFER];
                char defaultVersionString[VERSION_BUFFER];

                if (!GetCurrentVersion(versionString) || !GetDefaultVersion(defaultVersionString)) {
                    error = MxError_Version;
                    step = Done;
                    break;
                }

                if (strcmp(versionString, defaultVersionString) == 0)
                    step = GetSBR;
                else
                    step = SystemUpdate3;
                break;
            }
            case GetSBR: {
                int updateState;
                AppendAsterisk();
                if (!GetSBRSlotOSState(diskNumber, &updateState)) {
                    error = MxError_SBR;
                    step = Done;
                    break;
                }

                if (updateState == 2)
                    step = SystemUpdate2;
                else
                    step = SystemUpdate3;
                break;
            }
            case NormalBoot:
                error = NormalBootStep();
                step = Done;
                break;
            case SystemUpdate2:
                setStrForStep2();
                error = SystemUpdateStep2(diskNumber);
                step = Done;
                break;
            case SystemUpdate3:
                setStrForStep3();
                error = SystemUpdateStep3(diskNumber);
                step = Done;
                break;
            default:
                error = MxError_Unknown;
                step = Done;
                break;
        }
    }
    return error;
}

int setEventSource(char** source) {
    // if (WIN_EVENT->ready == 0) {
    //     return -3;
    // }
    // if (source == (char**)0x0) {
    //     return -2;
    // }
    // _memcpy_s(&WIN_EVENT->eventSource, 16, source, 16);
    return 0;
}
void setLogFn(void* log_fn) {
    // LOG_FUN = log_fn;
    return;
}

int __cdecl logToEventlog(LPCSTR msg) {
    //
    return 0;
}

bool ExitGraphics(void) {
    //   int iVar1;
    //   iVar1 = FUN_0040b9e0((int)&hInstance);
    //   return iVar1 != 0;
    return true;
}

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    PVOID old = NULL;
    Wow64DisableWow64FsRedirection(&old);

    if (!InitGraphics(hInstance, nShowCmd)) {
        MX_LOG("Error : InitGraphics failed.\n");
    }

    if (!PopulateEventInfo()) {
        LOG_EN_PLATFORM = 1;
        char* mxstartup = "mxstartup";
        setEventSource(&mxstartup);
        setLogFn(logToEventlog);
    }

    MxError_t err = DoStartup();
    if (!err) {
        if (!ExitGraphics()) {
            MX_LOG("Error : ExitGraphics failed.\n");
        }
        // FUN_00408810();#

        Sleep(1000000);
        return 0;
    }

    printf("Fatal: %04d\n", err);

    // while (1) {
    //  FUN_00424640();
    //    Sleep(100000);
    //}

    return 0;
}

int main() { return WinMain(NULL, NULL, NULL, 0); }
