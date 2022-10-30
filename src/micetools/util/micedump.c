#include <Windows.h>
#include <setupapi.h>
#include <shellapi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#pragma comment(lib, "Setupapi.lib")

#include "../lib/am/amEeprom.h"

#define OpenDriver(x) \
    CreateFileA("\\\\.\\" x, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL)

#define RULE "-----------------------------------------"
#define SECTION_HEAD(x) \
    puts("");           \
    puts(RULE);         \
    puts(x);            \
    puts(RULE);         \
    puts("");

typedef struct {
    uint64_t physAddr;
    DWORD dType;
    DWORD size;
} columba_request_t;

void scan_for_dmi(HANDLE columba, DWORD *stable_addr) {
    DWORD bytesOut;
    // short *stable_len;
    columba_request_t request;
    unsigned char readBuf[0x8010];

    uint64_t search_addr = 0xf0000;
    while (1) {
        request.physAddr = search_addr;
        request.dType = 1;
        request.size = sizeof readBuf;

        BOOL succ = DeviceIoControl(columba, (DWORD)IOCTL_COLUMBA_READ, &request,
                                    sizeof request, &readBuf, sizeof readBuf, &bytesOut, NULL);
        if (succ && bytesOut == sizeof readBuf) {
            for (unsigned int offset = 0; offset < 0x8008; offset++) {
                if (readBuf[offset] == '_' && readBuf[offset + 1] == 'D' &&
                    readBuf[offset + 2] == 'M' && readBuf[offset + 3] == 'I' &&
                    readBuf[offset + 4] == '_') {
                    *stable_addr = *(DWORD *)&readBuf[offset + 8];

                    return;
                }
            }
        }

        search_addr += 0x7ff0;
        if (search_addr > 0xfffdf) return;
    }
}

BOOL dump_columba() {
    SECTION_HEAD("columba");

    HANDLE columba = OpenDriver("columba");
    if (columba == INVALID_HANDLE_VALUE) return FALSE;

    DWORD stable_addr = 0;
    scan_for_dmi(columba, &stable_addr);
    if (stable_addr == 0) {
        CloseHandle(columba);
        return FALSE;
    }

    columba_request_t request;
    unsigned char readBuf[0x10000];

    printf("Found DMI at: %d\n", stable_addr);

    request.physAddr = stable_addr;
    request.dType = 1;
    if (stable_addr + 0x10000 < 0x100001) {
        request.size = 0x10000;
    } else {
        request.size = 0x100000 - stable_addr;
    }
    DWORD bytesReturned;

    BOOL s = DeviceIoControl(columba, (DWORD)IOCTL_COLUMBA_READ, &request, sizeof request,
                             readBuf, sizeof readBuf, &bytesReturned, NULL);
    if (!s || bytesReturned != sizeof readBuf) {
        CloseHandle(columba);
        return FALSE;
    }

    puts("DMI read sucessful.");
    FILE *dmi;
    fopen_s(&dmi, "dmi.bin", "wb");
    fwrite(readBuf, 1, sizeof readBuf, dmi);
    fclose(dmi);
    puts(" -> Written to dmi.bin");

    CloseHandle(columba);
    return TRUE;
}

BOOL dump_eeprom() {
    SECTION_HEAD("mxSMBus");

    HANDLE mxsmbus = amEepromCreateDeviceFile(&MXSMBUS_GUID, NULL, 0);
    if (mxsmbus == INVALID_HANDLE_VALUE) return FALSE;

    DWORD _dummy;
    DWORD version;
    DeviceIoControl(mxsmbus, (DWORD)IOCTL_MXSMBUS_GET_VERSION, NULL, 0, &version, sizeof version,
                    &_dummy, NULL);
    printf("mxSMBus version: %08x\n", version);

    BYTE data[0x20];
    for (WORD reg = 0; reg < 256; reg++) {
        if (!amEepromReadBlock(mxsmbus, reg & 0xFF, 0x20, data)) continue;
        printf("%02x: ", reg);
        for (int i = 0; i < 0x20; i++) printf("%02x ", data[i]);
        puts("");
    }

    CloseHandle(mxsmbus);
    return TRUE;
}

