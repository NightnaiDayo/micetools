/*
Currently unimplemented:
- Changing the screen resoltuion to something very small (lol)
- The UI that shows "NOW LOADING"
*/
#include <Windows.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SYSTEM_USER "SystemUser"
#define MXSTARTUP "C:\\System\\Execute\\mxstartup.exe"

#define RETRIES 100
#define STR_INDIR(x) #x
#define STR(x) STR_INDIR(x)

/*

No debugger:

  153815264b5839090b0d1c1a423c02241633130673071a1e38443912410b47380f213c1d
+ 2e0247311e162b666c6640393737724157001f56045b4b4f24333457335a26381f4c3349
= 'C:\Windows\System32\wbem\wmitemp.mof'

  270a2a053b29042b261d1b22070d140c  (wmitemp.mof)
+ 152c05381a141f494a48060223260d29  (key)
= '<6/=U=#tpe!$*3!5'



Has debugger:

  d0b1c034a32243340505a343659517c121f0319583d205c593a690719604846000e062a6
+ 63f10541f0c201c0703022f332a13094b542f130c25491a1c280a65440831296e2563590
% 255 (not 256!)
= 34a3c57594e444f47535c53797374756d63323c546279667562737c5d68796f6e2379737
flip nibbles of each byte
= 'C:\\WINDOWS\\system32\\drivers\\mxio.sys'

  9160e5c22392918371e43573f2b095b1  (mxio.sys)
+ 43368004f2a34211f4f12120b1b57151
% 255
= d49666c61636d39466d65693a4660703
flip nibbles of each byte
= 'Miflac=Ifme9Jfp0'

*/

#define PERFECT_RECREATION

#ifdef PERFECT_RECREATION
// C:\Windows\System32\wbem\wmitemp.mof (contains 270a2a053b29042b261d1b22070d140c)
// NOTE: On 64 bit windows this is going to be redirected to syswow64
#define PATH_NODEBUG_1 "153815264b5839090b0d1c1a423c02241633130673071a1e38443912410b47380f213c1d"
#define PATH_NODEBUG_2 "2e0247311e162b666c6640393737724157001f56045b4b4f24333457335a26381f4c3349"

// C:\WINDOWS\system32\drivers\mxio.sys (contains 9160e5c22392918371e43573f2b095b1)
// NOTE: On 64 bit windows this is going to be redirected to syswow64
#define PATH_DEBUG_1 "d0b1c034a32243340505a343659517c121f0319583d205c593a690719604846000e062a6"
#define PATH_DEBUG_2 "63f10541f0c201c0703022f332a13094b542f130c25491a1c280a65440831296e2563590"

#define NODEBUG_PSK "152c05381a141f494a48060223260d29"
#define DEBUG_PSK "43368004f2a34211f4f12120b1b57151"
#else
#define PASS_NODEBUG "Miflac=Ifme9Jfp0"
#define PASS_DEBUG "<6/=U=#tpe!$*3!5"
#endif

void reboot() {
    puts("Starting the next hop failed.");
    puts("We're not going to reboot, but mxprestartup normally would at this point.");
    exit(1);
}

char* load_password(const char* path1, const char* path2, const char* psk, bool flip) {
    char path[37];
    path[36] = '\0';
    char temp1[3] = { 0 };
    char temp2[3] = { 0 };
    char* pEnd;
    for (int i = 0; i < 72; i += 2) {
        temp1[0] = path1[i];
        temp1[1] = path1[i + 1];
        temp2[0] = path2[i];
        temp2[1] = path2[i + 1];
        uint8_t value = (strtol(temp1, &pEnd, 16) + strtol(temp2, &pEnd, 16)) % 0xff;
        if (flip)
            path[i >> 1] = ((value & 0xf0) >> 4) | ((value & 0x0f) << 4);
        else
            path[i >> 1] = value;
    }

    printf("Attempting to read key from %s\n", path);

    FILE* key_file;
    fopen_s(&key_file, path, "r");
    if (!key_file) {
        printf("! Failed to read file: %d\n", GetLastError());
        return NULL;
    }
    char key2[32];
    fread(key2, 1, 32, key_file);
    fclose(key_file);

    char* password = malloc(17);
    password[16] = '\0';

    for (int i = 0; i < 32; i += 2) {
        temp1[0] = psk[i];
        temp1[1] = psk[i + 1];
        temp2[0] = key2[i];
        temp2[1] = key2[i + 1];
        uint8_t value = (strtol(temp1, &pEnd, 16) + strtol(temp2, &pEnd, 16)) % 0xff;
        if (flip)
            password[i >> 1] = ((value & 0xf0) >> 4) | ((value & 0x0f) << 4);
        else
            password[i >> 1] = value;
    }

    return password;
}

