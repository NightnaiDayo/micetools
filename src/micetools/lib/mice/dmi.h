#include <Windows.h>
#include <stdint.h>

extern LPBYTE dmi_table;
extern WORD dmi_size;

enum DmiType {
    DmiTypeBios = 0,
    DmiTypeSystem = 1,
    DmiTypeString = 11,
};
typedef BYTE DmiType_t;

#pragma pack(push, 1)
typedef struct {
    DmiType_t Type;
    BYTE Length;
    WORD Handle;
} DMI_SECTION_HEADER;

typedef struct {
    CHAR Signature[5];
    BYTE Checksum;
    WORD StructLength;
    DWORD StructAddr;
    WORD NumberOfStructs;
    BYTE BCDRevision;
    BYTE Reserved;
} DMI_HEADER;

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

typedef struct {
    DMI_SECTION_HEADER Head;
    BYTE Manufacturer;
    BYTE ProductName;
    BYTE Version;
    BYTE Serial;
} DMI_SYSTEM;

typedef struct {
    DMI_SECTION_HEADER Head;
    BYTE NoStrings;
} DMI_STRING;
#pragma pack(pop)

static void dmi_init(void);
static void dmi_append(void* data, WORD size);
static void dmi_append_with_strings(void* data, WORD size, int num_strings, ...);

void dmi_build_default(void);
BYTE dmi_calc_checksum(const char* buf, int len);
