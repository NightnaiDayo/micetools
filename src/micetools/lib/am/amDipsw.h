#pragma once

#include <Windows.h>

#include "../_am.h"

AM_LIB_H_HEADER(amDipsw, AM_DIPSW)

typedef struct AM_DIPSW {
    BOOL m_Init;
    HANDLE m_Mutex;
    HANDLE m_Handle;
    BYTE m_Addr;
    //
    OVERLAPPED m_Overlapped;
    PSID m_Sid;
} AM_DIPSW;

int amDipswCreateMutex(AM_DIPSW *this);
HANDLE amDipswCreateDeviceFile(GUID *guid, DWORD memberIndex, char *name);

void amDipswExit(void);
void amDipswGetDriverVerision(void);
void amDipswInit(void);
void amDipswModifyByte(void);
void amDipswModifyByteEx(void);
void amDipswModifyByteInternal(void);
void amDipswModifyByteInternalEx(void);
void amDipswMxsmbusStatusCheck(void);
void amDipswReadByte(void);
void amDipswReadByteInternal(void);
void amDipswReadByteInternalEx(void);
void amDipswRequestGetDipsw(void);
void amDipswRequestGetPushsw(void);
void amDipswRequestReadByte(void);
void amDipswRequestSetJvsIoctl(void);
void amDipswRequestSetLed(void);
void amDipswResponse(void);
void amDipswResponseInternalEx(void);
void amDipswWriteByte(void);
void amDipswWriteByteInternal(void);
void amDipswWriteByteInternalEx(void);