char* get_password() {
    if (IsDebuggerPresent()) {
#ifdef PERFECT_RECREATION
        return load_password(PATH_DEBUG_1, PATH_DEBUG_2, DEBUG_PSK, true);
#else
        return PASS_DEBUG;
#endif
    }

#ifdef PERFECT_RECREATION
    return load_password(PATH_NODEBUG_1, PATH_NODEBUG_2, NODEBUG_PSK, false);
#else
    return PASS_NODEBUG;
#endif
}

int main(int argc, char** argv) {
    if (argc > 4) {
        puts("Usage:");
        printf("  %s\n", argv[0]);
        printf("  %s <mxstartup.exe>\n", argv[0]);
        printf("  %s <username> <password>\n", argv[0]);
        printf("  %s <mxstartup.exe> <username> <password>\n", argv[0]);
        exit(0);
    }

    PVOID old = NULL;
    Wow64DisableWow64FsRedirection(&old);

    BOOL spawned = FALSE;

    STARTUPINFOW startup_info = {
        .cb = 68,
        .wShowWindow = SW_SHOW,
        .dwFlags = STARTF_USESHOWWINDOW,
    };
    PROCESS_INFORMATION process = { 0 };

    for (int i = 0; i < RETRIES; i++) {
        char* password = argc == 3 ? argv[2] : argc == 4 ? argv[3] : get_password();

        if (password) {
            char* username = argc == 3 ? argv[1] : argc == 4 ? argv[2] : SYSTEM_USER;
            char* binary = (argc == 2 || argc == 4) ? argv[1] : MXSTARTUP;

            printf("Username: %s\n", username);
            printf("Password: %s\n", password);
            printf("Next hop: %s\n", binary);

            size_t _;
            wchar_t* wPassword = malloc((strlen(password) + 1) * (sizeof(wchar_t)));
            wchar_t* wUser = malloc((strlen(username) + 1) * (sizeof(wchar_t)));
            wchar_t* wBinary = malloc((strlen(binary) + 1) * (sizeof(wchar_t)));

            mbstowcs_s(&_, wPassword, strlen(password) + 1, password, _TRUNCATE);
            mbstowcs_s(&_, wUser, strlen(username) + 1, username, _TRUNCATE);
            mbstowcs_s(&_, wBinary, strlen(binary) + 1, binary, _TRUNCATE);

            spawned = CreateProcessWithLogonW(wUser, NULL, wPassword, 0, NULL, wBinary, HIGH_PRIORITY_CLASS, NULL, NULL,
                                              &startup_info, &process);
            free(wPassword);

            if (spawned)
                break;
            else
                puts("Failed to start next hop");

            if (process.hProcess != NULL) {
                CloseHandle(process.hProcess);
                process.hProcess = NULL;
            }
        } else {
            puts("Failed to get password");
        }

        Sleep(1000);
        puts("------------------------");
        printf("Retry %d\n", i + 1);
        puts("------------------------");
    }

    if (process.hProcess != NULL) {
        CloseHandle(process.hProcess);
        process.hProcess = NULL;
    }

    if (!spawned) {
        reboot();
    }

    return 0;
}
