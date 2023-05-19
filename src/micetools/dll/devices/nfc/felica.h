#include <Windows.h>

#include "../_devices.h"

typedef struct FELICA_ID {
    BYTE m_ID[8];
} FELICA_ID, *PFELICA_ID;

enum FelicaCommand {
    FelicaCommand_Poll = 0x00,
    FelicaCommand_Poll_Response = 0x01,
    FelicaCommand_RequestService = 0x02,
    FelicaCommand_RequestService_Response = 0x03,
    FelicaCommand_RequestResponse = 0x04,
    FelicaCommand_RequestResponse_Response = 0x05,
    FelicaCommand_ReadNoEnc = 0x06,
    FelicaCommand_ReadNoEnc_Response = 0x07,
    FelicaCommand_WriteNoEnc = 0x08,
    FelicaCommand_WriteNoEnc_Response = 0x09,
    FelicaCommand_Search = 0x0a,
    FelicaCommand_Search_Response = 0x0b,
    FelicaCommand_RequestSystemCode = 0x0c,
    FelicaCommand_RequestSystemCode_Response = 0x0d,
    FelicaCommand_Authentication1 = 0x10,
    FelicaCommand_Authentication1_Response = 0x11,
    FelicaCommand_Authentication2 = 0x12,
    FelicaCommand_Authentication2_Response = 0x13,
    FelicaCommand_Read = 0x14,
    FelicaCommand_Read_Response = 0x15,
    FelicaCommand_Write = 0x16,
    FelicaCommand_Write_Response = 0x17,

    FelicaCommand_GetNodeProperty = 0x28,
    FelicaCommand_GetNodeProperty_Response = 0x29,

    FelicaCommand_RequestServiceV2 = 0x32,
    FelicaCommand_RequestServiceV2_Response = 0x33,
    FelicaCommand_InternalAuthAndRead = 0x34,
    FelicaCommand_InternalAuthAndRead_Response = 0x35,
    FelicaCommand_ExternalAuthAndRead = 0x36,
    FelicaCommand_ExternalAuthAndRead_Response = 0x37,
    FelicaCommand_GetSystemStatus = 0x38,
    FelicaCommand_GetSystemStatus_Response = 0x39,
    FelicaCommand_RequestSpecVersion = 0x3c,
    FelicaCommand_RequestSpecVersion_Response = 0x3d,
    FelicaCommand_ResetMode = 0x3e,
    FelicaCommand_ResetMode_Response = 0x3f,
    FelicaCommand_Authentication1V2 = 0x40,
    FelicaCommand_Authentication1V2_Response = 0x41,
    FelicaCommand_Authentication2V2 = 0x42,
    FelicaCommand_Authentication2V2_Response = 0x43,
    FelicaCommand_ReadV2 = 0x44,
    FelicaCommand_ReadV2_Response = 0x45,
    FelicaCommand_WriteV2 = 0x46,
    FelicaCommand_WriteV2_Response = 0x47,

    FelicaCommand_UpdateRandomID = 0x4C,
    FelicaCommand_UpdateRandomID_Response = 0x4D,

    FelicaCommand_SetNodeProperty = 0x78,
    FelicaCommand_SetNodeProperty_Response = 0x79,

    FelicaCommand_MobileActive2 = 0xa4,
    FelicaCommand_MobileActive2_Response = 0xa5,
};
typedef BYTE FelicaCommand;

enum FelicaSystem {
    FelicaSystem_Any = 0xffff,

    FelicaSystem_NDEF = 0x12fc,
    FelicaSystem_Emulator = 0x4000,
    FelicaSystem_FelicaLite = 0x88b4,
    FelicaSystem_FelicaSecureID = 0x957a,
    FelicaSystem_JISX_Start = 0xaaa0,
    FelicaSystem_JISX_End = 0xaafe,
    FelicaSystem_CommonArea = 0xfe00,
    FelicaSystem_FelicaPlug = 0xfee1,

    FelicaSystem_Edy = 0xfe00,
    FelicaSystem_Cyberne = 0x0003,
    FelicaSystem_Suica = 0x0003,
    FelicaSystem_Pasmo = 0x0003,
};
typedef WORD FelicaSystem;

enum FelicaService {
    FelicaService_SuicaInOut = 0x108f,
    FelicaService_SuicaHistory = 0x090f,
    FelicaService_FelicaLiteRO = 0x0b00,
    FelicaService_FelicaLiteEW = 0x0900,
};
typedef WORD FelicaService;

