#include "amOemstring.h"

#include <stdio.h>

#include "../mice/dmi.h"
#include "../mice/mice.h"

AM_LIB_C_HEADER(amOemstring, AM_OEMSTRING)

#define amOemstringScanStart 0xf0000
#define amOemstringScanStep 0x7ff0
#define amOemstringScanEnd 0xfffdf
void amiOemstringLocateDMITable(HANDLE hColumba, LPDWORD lpDmiBase, LPWORD lpDmiLength) {
    AM_COLUMBA_REQUEST request;
    char buffer[amOemstringScanStep + 24];

    int scanAddr = amOemstringScanStart;
    do {
        request.m_physAddr.LowPart = scanAddr & 0xffffffff;
        request.m_physAddr.HighPart = scanAddr >> 31;
        request.m_elementSize = 1;
        request.m_elementCount = sizeof buffer;

        DWORD bytesOut = 0;
        BOOL ret = DeviceIoControl(hColumba, IOCTL_COLUMBA_READ, &request, sizeof request, buffer,
                                   sizeof buffer, &bytesOut, NULL);
        if (ret && bytesOut == sizeof buffer) {
            LPBYTE pBuffer = (LPBYTE)&buffer;
            for (int i = 0; i < (sizeof buffer) - 8; i++) {
                if (memcmp(pBuffer, "_DMI_", 5) == 0) {
                    *lpDmiLength = *((LPWORD)(pBuffer + 6));
                    *lpDmiBase = *((LPDWORD)(pBuffer + 8));
                    return;
                }
                pBuffer++;
            }
        }

        scanAddr += amOemstringScanStep;
    } while (scanAddr <= amOemstringScanEnd);
}

void amiOemstringStoreString(BYTE type, int stringno, LPSTR string) {
    if (type == DmiTypeBios) {
        if (stringno == 1)
            strncpy_s(amOemstring.m_biosVersion, sizeof amOemstring.m_biosVersion, string, 0xff);
        if (stringno == 2)
            strncpy_s(amOemstring.m_biosDate, sizeof amOemstring.m_biosDate, string, 0xff);
    } else if (type == DmiTypeSystem) {
        if (stringno == 0)
            strncpy_s(amOemstring.m_systemManufacturer, sizeof amOemstring.m_systemManufacturer,
                      string, 0xff);

    } else if (type == DmiTypeString) {
        if (stringno < 5)
            strncpy_s(amOemstring.m_strings[stringno], sizeof amOemstring.m_strings[stringno],
                      string, 0xff);
    }
}

BOOL amiOemstringLoadStrings(void) {
    if (amOemstring.m_loaded) return TRUE;
    ZeroMemory(&amOemstring, sizeof amOemstring);

    BYTE buffer[0x10000];

    HANDLE hColumba = CreateFileW(L"\\\\.\\columba", GENERIC_READ | GENERIC_WRITE, 0, NULL,
                                  OPEN_EXISTING, 0, NULL);
    if (hColumba == INVALID_HANDLE_VALUE) return FALSE;

    DWORD me = 0;

    DWORD dmiBase = 0;
    WORD dmiLength = 0;

    amiOemstringLocateDMITable(hColumba, &dmiBase, &dmiLength);
    if (dmiBase == 0) return FALSE;

    AM_COLUMBA_REQUEST request;
    request.m_physAddr.QuadPart = dmiBase;
    request.m_elementSize = 1;

    if (dmiBase + 0x10000 < 0x100001) {
        request.m_elementCount = 0x10000;
    } else {
        request.m_elementCount = 0x100000 - dmiBase;
    }

    DWORD nBytesReturned;
    if (!DeviceIoControl(hColumba, IOCTL_COLUMBA_READ, &request, sizeof request, buffer,
                         sizeof buffer, &nBytesReturned, NULL)) {
        return FALSE;
    }

    if (nBytesReturned != 0x10000) return FALSE;

    BYTE type_seen[128] = { 0 };

    LPBYTE pBuffer = (LPBYTE)&buffer;
    while (1) {
        DMI_SECTION_HEADER* header = (DMI_SECTION_HEADER*)pBuffer;
        if (header->Length == 0) break;
        // Avoid reading outside the DMI tables
        if ((pBuffer - (LPBYTE)&buffer) + header->Length >= dmiLength) break;
        // Avoid reading outside the buffer!
        if ((pBuffer - (LPBYTE)&buffer) + header->Length >= sizeof buffer) break;

        BYTE type = header->Type;
        pBuffer += header->Length;

        int stringno = 0;
        do {
            if (!type_seen[type]) amiOemstringStoreString(type, stringno, (LPSTR)pBuffer);
            stringno++;

            while (pBuffer[0] != '\0') pBuffer++;
            pBuffer++;
        } while (pBuffer[0] != '\0');
        pBuffer++;

        if (type < sizeof type_seen) type_seen[type] = 1;
    }

    amOemstring.m_loaded = TRUE;
    CloseHandle(hColumba);
    return TRUE;
}

