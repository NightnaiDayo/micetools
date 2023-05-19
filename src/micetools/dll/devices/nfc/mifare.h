#include <Windows.h>

#pragma pack(push, 1)
typedef struct MifareBlock {
    BYTE bytes[16];
} MifareBlock_t;

typedef struct MifareSector {
    MifareBlock_t blocks[4];
} MifareSector_t;

typedef struct MifareMemory_t {
    MifareSector_t sectors[16];
} MifareMemory_t;
#pragma pack(pop)