enum FelicaPollRequest {
    FelicaPollRequest_NoRequest = 0x00,
    FelicaPollRequest_SystemCode = 0x01,
    FelicaPollRequest_CommPerformance = 0x02,
};
typedef BYTE FelicaPollRequest;

enum FelicaStatus1 {
    FelicaStatus1_Success = 0x00,
    // Everything between here is dynamically generated, and I cba
    FelicaStatus1_Fatal = 0xFF,
};
typedef BYTE FelicaStatus1;

enum FelicaStatus2 {
    // Generic
    FelicaStatus2_Success = 0x00,
    FelicaStatus2_Overflow = 0x01,
    FelicaStatus2_CashbackExceeded = 0x02,
    FelicaStatus2_PurseOutOfRange = 0x03,
    FelicaStatus2_MemoryError = 0x70,
    FelicaStatus2_RewritesExceeded = 0x71,
    // Card specific
    FelicaStatus2_IllegalServiceNumber = 0xA1,
    FelicaStatus2_IllegalCommandPacket = 0xA2,
    FelicaStatus2_IllegalBlockList = 0xA3,
    FelicaStatus2_IllegalService = 0xA4,
    FelicaStatus2_AccessDenied = 0xA5,
    FelicaStatus2_IllegalServiceCode = 0xA6,
    FelicaStatus2_IllegalBlockListAccess = 0xA7,
    FelicaStatus2_IllegalBlockNumber = 0xA8,
    FelicaStatus2_DataWriteFailure = 0xA9,
    FelicaStatus2_KeyChangeFailure = 0xAA,
    FelicaStatus2_IllegalParity = 0xAB,
    FelicaStatus2_IllegalParameter = 0xAC,
    FelicaStatus2_ServiceExists = 0xAD,
    FelicaStatus2_IllegalSystemCode = 0xAE,
    FelicaStatus2_TooManyWrites = 0xAF,
    FelicaStatus2_IllegalPackage = 0xC0,
    FelicaStatus2_PackageDiscrepancy = 0xC1,
    FelicaStatus2_CommandDisabled = 0xC2,
    FelicaStatus2_IllegalNodeProperty = 0xC3,
};
typedef BYTE FelicaStatus2;

// https://www.sony.net/Products/felica/business/tech-support/list.html
enum FelicaChipType {
    FelicaChipType_STD_RC_S915 = 0x01,
    FelicaChipType_STD_RC_S952 = 0x08,
    FelicaChipType_STD_RC_S953 = 0x09,
    FelicaChipType_STD_RC_S960 = 0x0D,
    FelicaChipType_STD_RC_S962 = 0x20,
    FelicaChipType_STD_RC_SA00_1 = 0x32,
    FelicaChipType_STD_RC_SA20_1 = 0x44,
    FelicaChipType_STD_RC_SA20_2 = 0x45,

    FelicaChipType_Mobile_Ver10 = 0x06,  // And 07h
    FelicaChipType_Mobile_Ver20 = 0x10,  // Up to 13h
    FelicaChipType_Mobile_Ver30 = 0x14,  // And 15h
    FelicaChipType_Mobile_Ver40 = 0x17,
    FelicaChipType_Mobile_Ver41 = 0x18,  // Up to 1Fh

    FelicaChipType_Lite = 0xf0,   // RC-S965
    FelicaChipType_LiteS = 0xf1,  // RC-S966

    FelicaChipType_Link_LiteS = 0xf2,    // RC-S967
    FelicaChipType_Link_Plug = 0xe1,     // RC-S967
    FelicaChipType_Link_NFC_DEP = 0xff,  // RC-S967

    FelicaChipType_Plug = 0xe0,  // RC-S926
};
typedef BYTE FelicaChipType;

/**
 * Multiple vendors make use of FeliCa cards for their amusement cards, but we
 * can disamiguate them based on the specific FeliCa chip they use.
 *
 * Mobile phones will be using the Mobile chip versions. The specific OS
 * version only matters for when the game is requesting to send mail or open a
 * URL in the browser.
 *
 * The now-common Amusement IC cards all use FeliCa Lite or FeliCa LiteS. We
 * choose to emulate our AIC cards as LiteS (RC-S966).
 *
 * Banapasses (BNG) also use FeliCa, however use FeliCa standard. We're going
 * to keep things simple and just emulate RC-S915.
 */
