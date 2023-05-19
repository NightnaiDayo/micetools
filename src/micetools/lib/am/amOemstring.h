#pragma once

#include <Windows.h>

#include "../_am.h"

AM_LIB_H_HEADER(amOemstring, AM_OEMSTRING)

// Much easier than pulling in winddk.h
typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;
typedef struct {
    PHYSICAL_ADDRESS m_physAddr;
    DWORD m_elementSize;
    DWORD m_elementCount;
} AM_COLUMBA_REQUEST;

typedef struct AM_OEMSTRING {
    BOOL m_loaded;
    CHAR m_biosVersion[32];
    CHAR m_biosDate[11];
    CHAR m_systemManufacturer[32];
    CHAR m_strings[5][32];
} AM_OEMSTRING;


typedef enum {
    AM_OEMSTRING_STATUS_OK = 0,
    AM_OEMSTRING_STATUS_NG = -1,
    AM_OEMSTRING_STATUS_ERR_INVALID_PARAM = -2,
    AM_OEMSTRING_STATUS_ERR_SYS = -3,
} AM_OEMSTRING_STATUS;


void amiOemstringLocateDMITable(HANDLE hColumba, LPDWORD lpDmiBase, LPWORD lpDmiLength);
void amiOemstringStoreString(BYTE type, int stringno, LPSTR string);
BOOL amiOemstringLoadStrings(void);

AM_OEMSTRING_STATUS amOemstringGetManufacturer(LPSTR manufacturer);
AM_OEMSTRING_STATUS amOemstringGetOemstring(LPSTR oemstring, int which);
AM_OEMSTRING_STATUS amOemstringGetSBiosVer(LPSTR biosVersion);
AM_OEMSTRING_STATUS amOemstringGetSBiosReleaseDate(LPSTR biosDate);
