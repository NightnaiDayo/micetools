#include "mx.h"

#pragma pack(push, 1)
typedef struct {
    LPBYTE dest;
    LPBYTE src;
    DWORD nBytes;
} HTSYSMNT_1x, *LPHTSYSMNT_1x;

typedef struct {
    DWORD offset;
    LPBYTE data;
    DWORD nBytes;
} HTSYSMNT_2x, *LPHTSYSMNT_2x;

typedef struct {
    void (*callback)(void);
} HTSYSMNT_3x, *LPHTSYSMNT_3x;
#pragma pack(pop)

static BYTE HTSYS_BUFFER[256];

static BOOL htsysmntBufferTransact(DWORD offset, LPBYTE data, DWORD nbytes, BOOL doWrite) {
    LPBYTE source;
    LPBYTE destination;

    if (!(offset < 16 && nbytes < 16)) return FALSE;

    destination = HTSYS_BUFFER + offset * 16;
    source = data;
    if (!doWrite) {
        source = destination;
        destination = data;
    }
    memcpy(destination, source, nbytes);
    return TRUE;
}

static BOOL htsysmntExchangeBuffers(LPBYTE dest, LPBYTE src, DWORD nBytes, BOOL direction) {
    if (dest == NULL || src == NULL || nBytes == 0) return FALSE;

    LPBYTE destination;
    destination = dest;
    if (!direction) {
        destination = src;
        src = dest;
    }

    memcpy(src, destination, nBytes);
    return TRUE;
}

BOOL WINAPI htsysmnt_DeviceIoControl(file_context_t* ctx, DWORD dwIoControlCode, LPVOID lpInBuffer,
                                     DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                                     LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    DWORD expectedIn = 0;
    DWORD expectedOut = /* 4 */ sizeof(BOOL);

    switch (dwIoControlCode) {
        case IOCTL_CRACKPROOF_10:
        case IOCTL_CRACKPROOF_11:
        case IOCTL_CRACKPROOF_20:
        case IOCTL_CRACKPROOF_21:
            expectedIn = 12;
            break;
        case IOCTL_CRACKPROOF_30:
            expectedIn = 4;
            break;
        default:
            log_warning(plfHtsysmnt, "unhandled 0x%08x", dwIoControlCode);
            return FALSE;
    }

    if (nInBufferSize != expectedIn || nOutBufferSize != expectedOut) {
        return FALSE;
    }

    BOOL result = TRUE;

    switch (dwIoControlCode) {
        case IOCTL_CRACKPROOF_10:
        case IOCTL_CRACKPROOF_11: {
            result = htsysmntExchangeBuffers(
                ((LPHTSYSMNT_1x)lpInBuffer)->dest, ((LPHTSYSMNT_1x)lpInBuffer)->src,
                ((LPHTSYSMNT_1x)lpInBuffer)->nBytes, dwIoControlCode == IOCTL_CRACKPROOF_11);
            break;
        }

        case IOCTL_CRACKPROOF_20:
        case IOCTL_CRACKPROOF_21: {
            result = htsysmntBufferTransact(
                ((LPHTSYSMNT_2x)lpInBuffer)->offset, ((LPHTSYSMNT_2x)lpInBuffer)->data,
                ((LPHTSYSMNT_2x)lpInBuffer)->nBytes, dwIoControlCode == IOCTL_CRACKPROOF_21);
            break;
        }

        case IOCTL_CRACKPROOF_30:
            result = TRUE;
            ((LPHTSYSMNT_3x)lpInBuffer)->callback();
            break;
    }

    *(LPBOOL)lpOutBuffer = result;
    *lpBytesReturned = expectedOut;
    return TRUE;
}

void setup_htsysmnt() {
    file_hook_t* htsysmnt = new_file_hook(L"\\\\.\\Htsysm");
    htsysmnt->DeviceIoControl = &htsysmnt_DeviceIoControl;

    hook_file(htsysmnt);
}
