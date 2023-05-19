#pragma once
#include <Windows.h>

#include "felica.h"
#include "mifare.h"

enum CardType {
    CardType_Unknown = 0x00,
    CardType_Mifare = 0x10,
    CardType_FeliCa = 0x20,
};
typedef BYTE CardType;

struct ACCESS_CODE {
    BYTE m_AC[10];
};

#ifndef _NFC_H
typedef struct NFC_CARD NFC_CARD, *PNFC_CARD;
typedef struct ACCESS_CODE ACCESS_CODE, *PACCESS_CODE;
#define _NFC_H
#endif

#pragma pack(push, 1)
struct NFC_CARD {
    struct {
        BYTE m_Type;
        BYTE m_IdLen;
    } m_Header;
    union {
        struct {
            union {
                FELICA_ID m_IDm;
                struct {
                    WORD m_Code;     // Manufacturer code
                    WORD m_Machine;  // Manufacturer machine
                    INT16 m_Date;    // Days since 2000-01-01
                    WORD m_Serial;   // Daily serial
                };
            };
            union {
                BYTE m_PMm[8];
                struct {
                    BYTE m_RomType;
                    FelicaChipType m_ICType;
                    BYTE m_Response[6];  // Response time information
                };
            };
        } m_FeliCa;
        struct {
            DWORD m_Uid;
        } m_Mifare;
        struct {
            BYTE Unk00[7];
        } m_Amiibo;
    };
    union {
        MifareMemory_t m_MifareData;
        FelicaMemory_t m_FeliCaData;
    };
};
#pragma pack(pop)

#define NFC_FeliCaSize (sizeof((PNFC_CARD)0)->m_Header + sizeof((PNFC_CARD)0)->m_FeliCa)
#define NFC_MifareSize (sizeof((PNFC_CARD)0)->m_Header + sizeof((PNFC_CARD)0)->m_Mifare)
