#include <stdio.h>

#include "../lib/mice/mice.h"

// TODO: Ewww
BOOL(WINAPI* TruePathFileExistsA)(LPCSTR pszPath) = PathFileExistsA;

int main() {
    MiceFSInit();
    // MiceFSAddDevLayers();
    // MiceFSAddRingedgeLayers(FALSE);

    CHAR path[MAX_PATH + 1];
    strcpy_s(path, sizeof path, "X:\\Systemconfig.txt");

    MICE_FS_PATH_TOK pathTok;
    LPSTR pathComponent = MiceFSPathTokA(path, _countof(path), &pathTok);

    while (pathComponent) {
        printf("Component : '%s'\n", pathComponent);
        pathComponent = MiceFSPathTokNextA(&pathTok);
    }


    // CHAR tail[MAX_PATH + 1];
    // printf("Test 1: %d\n", MiceFSMatchPathA("C:\\windows\\system32\\etc\\help.exe", "C:\\WINDOWS", tail, sizeof(tail)));
    // printf("Tail: %s\n", tail);

    // LPSTR redirected = NULL;
    // MiceFSRedirectPathA("X:\\game.bat", &redirected);
    // printf("Redirected path: %s\n", redirected);
}