typedef enum FelicaEmulationMode {
    // Emulate a Suica mobile phone (FelicaChipType_Mobile_Ver**)
    FelicaEmulationMode_Suica,  // TODO: Which OS version do we want to emulate?
    // Emulate an AmusementIC card (FelicaChipType_LiteS)
    FelicaEmulationMode_AIC,
    // Emulate a Banapass card (FelicaChipType_STD_RC_S915)
    FelicaEmulationMode_BNG,
} FelicaEmulationMode;

typedef enum FelicaAICVendor {
    FelicaAICVendor_BandaiNamcoGames,          // FeliCa Lite, never used afaik
    FelicaAICVendor_BandaiNamcoEntertainment,  // FeliCa LiteS
    FelicaAICVendor_SEGA,                      // FeliCa LiteS
    FelicaAICVendor_Konami,                    // FeliCa LiteS
} FelicaAICVendor;

#define FelicaSystemBlock_RC 0x00
#define FelicaSystemBlock_MAC 0x01
#define FelicaSystemBlock_ID 0x02
#define FelicaSystemBlock_D_ID 0x03
#define FelicaSystemBlock_SER_C 0x04
#define FelicaSystemBlock_SYS_C 0x05
#define FelicaSystemBlock_CKV 0x06
#define FelicaSystemBlock_CK 0x07
#define FelicaSystemBlock_MC 0x08

#pragma pack(push, 1)
typedef struct FelicaBlock {
    BYTE bytes[16];
} FelicaBlock_t, *PFelicaBlock_t;
typedef struct FelicaBlock_RC {
    union {
        struct {
            DWORD64 m_RC1;
            DWORD64 m_RC2;
        };
        DWORD64 m_RC[2];
    };
} FelicaBlock_RC;
typedef struct FelicaBlock_MAC {
    BYTE m_MAC[8];
    BYTE Rsv08[8];
} FelicaBlock_MAC;
typedef struct FelicaBlock_ID {
    FELICA_ID m_FelicaId;
    BYTE m_DFC[2];
    BYTE m_Arbitrary[6];
} FelicaBlock_ID;
typedef struct FelicaBlock_D_ID {
    FELICA_ID m_FelicaId;
    BYTE m_PMm[8];
} FelicaBlock_D_ID;
typedef struct FelicaBlock_SER_C {
    BYTE m_SER_C[2];
    BYTE Rsv02[14];
} FelicaBlock_SER_C;
typedef struct FelicaBlock_SYS_C {
    WORD m_SYS_C;
    BYTE Rsv02[14];
} FelicaBlock_SYS_C;
typedef struct FelicaBlock_CKV {
    BYTE m_CKV[2];
    BYTE Rsv02[14];
} FelicaBlock_CKV;
typedef struct FelicaBlock_CK {
    BYTE m_CK1[8];
    BYTE m_CK2[8];
} FelicaBlock_CK;
typedef struct FelicaBlock_MC {
    // I really don't want to map this all out, and don't think I need to.
    // So I'm not going to.
    // Page 30 on the below URL if ever actually needed.
    // https://www.sony.net/Products/felica/business/tech-support/data/fls_usmnl_1.31.pdf
    BYTE Rsv00[16];
} FelicaBlock_MC;

typedef struct FelicaMemory {
    FelicaEmulationMode m_EmuMode;
    FelicaAICVendor m_AICVendor;
    FelicaBlock_t m_UserBlocks[15];
    union {
        FelicaBlock_t m_SystemBlocks[9];
        struct {
            FelicaBlock_RC m_SystemBlock_RC;
            FelicaBlock_MAC m_SystemBlock_MAC;
            FelicaBlock_ID m_SystemBlock_ID;
            FelicaBlock_D_ID m_SystemBlock_D_ID;
            FelicaBlock_SER_C m_SystemBlock_SER_C;
            FelicaBlock_SYS_C m_SystemBlock_SYS_C;
            FelicaBlock_CKV m_SystemBlock_CKV;
            FelicaBlock_CK m_SystemBlock_CK;
            FelicaBlock_MC m_SystemBlock_MC;
        };
    };
} FelicaMemory_t;

