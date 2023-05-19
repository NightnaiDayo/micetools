#include <stdio.h>
#include <string.h>

#include "micedump.h"

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "dmi") == 0)
            miceDumpDMI();
        else if (strcmp(argv[i], "eeprom") == 0)
            miceDumpEEPROM();
        else if (strcmp(argv[i], "platform") == 0)
            miceDumpPlatform();
        else if (strcmp(argv[i], "sram") == 0)
            miceDumpSRAM();
        else if (strcmp(argv[i], "dongle") == 0)
            miceDumpKCMxkeychip();
        else if (strcmp(argv[i], "kcpic") == 0)
            miceDumpKCPIC();
        else if (strcmp(argv[i], "cmos") == 0)
            miceDumpCMOS();
        else
            printf("Unknown dump type: %s\n", argv[i]);
    }

    return 0;
}
