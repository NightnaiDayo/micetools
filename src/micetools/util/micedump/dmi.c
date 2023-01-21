#include <Windows.h>
#include <stdio.h>

#include "../lib/am/amOemstring.h"

BOOL miceDumpDMI() {
    amOemstringDebugLevel = 1;

    fprintf(stderr, "Dumping DMI using %s\n", amOemstringVersion);

    char oemstring[32];
    int err = 0;

    err = amOemstringGetManufacturer(oemstring);
    if (err) {
        fprintf(stderr, "amOemstringGetManufacturer error: %d\n", err);
        return FALSE;
    }
    printf("Manufacturer: %s\n", oemstring);

    puts("OEM Strings:");
    for (int which = 0; which < 5; which++) {
        err = amOemstringGetOemstring(oemstring, which);
        if (err) {
            fprintf(stderr, "amOemstringGetOemstring error: %d\n", err);
            return FALSE;
        }
        printf(" %d: %s\n", which + 1, oemstring);
    }
    err = amOemstringGetSBiosVer(oemstring);
    if (err) {
        fprintf(stderr, "amOemstringGetSBiosVer error: %d\n", err);
        return FALSE;
    }
    printf("BIOS Version: %s\n", oemstring);

    err = amOemstringGetSBiosReleaseDate(oemstring);
    if (err) {
        fprintf(stderr, "amOemstringGetSBiosReleaseDate error: %d\n", err);
        return FALSE;
    }
    printf("BIOS Date: %s\n", oemstring);

    return TRUE;
}