unsigned char sram_buf[1024 * 2048];
BOOL dump_sram() {
    SECTION_HEAD("mxSRAM");

    HANDLE mxsram = OpenDriver("mxsram");
    if (mxsram == INVALID_HANDLE_VALUE) return FALSE;

    DWORD _dummy;
    BOOL s;

    DWORD version;
    s = DeviceIoControl(mxsram, (DWORD)IOCTL_MXSRAM_PING, NULL, 0, &version, sizeof version,
                        &_dummy, NULL);
    if (!s) {
        CloseHandle(mxsram);
        return FALSE;
    }
    printf("mxSRAM version: %04x\n", version);

    DISK_GEOMETRY geom;
    s = DeviceIoControl(mxsram, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &geom, sizeof geom, &_dummy,
                        NULL);
    if (!s) {
        CloseHandle(mxsram);
        return FALSE;
    }
    puts("mxSRAM geometry:");
    printf(":: Cylinders: %lld\n", geom.Cylinders.QuadPart);
    printf(":: MediaType: %d\n", geom.MediaType);
    printf(":: TracksPerCylinder: %d\n", geom.TracksPerCylinder);
    printf(":: SectorsPerTrack: %d\n", geom.SectorsPerTrack);
    printf(":: BytesPerSector: %d\n", geom.BytesPerSector);

    DWORD ssize;
    s = DeviceIoControl(mxsram, (DWORD)IOCTL_MXSRAM_GET_SECTOR_SIZE, NULL, 0, &ssize, sizeof ssize,
                        &_dummy, NULL);
    if (!s) {
        CloseHandle(mxsram);
        return FALSE;
    }
    printf(":: Sector Size: %d\n", ssize);

    DWORD read;
    if (!ReadFile(mxsram, sram_buf, sizeof sram_buf, &read, NULL)) {
        CloseHandle(mxsram);
        return FALSE;
    }
    printf("Read %d bytes\n", read);
    if (read != sizeof sram_buf) puts("W: incomplete");

    FILE *sram;
    fopen_s(&sram, "sram.bin", "wb");
    fwrite(sram_buf, 1, sizeof sram_buf, sram);
    fclose(sram);
    puts(" -> Written to sram.bin");
    CloseHandle(mxsram);
    return TRUE;
}

BYTE superio_read(HANDLE mxsuperio, BYTE chip, BYTE device, BYTE index) {
    DWORD _dummy;
    BYTE payload[4] = { chip, device, index, 0 };
    DeviceIoControl(mxsuperio, (DWORD)IOCTL_MXSUPERIO_READ, payload, sizeof payload, payload,
                    sizeof payload, &_dummy, NULL);
    return payload[3];
}

#define SUPERIO_LD_FDC 0
#define SUPERIO_LD_PARALLEL 1
#define SUPERIO_LD_UART_A 2
#define SUPERIO_LD_UART_B 3
#define SUPERIO_LD_KEYBOARD 5
#define SUPERIO_LD_UART_C 6
#define SUPERIO_LD_GPIO34 7
#define SUPERIO_LD_WDTO_PLED_GPIO56 8
#define SUPERIO_LD_GPIO12_SUSLED 9
#define SUPERIO_LD_ACPI 10
#define SUPERIO_LD_HWMON 11
#define SUPERIO_LD_PECI_SST 12
#define SUPERIO_LD_UART_D 13
#define SUPERIO_LD_UART_E 14
#define SUPERIO_LD_UART_F 15

BOOL dump_superio() {
    SECTION_HEAD("mxSuperIO");

    HANDLE mxsuperio = OpenDriver("mxsuperio");
    if (mxsuperio == INVALID_HANDLE_VALUE) return FALSE;
    BOOL s;
    DWORD _dummy;

    DWORD version;
    s = DeviceIoControl(mxsuperio, (DWORD)IOCTL_MXSUPERIO_PING, NULL, 0, &version, sizeof version,
                        &_dummy, NULL);
    if (!s) {
        CloseHandle(mxsuperio);
        return FALSE;
    }
    printf("mxSuperIO version: %08x\n", version);

    BYTE ver_msb;
    ver_msb = superio_read(mxsuperio, 0, SUPERIO_LD_FDC, 0x20);
    if (ver_msb != 0xff) {
        puts(":: Chip 0 present");
        printf(" -> Version: %02x%02x\n", ver_msb,
               superio_read(mxsuperio, 0, SUPERIO_LD_FDC, 0x21));
    } else
        puts(":: Chip 0 unpopulated");
    ver_msb = superio_read(mxsuperio, 1, SUPERIO_LD_FDC, 0x20);
    if (ver_msb != 0xff) {
        puts(":: Chip 1 present");
        printf(" -> Version: %02x%02x\n", ver_msb,
               superio_read(mxsuperio, 1, SUPERIO_LD_FDC, 0x21));
    } else
        puts(":: Chip 1 unpopulated");

    puts(":: Super lazy dump of chip 1, bank 0:");
    for (uint8_t reg = 0; reg < 0xff; reg++) {
        unsigned char packet[3] = { 1, reg, 0 };
        DeviceIoControl(mxsuperio, (DWORD)IOCTL_MXSUPERIO_HWMONITOR_LPC_READ, &packet,
                        sizeof packet, &packet, sizeof packet, &_dummy, NULL);
        printf(" -> %02x: %02x\n", reg, packet[2]);
    }

    CloseHandle(mxsuperio);
    return TRUE;
}

int main() {
    // if (!dump_columba()) {
    //     printf("Failed to dump DMI: %03x\n", GetLastError());
    // }
    if (!dump_eeprom()) {
        printf("Failed to dump EEPROM: %03x\n", GetLastError());
    }
    // if (!dump_sram()) {
    //     printf("Failed to dump SRAM: %03x\n", GetLastError());
    // }
    // if (!dump_superio()) {
    //     printf("Failed to dump SuperIO: %03x\n", GetLastError());
    // }
    return 0;
}
