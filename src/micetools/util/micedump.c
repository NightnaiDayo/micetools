#include <Windows.h>
#include <setupapi.h>
#include <shellapi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#pragma comment(lib, "Setupapi.lib")

#include "../lib/am/amEeprom.h"
#include "../lib/mxk/mxk.h"

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

        BOOL succ = DeviceIoControl(columba, IOCTL_COLUMBA_READ, &request, sizeof request, &readBuf,
                                    sizeof readBuf, &bytesOut, NULL);
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

    BOOL s = DeviceIoControl(columba, IOCTL_COLUMBA_READ, &request, sizeof request, readBuf,
                             sizeof readBuf, &bytesReturned, NULL);
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
    DeviceIoControl(mxsmbus, IOCTL_MXSMBUS_GET_VERSION, NULL, 0, &version, sizeof version, &_dummy,
                    NULL);
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
    s = DeviceIoControl(mxsram, IOCTL_MXSRAM_PING, NULL, 0, &version, sizeof version, &_dummy,
                        NULL);
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
    s = DeviceIoControl(mxsram, IOCTL_MXSRAM_GET_SECTOR_SIZE, NULL, 0, &ssize, sizeof ssize,
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
    DeviceIoControl(mxsuperio, IOCTL_MXSUPERIO_READ, payload, sizeof payload, payload,
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
    s = DeviceIoControl(mxsuperio, IOCTL_MXSUPERIO_PING, NULL, 0, &version, sizeof version, &_dummy,
                        NULL);
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
        DeviceIoControl(mxsuperio, IOCTL_MXSUPERIO_HWMONITOR_LPC_READ, &packet, sizeof packet,
                        &packet, sizeof packet, &_dummy, NULL);
        printf(" -> %02x: %02x\n", reg, packet[2]);
    }

    CloseHandle(mxsuperio);
    return TRUE;
}

void kc_experiments(HANDLE mxparallel) {
    BYTE packet[16];
    BYTE test;

    // Returns FF all the time!
    // puts("Attempting to rekey key!!");
    // // OLD ID: A72E-02D11266103
    // ZeroMemory(packet, 16);
    // packet[0] = 23;
    // mxkSendPacket(mxparallel, packet);
    // memcpy(packet, "A72E-69696969696", 16);
    // mxkSendPacket(mxparallel, packet);
    // mxkRecvPacket(mxparallel, packet);
    // puts("Response:");
    // for (int i = 0; i < 16; i++) {
    //     printf("%02x ", packet[i]);
    // }
    // puts("");

    /*
    15: seems to send random bytes back? rng?
    16: echo, but byte 0 == 0
    17: echo, but byte 0 == 0
    18: hangs. need to send some more payload?
    19: as 18

    26:
    */
    // puts("KEY_R:");
    // for (int i = 0; i < 16; i++) {
    //     printf("%02x ", KEY_R[i]);
    // }
    // puts("");
    // puts("KEY_S:");
    // for (int i = 0; i < 16; i++) {
    //     printf("%02x ", KEY_S[i]);
    // }
    // puts("");

    // for (int i = 0; i < 10; i++) {
    //     for (int j = 0; j < 2; j++) {
    //         for (test = 15; test < 16; test++) {
    //             ZeroMemory(packet, 16);
    //             packet[0] = test;
    //             printf("Sending %d\n", test);
    //             mxkSendPacket(mxparallel, packet);
    //             mxkRecvPacket(mxparallel, packet);
    //             puts("Response:");
    //             for (int i = 0; i < 16; i++) {
    //                 printf("%02x ", packet[i]);
    //             }
    //             puts("");
    //         }
    //     }
    //     Sleep(250);
    // }

    ZeroMemory(packet, 16);
    for (int i = 0; i < 16; i++) packet[i] = i;
    packet[0] = 17;
    printf("Sending %d\n", 17);
    mxkSendPacket(mxparallel, packet);

    /**
     * 18:
     *      Returns the same stuff for all keychips
     *      00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
     *      returns: 21 D8 A8 21 2A CD A9 4A 5C D9 7A 35 43 ED 83 7A
     *      12 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
     *      returns: 65 FA F5 BE 9F 93 8D 02 6E C4 D5 60 6E C1 74 D6
     * 19:
     *      00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
     *      returns: 57 45 43 37 D0 80 48 B1 86 26 A7 81 70 1D 1F 4F
     * */
    BYTE response[16];
    mxkRecvPacket(mxparallel, response);
    puts("Response:");
    for (int i = 0; i < 16; i++) {
        printf("%02x ", response[i]);
    }
    puts("");

    puts("Sending DoAs");
    packet[0] = EepromWrite;
    packet[1] = 0x20;
    mxkSendPacket(mxparallel, packet);
    ZeroMemory(packet, 16);
    packet[0] = 'D';
    packet[1] = 'o';
    packet[2] = 'A';
    packet[3] = 's';
    mxkSendPacket(mxparallel, packet);
    mxkRecvPacket(mxparallel, packet);
    if (packet[0] == 0xff) puts("Failed to DoAs");
    else puts("DoAs'd");

}

