#include <Windows.h>

#include "../../input.h"
#include "../jvs.h"

#define JVS_IO_MAX 31

#define MICE_JVS_MAX 256
// Every single byte could be masked
#define MICE_JVS_MASKED (MICE_JVS_MAX * 2)

#define new(type, instance) _##type##_vftable.ctor(&instance)

typedef struct MICE_JVS MICE_JVS, *PMICE_JVS;
typedef struct MICE_JVS_vftable MICE_JVS_vftable;

struct MICE_JVS_vftable {
    void (*ctor)(PMICE_JVS this);
    JVS_STATUS (*transact)(PMICE_JVS this, BYTE command);
    void (*readAllButtons)(PMICE_JVS this);
};

extern MICE_JVS_vftable _MiceJvsBase_vftable;
extern MICE_JVS_vftable _MiceJvs837_14572_vftable;
extern MICE_JVS_vftable _MiceJvs837_14895_vftable;

struct MICE_JVS {
    MICE_JVS_vftable* vftable;

    LPBYTE m_lpInData;
    DWORD m_nInIdx;
    LPBYTE m_lpOutData;
    DWORD m_nOutIdx;

    BYTE m_Address;

    LPCSTR m_BoardId;
    BYTE m_CommandVersion;
    BYTE m_JvsRevision;
    BYTE m_VersionComm;

    BOOL m_SenseOut;

    BYTE m_Players;
    BYTE m_ButtonsPerPlayer;
    BYTE m_Coins;
    BYTE m_NumButtons;
    LPBOOL m_ButtonStates;
    PMICE_BUTTON_BINDING m_Bindings;

    DWORD m_CoinCounts[2];
};
#pragma pack(push, 1)
typedef struct JVS_PACKET {
    BYTE m_Sync;
    BYTE m_Dst;
    BYTE m_nBytes;
    BYTE m_Data[MICE_JVS_MAX - 3];
} JVS_PACKET, *PJVS_PACKET;
#pragma pack(pop)

static inline BYTE MiceJvsRead(PMICE_JVS this) {
    if (this->m_lpInData == NULL) return 0xff;
    if (this->m_nInIdx < MICE_JVS_MAX) return this->m_lpInData[this->m_nInIdx++];
    return 0xff;
}

#include <stdio.h>
static inline void MiceJvsWrite(PMICE_JVS this, BYTE data) {
    if (this->m_lpOutData == NULL) return;
    if (this->m_nOutIdx < MICE_JVS_MAX) this->m_lpOutData[this->m_nOutIdx++] = data;
}
