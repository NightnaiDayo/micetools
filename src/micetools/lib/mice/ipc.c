#include "ipc.h"

#include <stdio.h>

#include "../util/pid.h"

PMICE_IPC_DATA _miceIpcData = NULL;
static HANDLE hMapFile = NULL;
static HANDLE _miceIpcSendEvent = NULL;
static HANDLE _miceIpcPopEvent = NULL;
static HANDLE _miceIpcMutex = NULL;

BOOL MiceIpcGetGameId(LPSTR lpGameId) {
    if (_miceIpcData == NULL) return FALSE;
    if (!_miceIpcData->m_LauncherIsReady) return FALSE;
    memcpy(lpGameId, _miceIpcData->m_GameId, 4);
    return TRUE;
}

BOOL MiceIpcSetup(BOOL isOwner) {
    DWORD pid;
    if (isOwner)
        pid = GetCurrentProcessId();
    else
        pid = GetOutermostMiceId();

    char mapName[MAX_PATH + 1];
    sprintf_s(mapName, MAX_PATH, "Local\\MiceMappingObject%d", pid);

    SetLastError(0);
    if (isOwner)
        hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                                      sizeof *_miceIpcData, mapName);
    else {
        hMapFile = OpenFileMappingA(FILE_MAP_READ | FILE_MAP_WRITE, TRUE, mapName);
    }
    if (hMapFile == NULL) {
        fprintf(stderr, "Opening shared memory failed: %d\n", GetLastError());
        return FALSE;
    }

    SetLastError(0);
    _miceIpcData = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof *_miceIpcData);
    if (_miceIpcData == NULL) {
        fprintf(stderr, "Mapping view of memory failed: %d\n", GetLastError());
        return FALSE;
    }

    sprintf_s(mapName, MAX_PATH, "MiceMappingObject-Mutex%d", pid);
    _miceIpcMutex = CreateMutexA(NULL, 0, mapName);
    sprintf_s(mapName, MAX_PATH, "MiceMappingObject-SendE%d", pid);
    _miceIpcSendEvent = CreateEventA(NULL, FALSE, FALSE, mapName);
    sprintf_s(mapName, MAX_PATH, "MiceMappingObject-PopE%d", pid);
    _miceIpcPopEvent = CreateEventA(NULL, FALSE, FALSE, mapName);

    if (isOwner) {
        ZeroMemory(_miceIpcData, sizeof *_miceIpcData);
    } else {
        if (!_miceIpcData->m_LauncherIsReady) {
            fprintf(stderr, "IPC connected, but launcher never signalled!\n");
            return FALSE;
        }
    }
    return TRUE;
}

void _MiceIpcLock(void) { WaitForSingleObject(_miceIpcMutex, INFINITE); }
void _MiceIpcUnlock(void) { ReleaseMutex(_miceIpcMutex); }

BOOL MiceIpcPushMessage(PMICE_IPC_MESSAGE lpMessage, BOOL bBlocking) {
    if (!_miceIpcData->m_LauncherIsReady) return FALSE;
    _MiceIpcLock();

    if (_miceIpcData->m_MessagesWaiting == MICE_IPC_MAX_MESSAGES) {
        if (!bBlocking) {
            _MiceIpcUnlock();
            return FALSE;
        }

        do {
            _MiceIpcUnlock();
            WaitForSingleObject(_miceIpcPopEvent, INFINITE);
            _MiceIpcLock();
        } while (_miceIpcData->m_MessagesWaiting == MICE_IPC_MAX_MESSAGES);
    }

    memcpy(&_miceIpcData->m_MessageQueue[_miceIpcData->m_MessageWriteHead], lpMessage,
           sizeof *lpMessage);
    _miceIpcData->m_MessageWriteHead++;
    if (_miceIpcData->m_MessageWriteHead == MICE_IPC_MAX_MESSAGES)
        _miceIpcData->m_MessageWriteHead = 0;
    _miceIpcData->m_MessagesWaiting++;

    SetEvent(_miceIpcSendEvent);

    _MiceIpcUnlock();
    return TRUE;
}

BOOL MiceIpcPopMessage(PMICE_IPC_MESSAGE lpMessage, BOOL bBlocking) {
    _MiceIpcLock();

    if (_miceIpcData->m_MessagesWaiting == 0) {
        if (!bBlocking) {
            _MiceIpcUnlock();
            return FALSE;
        }

        do {
            _MiceIpcUnlock();
            WaitForSingleObject(_miceIpcSendEvent, INFINITE);
            _MiceIpcLock();
        } while (_miceIpcData->m_MessagesWaiting == 0);
    }

    int index = _miceIpcData->m_MessageWriteHead - _miceIpcData->m_MessagesWaiting;
    if (index < 0) index += MICE_IPC_MAX_MESSAGES;
    // Impossible condition, but guard against it just in case!
    if (index < 0 || index >= MICE_IPC_MAX_MESSAGES) {
        _MiceIpcUnlock();
        return FALSE;
    }

    memcpy(lpMessage, &_miceIpcData->m_MessageQueue[index],
           sizeof _miceIpcData->m_MessageQueue[index]);
    _miceIpcData->m_MessagesWaiting--;

    // Signal to any pushes waiting that the queue should have more now
    SetEvent(_miceIpcPopEvent);

    _MiceIpcUnlock();
    return TRUE;
}

static CHAR _szMiceRelative[MAX_PATH + 1];
static DWORD _szMiceRelativeN = 0xffffffff;
LPSTR MiceIpcRelativePath(LPSTR lpChild) {
    if (_miceIpcData && _miceIpcData->m_LauncherIsReady) {
        if (_szMiceRelative[0] == '\0' || _szMiceRelativeN == 0xffffffff) {
            strcpy_s(_szMiceRelative, sizeof _szMiceRelative, _miceIpcData->m_MiceBase);
            _szMiceRelativeN = strlen(_szMiceRelative);

            if (_szMiceRelativeN == 0) {
                strcat_s(_szMiceRelative, sizeof _szMiceRelative, "mice\\");
                _szMiceRelativeN += 5;
            } else {
                strcat_s(_szMiceRelative, sizeof _szMiceRelative, "\\mice\\");
                _szMiceRelativeN += 6;
            }
        }

        strcpy_s(&_szMiceRelative[_szMiceRelativeN], sizeof _szMiceRelative - _szMiceRelativeN,
                 lpChild);
    } else {
        sprintf_s(_szMiceRelative, sizeof _szMiceRelative, ".\\mice\\%s", lpChild);
    }

    return _szMiceRelative;
}