typedef struct FelicaRequest_Poll {
    FelicaSystem m_SystemCode;
    FelicaPollRequest m_Request;
    BYTE m_TimeSlot;
} FelicaRequest_Poll, *PFelicaRequest_Poll;
typedef struct FelicaResponse_Poll {
    BYTE m_PMm[8];
    WORD m_Value;
} FelicaResponse_Poll, *PFelicaResponse_Poll;

typedef struct FelicaRequest_ReadNoEnc {
    FELICA_ID m_IDm;
    BYTE m_Rest[];
} FelicaRequest_ReadNoEnc, *PFelicaRequest_ReadNoEnc;
typedef struct FelicaResponse_ReadNoEnc {
    BYTE m_Status1;
    BYTE m_Status2;
    BYTE m_BlockCount;
    BYTE m_BlockData[];
} FelicaResponse_ReadNoEnc, *PFelicaResponse_ReadNoEnc;

typedef struct FelicaRequest_WriteNoEnc {
    FELICA_ID m_IDm;
    BYTE m_Rest[];
} FelicaRequest_WriteNoEnc, *PFelicaRequest_WriteNoEnc;
typedef struct FelicaResponse_WriteNoEnc {
    BYTE m_Status1;
    BYTE m_Status2;
} FelicaResponse_WriteNoEnc, *PFelicaResponse_WriteNoEnc;

typedef struct FelicaRequest_SystemCode {
    FELICA_ID m_IDm;
} FelicaRequest_SystemCode, *PFelicaRequest_SystemCode;
typedef struct FelicaResponse_SystemCode {
    BYTE m_SystemsCount;
    WORD m_SystemCode;
} FelicaResponse_SystemCode, *PFelicaResponse_SystemCode;

typedef struct FelicaRequest_Active2 {
    FELICA_ID m_IDm;
    BYTE m_Val;
} FelicaRequest_Active2, *PFelicaRequest_Active2;
typedef struct FelicaResponse_Active2 {
    BYTE m_Data;
} FelicaResponse_Active2, *PFelicaResponse_Active2;

typedef struct FelicaEncapRequest {
    FELICA_ID m_IDm;
    BYTE m_FelicaLen;
    BYTE m_FelicaCommand;
    union {
        FelicaRequest_Poll m_Poll;
        FelicaRequest_ReadNoEnc m_ReadNoEnc;
        FelicaRequest_WriteNoEnc m_WriteNoEnc;
        FelicaRequest_SystemCode m_SystemCode;
        FelicaRequest_Active2 m_Active2;
        BYTE m_Body[238];
    };
} FelicaEncapRequest, *PFelicaEncapRequest;
typedef struct FelicaEncapResponse {
    BYTE m_FelicaLen;
    BYTE m_FelicaCommand;
    FELICA_ID m_IDm;
    union {
        FelicaResponse_Poll m_Poll;
        FelicaResponse_ReadNoEnc m_ReadNoEnc;
        FelicaResponse_WriteNoEnc m_WriteNoEnc;
        FelicaResponse_SystemCode m_SystemCode;
        FelicaResponse_Active2 m_Active2;
        BYTE m_Body[246];
    };
} FelicaEncapResponse, *PFelicaEncapResponse;
#pragma pack(pop)

#define FelicaRespSize(x) (10 + sizeof x)

#ifndef _NFC_H
typedef struct NFC_CARD NFC_CARD, *PNFC_CARD;
typedef struct ACCESS_CODE ACCESS_CODE, *PACCESS_CODE;
#define _NFC_H
#endif

void FelicaPoll(PFelicaEncapRequest lpReq, PFelicaEncapResponse lpResponse, PNFC_CARD lpCard);
void FelicaReadNoEnc(PFelicaEncapRequest lpReq, PFelicaEncapResponse lpResponse, PNFC_CARD lpCard);
void FelicaWriteNoEnc(PFelicaEncapRequest lpReq, PFelicaEncapResponse lpResponse, PNFC_CARD lpCard);
void FelicaSystemCode(PFelicaEncapRequest lpReq, PFelicaEncapResponse lpResponse, PNFC_CARD lpCard);
void FelicaEncap(com_device_t* dev, comio_recv_head_t* req, LPBYTE lpPacket, PNFC_CARD lpCard);
void FelicaPopulate(PNFC_CARD lpCard, PFELICA_ID lpFelicaId, PACCESS_CODE lpAccessCode);
