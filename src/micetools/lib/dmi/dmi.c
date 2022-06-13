#include "dmi.h"

LPBYTE dmi_table = NULL;
WORD dmi_size = 0;
size_t _dmi_max = 0;

DMI_BIOS deafult_dmi_bios = {
    .Type = 0x00,
    .Length = 0x12,
    .Handle = 0x0000,
    .Vendor = 0x00,
    .Version = 0x00,
    .StartSegment = 0x0000,
    .ReleaseDate = 0x00,
    .ROMSize = 0x00,
    .Chars = 0x04,
};

DMI_SYSTEM default_dmi_system = {
    .Type = 0x01,
    .Length = 0x08,
    .Handle = 0x0000,
    .Manufacturer = 0x00,
    .ProductName = 0x00,
    .Version = 0x00,
    .Serial = 0x00,
};

DMI_STRING deafult_dmi_string = {
    .Type = 0x0b,
    .Length = 0x05,
    .Handle = 0x0000,
    .NoStrings = 0x00,
};

static void dmi_init(void) {
	if (dmi_table) free(dmi_table);
	dmi_table = (LPBYTE)malloc(0xff);
	dmi_size = 0;
	_dmi_max = 1024;
}

static void dmi_append(void* data, size_t size) {
	if (!dmi_table) return;
	while (dmi_size + (size + 1) >= _dmi_max) {
		LPBYTE new_table = (LPBYTE)realloc(dmi_table, _dmi_max += 0xff);
		if (!new_table) return;
		dmi_table = new_table;
	}
	memcpy(dmi_table + dmi_size, &data, size);
	dmi_size += size;
	dmi_table[dmi_size++] = 0;
	dmi_table[dmi_size++] = 0;
}

static void dmi_append_with_strings(void* data, size_t size, int num_strings, ...) {
	va_list args;
	va_start(args, num_strings);

	dmi_append(data, size);
	dmi_size -= 2;
	for (int i = 0; i < num_strings; i++) {
		char* str = va_arg(args, char*);
		int len = strlen(str);
// #pragma warning(disable : 4996)
		strcpy((char*)dmi_table + dmi_size, str);
		dmi_size += len + 1;
		dmi_table[dmi_size - 1] = 0;
	}
	dmi_table[dmi_size++] = 0;

	va_end(args);
}

void dmi_build_default() {
	dmi_init();
	dmi_append(&deafult_dmi_bios, sizeof deafult_dmi_bios);
	// Platform AAM: Board type one of "Supermicro"(=1) or "Advantech"(=2)
	// Platform AAL: Board type one of "NEC"(=0) or "AAL2"(=3)

    dmi_append_with_strings(&default_dmi_system, sizeof default_dmi_system, 1, "Supermicro");

    deafult_dmi_string.NoStrings = 3;

	dmi_append_with_strings(&deafult_dmi_string, sizeof deafult_dmi_string,
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
		5, ".", ".", "AAM", ".", "AAL"
	);
}

BYTE dmi_calc_checksum(const char* buf, int len) {
	int sum = 0;
	int a;
	for (a = 0; a < len; a++)
		sum += buf[a];
	return (BYTE)(sum == 0);
}