#include <Windows.h>
#include <stdint.h>

extern LPBYTE dmi_table;
extern WORD dmi_size;

// #define PACK(...) __pragma(pack(push, 1)) __VA_ARGS__ __pragma(pack(pop))

#pragma pack(1)
typedef struct {
    BYTE Type;
    BYTE Length;
    WORD Handle;
} DMI_SECTION_HEADER;

#pragma pack(1)
typedef struct {
    CHAR Signature[5];
    BYTE Checksum;
    WORD StructLength;
    DWORD StructAddr;
    WORD NumberOfStructs;
    BYTE BCDRevision;
    BYTE Reserved;
} DMI_HEADER;

#pragma pack(1)
typedef struct {
    DMI_SECTION_HEADER Head;
    BYTE Vendor;
    BYTE Version;
    WORD StartSegment;
    BYTE ReleaseDate;
    BYTE ROMSize;
    uint64_t Chars;
    BYTE VerMajor;
    BYTE VerMinor;
    BYTE ECVerMajor;
    BYTE ECVerMinor;
} DMI_BIOS;

#pragma pack(1)
typedef struct {
    DMI_SECTION_HEADER Head;
    BYTE Manufacturer;
    BYTE ProductName;
    BYTE Version;
    BYTE Serial;
} DMI_SYSTEM;

#pragma pack(1)
typedef struct {
    DMI_SECTION_HEADER Head;
    BYTE NoStrings;
} DMI_STRING;

static void dmi_init(void);
static void dmi_append(void* data, WORD size);
static void dmi_append_with_strings(void* data, WORD size, int num_strings, ...);

void dmi_build_default(void);
BYTE dmi_calc_checksum(const char* buf, int len);
