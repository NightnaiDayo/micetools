#include <Windows.h>
#include <stdio.h>

#include "../lib/am/amPlatform.h"

static const char* sBoardType[] = {
    "RingEdge", "RingWide (Spec 1)", "RingWide (Spec 2)", "RingEdge 2", "Unknown",
};

void miceDumpPlatform(void) {
    fprintf(stderr, "Dumping platform information using %s\n", amPlatformVersion);

    AM_PLATFORM_BOARD_TYPE boardType;
    amPlatformGetBoardType(&boardType);
    AM_PLATFORM_PLATFORM_ID platformId;
    amPlatformGetPlatformId(&platformId);

    printf("Board type: %s\n", sBoardType[boardType]);
    printf("Platform ID: %s\n", platformId.strPlatformId);
}