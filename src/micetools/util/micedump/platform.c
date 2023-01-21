#include <Windows.h>
#include <stdio.h>

#include "../lib/am/amPlatform.h"

void miceDumpPlatform(void) {
    fprintf(stderr, "Dumping platform information using %s\n", amPlatformVersion);

    AM_PLATFORM_BOARD_TYPE boardType;
    amPlatformGetBoardType(&boardType);
    AM_PLATFORM_PLATFORM_ID platformId;
    amPlatformGetPlatformId(&platformId);

    printf("Board type: %d\n", boardType);
    printf("Platform ID: %s\n", platformId.strPlatformId);
}