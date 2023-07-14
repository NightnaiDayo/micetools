#include "da.h"

#include <malloc.h>
#include <stdlib.h>

static inline void _MiceDAGrow(PMICE_DA lpDa) {
    DWORD_PTR newSize = lpDa->m_Capacity * 2 + 1;
    if (lpDa->m_Array) {
        LPVOID newArray = realloc(lpDa->m_Array, lpDa->m_Size * newSize);
        if (newArray != lpDa->m_Array) {
            memcpy(newArray, lpDa->m_Array, lpDa->m_Size * lpDa->m_Capacity);
        }
        lpDa->m_Array = newArray;
    } else {
        lpDa->m_Array = malloc(lpDa->m_Size * newSize);
    }
    lpDa->m_Capacity = newSize;
}

BOOL MiceDAPush(PMICE_DA lpDa, LPVOID lpSrc) {
    if (!lpDa || !lpSrc) return FALSE;
    if (lpDa->m_Capacity == lpDa->m_Length) _MiceDAGrow(lpDa);

    _MICE_DA_COPY_INTO_DA(lpDa, lpDa->m_Length, lpSrc);
    lpDa->m_Length++;
    return TRUE;
}
BOOL MiceDAPop(PMICE_DA lpDa, LPVOID lpDst) {
    if (!lpDa) return FALSE;
    if (lpDa->m_Length == 0) return FALSE;
    lpDa->m_Length--;
    if (lpDst) _MICE_DA_COPY_OUTOF_DA(lpDa, lpDa->m_Length, lpDst);
    return TRUE;
}

BOOL MiceDAShift(PMICE_DA lpDa, LPVOID lpSrc) {
    if (!lpDa || !lpSrc) return FALSE;
    if (lpDa->m_Capacity == lpDa->m_Length) _MiceDAGrow(lpDa);

    // Copy every value one index forwards
    for (DWORD_PTR i = lpDa->m_Length; i; i--) {
        memcpy(_MICE_DA_INDEX_P(lpDa, i), _MICE_DA_INDEX_P(lpDa, i + 1), lpDa->m_Size);
    }
    _MICE_DA_COPY_INTO_DA(lpDa, 0, lpSrc);
    lpDa->m_Length++;
    return TRUE;
}
BOOL MiceDAUnshift(PMICE_DA lpDa, LPVOID lpDst) {
    if (!lpDa) return FALSE;
    if (lpDst) _MICE_DA_COPY_OUTOF_DA(lpDa, 0, lpDst);
    lpDa->m_Length--;
    // Copy every value one index back
    for (DWORD_PTR i = 0; i < lpDa->m_Length; i++) {
        memcpy(_MICE_DA_INDEX_P(lpDa, i), _MICE_DA_INDEX_P(lpDa, i + 1), lpDa->m_Size);
    }
    return TRUE;
}
BOOL MiceDASet(PMICE_DA lpDa, DWORD_PTR dwIndex, LPVOID lpValue) {
    if (!lpDa || !lpValue) return FALSE;
    if (dwIndex >= lpDa->m_Length) return FALSE;
    _MICE_DA_COPY_INTO_DA(lpDa, dwIndex, lpValue);
    return TRUE;
}
LPVOID MiceDAGet(PMICE_DA lpDa, DWORD_PTR dwIndex) {
    if (!lpDa) NULL;
    if (dwIndex >= lpDa->m_Length) return NULL;
    return _MICE_DA_INDEX_P(lpDa, dwIndex);
}

BOOL MiceDARemove(PMICE_DA lpDa, DWORD_PTR dwIndex) {
    if (!lpDa) return FALSE;
    if (dwIndex > lpDa->m_Length) return FALSE;
    if (dwIndex == lpDa->m_Length) {
        lpDa->m_Length--;
        return TRUE;
    }
    lpDa->m_Length--;
    for (DWORD_PTR i = dwIndex; i < lpDa->m_Length; i++) {
        memcpy(_MICE_DA_INDEX_P(lpDa, i), _MICE_DA_INDEX_P(lpDa, i + 1), lpDa->m_Size);
    }
    return TRUE;
}

DWORD_PTR MiceDALength(PMICE_DA lpDa) {
    if (!lpDa) _invalid_parameter_noinfo_noreturn();
    return lpDa->m_Length;
}