BYTE KEYCHIP_ID[16];
BYTE MAIN_ID[16];
BOOL dump_keychip() {
    SECTION_HEAD("keychip");
    FILE *fd;

    HANDLE mxparallel = OpenDriver("mxparallel");
    if (mxparallel == INVALID_HANDLE_VALUE) return FALSE;

    mxkTransportInitPic(mxparallel);
    if (!mxkExchengeAesKey(mxparallel)) {
        puts("Failed to exchange keys!");
        return FALSE;
    }

    unsigned short version;
    if (!mxkVersion(mxparallel, &version)) {
        puts("Failed to request version!");
    } else {
        printf("Version:\t%02x%02x\n", version & 0xff, version >> 8);
    }

    ZeroMemory(MAIN_ID, 16);
    if (!mxkGetMainId(mxparallel, MAIN_ID)) {
        puts("Failed to request main ID!");
    } else {
        printf("Main ID:\t%.16s\n", MAIN_ID);
    }

    ZeroMemory(MAIN_ID, 16);
    if (!mxkSetMainId(mxparallel, MAIN_ID)) {
        puts("Failed to zero main ID!");
    }

    ZeroMemory(KEYCHIP_ID, 16);
    if (!mxkGetKeyId(mxparallel, KEYCHIP_ID)) {
        puts("Failed to request key ID!");
    } else {
        printf("Keychip ID:\t%.16s\n", KEYCHIP_ID);
    }

    appboot_t appboot;
    if (!mxkGetAppBootInfo(mxparallel, &appboot)) {
        puts("Failed to request appboot info!");
    } else {
        printf("Game ID:\t%.4s\n", appboot.game_id);
        printf("Region:\t\t%02x\n", appboot.region);
        printf("Model Type:\t%02x\n", appboot.model_type);
        printf("Sys Flag:\t%02x\n", appboot.system_flag);
        printf("Platform ID:\t%.3s\n", appboot.platform_id);
        printf("DVD Flag:\t%02x\n", appboot.dvd_flag);
        printf("Network addr:\t%d.%d.%d.%d\n", appboot.network_addr & 0xff,
               (appboot.network_addr >> 8) & 0xff, (appboot.network_addr >> 16) & 0xff,
               (appboot.network_addr >> 24) & 0xff);
        puts("Appboot seed:");
        for (int i = 0; i < 16; i++) {
            printf(" %02x", appboot.seed[i]);
        }
        puts("");
    }

    DWORD playcount;
    if (!mxkGetPlayCounter(mxparallel, &playcount)) {
        puts("Failed to request playcount!");
    } else {
        printf("Playcount:\t%d\n", playcount);
    }

    kc_experiments(mxparallel);

    goto skip_kc_dump;

    unsigned char eeprom_block[0x10];
    puts("Dumping EEPROM...");
    fopen_s(&fd, "eeprom.bin", "wb");
    for (int page = 0; page < 0x80; page++) {
        if (!mxkEepromRead(mxparallel, page, eeprom_block)) {
            printf("\nFailed to read EEPROM:%02x\n", page);
        } else {
            fseek(fd, page * 16, SEEK_SET);
            fwrite(eeprom_block, sizeof eeprom_block, 1, fd);
            printf("Wrote EEPROM:%02x to eeprom.bin\r", page);
        }
    }
    puts("\nEEPROM dump complete");
    fclose(fd);

    unsigned char nvram_block[0x100];
    puts("Dumping NVRAM...");
    fopen_s(&fd, "nvram.bin", "wb");
    for (int addr = 0; addr < 0x800; addr += 16 * 16) {
        if (!mxkNvramRead(mxparallel, addr, 16, nvram_block)) {
            printf("\nFailed to read NVRAM:%03x\n", addr);
        } else {
            fseek(fd, addr, SEEK_SET);
            fwrite(nvram_block, sizeof nvram_block, 1, fd);
            printf("Wrote NVRAM:%03x to nvram.bin\r", addr);
        }
    }
    puts("\nNVRAM dump complete");
    fclose(fd);

    unsigned char billing[0x10c];
    if (!mxkFlashRead(mxparallel, 0x7a000, 0x10c, billing)) {
        puts("Failed to request billing information");
    } else {
        fopen_s(&fd, "billing.bin", "wb");
        fwrite(billing, sizeof billing, 1, fd);
        fclose(fd);
        puts("Wrote billing information to billing.bin");
    }

    unsigned char billing_copy[0x10c];
    if (!mxkFlashRead(mxparallel, 0x7b000, 0x10c, billing_copy)) {
        puts("Failed to request backup billing information");
    } else {
        if (memcmp(billing, billing_copy, 0x10c) == 0) {
            puts("Billing backup [OK]");
        } else {
            puts("Billing backup miss-match!");
            fopen_s(&fd, "billing-backup.bin", "wb");
            fwrite(billing_copy, sizeof billing_copy, 1, fd);
            fclose(fd);
            puts("Wrote billing backup information to billing-backup.bin");
        }
    }

    unsigned char flash[0x1000];
    puts("Attempting to dump flash...");
    fopen_s(&fd, "flash.bin", "wb");
    for (int addr = 0; addr < 0x80000; addr += sizeof flash) {
        if (!mxkFlashRead(mxparallel, addr, sizeof flash, flash)) {
            printf("\nFailed to read flash:%05x\n", addr);
        } else {
            fseek(fd, addr, SEEK_SET);
            fwrite(flash, sizeof flash, 1, fd);
            printf("Wrote flash:%05x to flash.bin\r", addr);
        }
    }
    puts("\nFlash dump complete");
    fclose(fd);

skip_kc_dump:
    CloseHandle(mxparallel);
    return TRUE;
}

int main() {
    // if (!dump_columba()) {
    //     printf("Failed to dump DMI: %03x\n", GetLastError());
    // }
    if (!dump_keychip()) {
        printf("Failed to dump keychip: %03x\n", GetLastError());
    }
    // if (!dump_eeprom()) {
    //     printf("Failed to dump EEPROM: %03x\n", GetLastError());
    // }
    // if (!dump_sram()) {
    //     printf("Failed to dump SRAM: %03x\n", GetLastError());
    // }
    // if (!dump_superio()) {
    //     printf("Failed to dump SuperIO: %03x\n", GetLastError());
    // }
    return 0;
}