AM_OEMSTRING_STATUS amOemstringGetManufacturer(LPSTR manufacturer) {
    if (manufacturer == NULL) return AM_OEMSTRING_STATUS_ERR_INVALID_PARAM;
    if (amiOemstringLoadStrings() == 0) {
        manufacturer[0] = '\0';
        return AM_OEMSTRING_STATUS_ERR_SYS;
    }
    strncpy_s(manufacturer, sizeof amOemstring.m_systemManufacturer,
              amOemstring.m_systemManufacturer, 0xffffffff);
    return AM_OEMSTRING_STATUS_OK;
}

AM_OEMSTRING_STATUS amOemstringGetSBiosVer(LPSTR biosVersion) {
    if (biosVersion == NULL) return AM_OEMSTRING_STATUS_ERR_INVALID_PARAM;
    if (amiOemstringLoadStrings() == 0) {
        biosVersion[0] = '\0';
        return AM_OEMSTRING_STATUS_ERR_SYS;
    }
    strncpy_s(biosVersion, sizeof amOemstring.m_biosVersion, amOemstring.m_biosVersion, 0xffffffff);
    return AM_OEMSTRING_STATUS_OK;
}

AM_OEMSTRING_STATUS amOemstringGetSBiosReleaseDate(LPSTR biosDate) {
    if (biosDate == NULL) return AM_OEMSTRING_STATUS_ERR_INVALID_PARAM;
    if (amiOemstringLoadStrings() == 0) {
        biosDate[0] = '\0';
        return AM_OEMSTRING_STATUS_ERR_SYS;
    }
    strncpy_s(biosDate, sizeof amOemstring.m_biosDate, amOemstring.m_biosDate, 0xffffffff);
    return AM_OEMSTRING_STATUS_OK;
}

AM_OEMSTRING_STATUS amOemstringGetOemstring(LPSTR oemstring, int which) {
    LPSTR sVal;

    if (oemstring == NULL) return AM_OEMSTRING_STATUS_ERR_INVALID_PARAM;

    switch (which) {
        case 0:
            sVal = amOemstring.m_strings[0];
            break;
        case 1:
            sVal = amOemstring.m_strings[1];
            break;
        case 2:
            sVal = amOemstring.m_strings[2];
            break;
        case 3:
            sVal = amOemstring.m_strings[3];
            break;
        case 4:
            sVal = amOemstring.m_strings[4];
            break;
        default:
            sVal = NULL;
    }

    oemstring[0] = '\0';
    if (sVal == NULL) return AM_OEMSTRING_STATUS_ERR_INVALID_PARAM;
    if (amiOemstringLoadStrings() == 0) return AM_OEMSTRING_STATUS_ERR_SYS;
    strncpy_s(oemstring, sizeof amOemstring.m_strings[0], sVal, 0xffffffff);
    return AM_OEMSTRING_STATUS_OK;
}
