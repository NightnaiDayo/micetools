#pragma once

#include <Windows.h>

#define MICE_IPC_MAX_MESSAGES 160

typedef enum MICE_IPC_TYPE {
    MICE_IPC_TYPE_INVALID = 0,
    MICE_IPC_TYPE_LOG = 1,
} MICE_IPC_TYPE;

typedef struct MICE_IPC_MESSAGE {
    MICE_IPC_TYPE m_Type;
    DWORD m_Size;
    BYTE m_Message[1024];
} MICE_IPC_MESSAGE, *PMICE_IPC_MESSAGE;

typedef struct MICE_IPC_DATA {
    BOOL m_LauncherIsReady;
    char m_GameId[4];
    char m_PathPrefix[MAX_PATH + 1];
    char m_MiceBase[MAX_PATH + 1];
    char m_MiceDll[MAX_PATH + 1];

    DWORD m_MessageWriteHead;
    DWORD m_MessagesWaiting;
    MICE_IPC_MESSAGE m_MessageQueue[MICE_IPC_MAX_MESSAGES];
} MICE_IPC_DATA, *PMICE_IPC_DATA;

BOOL MiceIpcSetup(BOOL isOwner);
BOOL MiceIpcGetGameId(LPSTR lpGameId);

void _MiceIpcLock(void);
void _MiceIpcUnlock(void);

BOOL MiceIpcPushMessage(PMICE_IPC_MESSAGE lpMessage, BOOL bBlocking);
BOOL MiceIpcPopMessage(PMICE_IPC_MESSAGE lpMessage, BOOL bBlocking);
LPSTR MiceIpcRelativePath(LPSTR lpChild);

extern PMICE_IPC_DATA _miceIpcData;
