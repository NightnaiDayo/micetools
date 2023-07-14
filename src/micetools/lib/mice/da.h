#include <Windows.h>

typedef struct MICE_DA {
    DWORD_PTR m_Capacity;
    DWORD_PTR m_Length;
    DWORD_PTR m_Size;
    LPVOID m_Array;
} MICE_DA, *PMICE_DA;
#define MICE_DA_NEW(name, type)                                                 \
    MICE_DA __mda##name = {                                                     \
        .m_Capacity = 0, .m_Length = 0, .m_Size = sizeof(type), .m_Array = NULL \
    };                                                                          \
    PMICE_DA name = &__mda##name;

#define _MICE_DA_INDEX_P(lpDa, index) \
    (LPVOID)((DWORD_PTR)((lpDa)->m_Array) + (index) * (lpDa)->m_Size)
#define _MICE_DA_COPY_INTO_DA(lpDa, index, value) \
    memcpy(_MICE_DA_INDEX_P((lpDa), (index)), (value), (lpDa)->m_Size)
#define _MICE_DA_COPY_OUTOF_DA(lpDa, index, value) \
    memcpy((value), _MICE_DA_INDEX_P((lpDa), (index)), (lpDa)->m_Size)

#define MICE_DA_ITER(lpDa, type, name)                                                  \
    for (DWORD_PTR __mdaI##lpDa = 0; __mdaI##lpDa < (lpDa)->m_Length; __mdaI##lpDa++) { \
        type* name = (type*)_MICE_DA_INDEX_P((lpDa), __mdaI##lpDa);
#define MICE_DA_ITER_END }
#define MICE_DA_REMOVE_CURRENT(lpDa) MiceDARemove((lpDa), __mdaI##lpDa)

BOOL MiceDAPush(PMICE_DA lpDa, LPVOID lpSrc);
BOOL MiceDAPop(PMICE_DA lpDa, LPVOID lpDst);
BOOL MiceDAShift(PMICE_DA lpDa, LPVOID lpSrc);
BOOL MiceDAUnshift(PMICE_DA lpDa, LPVOID lpDst);
BOOL MiceDASet(PMICE_DA lpDa, DWORD_PTR dwIndex, LPVOID lpValue);
LPVOID MiceDAGet(PMICE_DA lpDa, DWORD_PTR dwIndex);
BOOL MiceDARemove(PMICE_DA lpDa, DWORD_PTR dwIndex);
DWORD_PTR MiceDALength(PMICE_DA lpDa);
