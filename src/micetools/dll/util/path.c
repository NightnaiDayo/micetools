#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../hooks/files.h"

BOOL PathEqual(LPCSTR path1, LPCSTR path2) {
    char buffer1[MAX_PATH];
    char buffer2[MAX_PATH];

    GetFullPathNameA(path1, _countof(buffer1), buffer1, NULL);
    GetFullPathNameA(path2, _countof(buffer2), buffer2, NULL);

    return strcmp(path1, path2) == 0;
}
BOOL PathPrefix(LPCSTR path, LPCSTR prefix) {
    char buffer1[MAX_PATH];
    char buffer2[MAX_PATH];

    GetFullPathNameA(path, _countof(buffer1), buffer1, NULL);
    GetFullPathNameA(prefix, _countof(buffer2), buffer2, NULL);

    return strstr(buffer1, buffer2) == buffer1;
}

static char WORKING_DIR[MAX_PATH + 1] = { 0 };
char GetGamedataDrive(void) {
    if (WORKING_DIR[0] == 0x00) _GetCurrentDirectoryA(sizeof WORKING_DIR, WORKING_DIR);
    return WORKING_DIR[0];
}
BOOL IsGamedataLocalPath(LPCSTR path) {
    if (WORKING_DIR[0] == 0x00) _GetCurrentDirectoryA(sizeof WORKING_DIR, WORKING_DIR);
    return PathPrefix(path, WORKING_DIR);
}
