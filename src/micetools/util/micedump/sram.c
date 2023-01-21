#include <Windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>

#include "../lib/am/amSram.h"

#define DUMP_FILE "micedump-sram.bin"

BOOL miceDumpSRAM() {
    amSramDebugLevel = 1;

    fprintf(stderr, "Dumping SRAM using %s\n", amSramVersion);

    AM_SRAM_STATUS err = amSramInit();
    if (err != AM_SRAM_STATUS_OK) {
        printf("amSramInit failed: %d\n", err);
        return FALSE;
    }

    DISK_GEOMETRY geom;
    DWORD bytesReturned;
    BOOL s = DeviceIoControl(amSram.m_handle, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &geom,
                             sizeof geom, &bytesReturned, NULL);
    if (!s) {
        puts("IOCTL_DISK_GET_DRIVE_GEOMETRY failed");
        amSramExit();
        return FALSE;
    }
    puts("SRAM geometry:");
    printf(":: Cylinders: %lld\n", geom.Cylinders.QuadPart);
    printf(":: MediaType: %d\n", geom.MediaType);
    printf(":: TracksPerCylinder: %d\n", geom.TracksPerCylinder);
    printf(":: SectorsPerTrack: %d\n", geom.SectorsPerTrack);
    printf(":: BytesPerSector: %d\n", geom.BytesPerSector);

    puts("amSram computed values:");
    printf(":: amSram.m_size: %lld\n", amSram.m_size.QuadPart);
    printf(":: amSram.m_sectorSize: %d\n", amSram.m_sectorSize);

    DWORD toRead = amSram.m_size.QuadPart & 0xffffffff;

    LPBYTE dump_buffer = malloc(toRead);
    if (!dump_buffer) {
        printf("Failed to malloc %ld bytes for buffer!\n", toRead);
        return FALSE;
    }
    amSramRead(0, dump_buffer, toRead);

    FILE *sramFile;
    fopen_s(&sramFile, DUMP_FILE, "wb");
    fwrite(dump_buffer, 1, toRead, sramFile);
    fclose(sramFile);
    printf(" -> Written to %s\n", DUMP_FILE);

    amSramExit();

    return TRUE;
}
