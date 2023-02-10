#include "dmi.h"

#include <stdlib.h>

LPBYTE dmi_table = NULL;
WORD dmi_size = 0;
WORD _dmi_max = 0;

DMI_BIOS default_dmi_bios = {
    .Head.Type = DmiTypeBios,
    .Head.Length = 0x12,
    .Head.Handle = 0x0000,
    .Vendor = 0x01,          // "American Megatrends Inc."
    .Version = 0x02,         // "080015 "
    .StartSegment = 0x0000,  // '00 f0'h
    .ReleaseDate = 0x03,     // "07/28/2011"
    .ROMSize = 0x00,         // 03h
    .Chars = 0x1f,           // '1F 90 DA 8B'h
    .VerMajor = 0x08,
    .VerMinor = 0x0f,
    .ECVerMajor = 0xff,
    .ECVerMinor = 0xff,
};

DMI_SYSTEM default_dmi_system = {
    .Head.Type = DmiTypeSystem,
    .Head.Length = 0x08,
    .Head.Handle = 0x0001,
    .Manufacturer = 0x01,  // "NEC"
    .ProductName = 0x02,   // "To Be Filled By O.E.M."
    .Version = 0x03,       // "To Be Filled By O.E.M."
    .Serial = 0x04,        // "To Be Filled By O.E.M."
};

DMI_STRING default_dmi_string = {
    .Head.Type = DmiTypeString,
    .Head.Length = 0x05,
    .Head.Handle = 0x0002,
    .NoStrings = 0x00,
};

static void dmi_init(void) {
    if (dmi_table) free(dmi_table);
    _dmi_max = 0xff;
    dmi_table = (LPBYTE)malloc(_dmi_max);
    memset(dmi_table, 0, _dmi_max);
    dmi_size = 0;
}

static void dmi_append(void* data, WORD size) {
    if (!dmi_table) return;
    while (dmi_size + (size + 1) >= _dmi_max) {
        LPBYTE new_table = (LPBYTE)realloc(dmi_table, _dmi_max += 0xff);
        if (!new_table) return;
        dmi_table = new_table;
    }
    memcpy(dmi_table + dmi_size, data, size);
    dmi_size += size;
    dmi_table[dmi_size++] = 0;
    dmi_table[dmi_size++] = 0;
}

static void dmi_append_with_strings(void* data, WORD size, int num_strings, ...) {
    va_list args;
    va_start(args, num_strings);

    dmi_append(data, size);
    dmi_size -= 2;
    for (int i = 0; i < num_strings; i++) {
        char* str = va_arg(args, char*);
        WORD len = strlen(str) & 0xffff;
        memcpy((char*)dmi_table + dmi_size, str, len + 1);
        dmi_size += len + 1;
        dmi_table[dmi_size - 1] = 0;
    }
    dmi_table[dmi_size++] = 0;

    va_end(args);
}

void dmi_build_default() {
    dmi_init();
    dmi_append_with_strings(&default_dmi_bios, sizeof default_dmi_bios, 3,
                            "American Megatrends Inc.", "080015 ", "07/28/2011");

    // Platform AAM: Board type one of "Supermicro"(=1) or "Advantech"(=2)
    // Platform AAL: Board type one of "NEC"(=0) or "AAL2"(=3)
    dmi_append_with_strings(&default_dmi_system, sizeof default_dmi_system, 4, "NEC",
                            "To Be Filled By O.E.M.", "To Be Filled By O.E.M.",
                            "To Be Filled By O.E.M.");

    default_dmi_string.NoStrings = 5;

    dmi_append_with_strings(&default_dmi_string, sizeof default_dmi_string,
                            // OEM strings:
                            // 0: ??
                            // 1: ??
                            // 2: Platform ID (AAL, AAM)
                            //      AAL = Nvidia drivers
                            //      AAM = AMD drivers
                            //      *** = No dedicated drivers
                            // 3: ??
                            // 4: Board type (AAL, NEC, AAL2, " ", AAM, Supermicro, Advantech)
                            //      If present -> makes board = 4
                            //      AAL = 4
                            //      AAM = 4
                            //      Supermicro = 4
                            //      Advantech = 4
                            // These values are pulled from an 846-5004D
                            5, "DAC-BJ02", "DAC-BJ02", "AAL", "Advantech", "AAL2");
}

BYTE dmi_calc_checksum(const char* buf, int len) {
    int sum = 0;
    int a;
    for (a = 0; a < len; a++) sum += buf[a];
    return (BYTE)(sum == 0);
}
