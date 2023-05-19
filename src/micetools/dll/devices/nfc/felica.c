#include "../../../lib/mice/spad.h"
#include "../_devices.h"
#include "../ser_tn32msec.h"
#include "bng.h"
#include "nfc.h"

static const BYTE PMmTiming[6] = { 0x00, 0x00, 0x00, 0x01, 0x43, 0x00 };
#define DFC_BNG_EDY_0 0x8C
#define DFC_BNG_EDY_1 0xA2
#define DFC_BNG_LITE_0 0x00
#define DFC_BNG_LITE_1 0x3A
#define DFC_KNM_LITES_0 0x00
#define DFC_KNM_LITES_1 0x68
#define DFC_BNE_LITES_0 0x00
#define DFC_BNE_LITES_1 0x2A
#define DFC_SEGA_LITES_0 0x00
#define DFC_SEGA_LITES_1 0x78
#define DFC_TEST_0 0x00
#define DFC_TEST_1 0x00
static const BYTE ID_Arbitrary[6] = { 0x05, 0x73, 0x02, 0x01, 0x02, 0x00 };

inline static void _FelicaDumpChars(LPBYTE lpBytes, BYTE nBytes) {
    printf("|");
    for (int i = 0; i < nBytes; i++) {
        BYTE chr = lpBytes[i];
        if (' ' < chr && chr <= '~')
            printf("%c", chr);
        else
            printf(".");
    }
    printf("|");
}

static void _FelicaDumpBlock(PNFC_CARD lpCard, BYTE block, BYTE nBytes, LPCSTR lpName) {
    PFelicaBlock_t lpBlock;
    if (block < 0xf)
        lpBlock = &lpCard->m_FeliCaData.m_UserBlocks[block];
    else
        lpBlock = &lpCard->m_FeliCaData.m_SystemBlocks[block - 0x80];

    printf("[%02X] ", block);
    for (int i = 0; i < 8; i++) {
        if (i < nBytes)
            printf("%02X ", lpBlock->bytes[i]);
        else
            printf("-- ");
    }
    if (lpName)
        printf("(%s)", lpName);
    else
        _FelicaDumpChars(lpBlock->bytes, 8);
    printf("\n     ");

    for (int i = 8; i < 16; i++) {
        if (i < nBytes)
            printf("%02X ", lpBlock->bytes[i]);
        else
            printf("-- ");
    }
    if (!lpName) _FelicaDumpChars(&(lpBlock->bytes[8]), 8);
    printf("\n");
}

void FelicaDumpCard(PNFC_CARD lpCard) {
    for (BYTE block = 0; block < 15; block++) {
        _FelicaDumpBlock(lpCard, block, 16, NULL);
    }
    _FelicaDumpBlock(lpCard, 0x80, 16, "RC");
    _FelicaDumpBlock(lpCard, 0x81, 8, "MAC");
    _FelicaDumpBlock(lpCard, 0x82, 16, "ID");
    _FelicaDumpBlock(lpCard, 0x83, 16, "D_ID");
    _FelicaDumpBlock(lpCard, 0x84, 2, "SER_C");
    _FelicaDumpBlock(lpCard, 0x85, 2, "SYS_C");
    _FelicaDumpBlock(lpCard, 0x86, 2, "CKV");
    _FelicaDumpBlock(lpCard, 0x87, 16, "CK");
    _FelicaDumpBlock(lpCard, 0x88, 13, "MC");
}

void FelicaPopulate(PNFC_CARD lpCard, PFELICA_ID lpFelicaId, PACCESS_CODE lpAccessCode) {
    ZeroMemory(&lpCard->m_FeliCaData, sizeof lpCard->m_FeliCaData);

    lpCard->m_Header.m_Type = CardType_FeliCa;
    lpCard->m_Header.m_IdLen = sizeof(lpCard->m_FeliCa);
    memcpy(&lpCard->m_FeliCa.m_IDm, lpFelicaId, sizeof *lpFelicaId);

    // TODO: Toggle between suica and AIC mode.
    lpCard->m_FeliCaData.m_EmuMode = FelicaEmulationMode_AIC;
    lpCard->m_FeliCaData.m_AICVendor = FelicaAICVendor_Konami;

    lpCard->m_FeliCa.m_PMm[0] = 0x00;
    switch (lpCard->m_FeliCaData.m_EmuMode) {
        case FelicaEmulationMode_Suica:
            lpCard->m_FeliCa.m_PMm[1] = FelicaChipType_Mobile_Ver10;
            lpCard->m_FeliCaData.m_SystemBlock_SYS_C.m_SYS_C = _byteswap_ushort(FelicaSystem_Suica);

            // DFC shouldn't matter for Suica
            lpCard->m_FeliCaData.m_SystemBlock_ID.m_DFC[0] = DFC_TEST_0;
            lpCard->m_FeliCaData.m_SystemBlock_ID.m_DFC[1] = DFC_TEST_1;
            break;
        case FelicaEmulationMode_AIC:
            lpCard->m_FeliCaData.m_SystemBlock_SYS_C.m_SYS_C =
                _byteswap_ushort(FelicaSystem_FelicaLite);

            /**
             * The DFC code is used to identify the card manufacturer. Each
             * manufacturer uses a different derivation algorithm for FeliCa ID
             * to Access Code conversion, which AiMeDB is able to validate
             * based on this information.
             *
             * This validation is performed as part of
             * AiMe_DB_com_get_AiMeAccountS, added in AiMeDB version 4.0.0
             *
             * To the best of my knowledge this was added long after RingEdge
             * was a dead platform, so isn't something we will need to concern
             * ourselves with here.
             *
             * Maimai itself only does a check for if this is
             * FelicaAICVendor_BandaiNamco_LiteS, in which case special
             * handling is required.
             *
             * TODO: Right now things seem to test bad if it's _not_ a BNE card??
             */
            switch (lpCard->m_FeliCaData.m_AICVendor) {
                case FelicaAICVendor_BandaiNamcoGames:
                    lpCard->m_FeliCaData.m_SystemBlock_ID.m_DFC[0] = DFC_BNG_LITE_0;
                    lpCard->m_FeliCaData.m_SystemBlock_ID.m_DFC[1] = DFC_BNG_LITE_1;
                    break;
                case FelicaAICVendor_BandaiNamcoEntertainment: {
                    lpCard->m_FeliCaData.m_SystemBlock_ID.m_DFC[0] = DFC_BNE_LITES_0;
                    lpCard->m_FeliCaData.m_SystemBlock_ID.m_DFC[1] = DFC_BNE_LITES_1;

                    BYTE block0D[8] = {
                        // TODO: Work backwards and figure out where the key is coming from
                        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, /*sum*/ 0x00,
                    };
                    NbgiWriteBNESignedBlock(lpCard, 0x0D, 0, block0D);

                    break;
                }
                case FelicaAICVendor_SEGA:
                    lpCard->m_FeliCaData.m_SystemBlock_ID.m_DFC[0] = DFC_SEGA_LITES_0;
                    lpCard->m_FeliCaData.m_SystemBlock_ID.m_DFC[1] = DFC_SEGA_LITES_1;
                    break;
                case FelicaAICVendor_Konami:
                    lpCard->m_FeliCaData.m_SystemBlock_ID.m_DFC[0] = DFC_KNM_LITES_0;
                    lpCard->m_FeliCaData.m_SystemBlock_ID.m_DFC[1] = DFC_KNM_LITES_1;
                    break;
                default:
                    lpCard->m_FeliCaData.m_SystemBlock_ID.m_DFC[0] = DFC_TEST_0;
                    lpCard->m_FeliCaData.m_SystemBlock_ID.m_DFC[1] = DFC_TEST_1;
                    break;
            }

            // Write the card IC type into the PMm
            switch (lpCard->m_FeliCaData.m_AICVendor) {
                case FelicaAICVendor_BandaiNamcoGames:
                    lpCard->m_FeliCa.m_PMm[1] = FelicaChipType_Lite;
                    break;
                case FelicaAICVendor_BandaiNamcoEntertainment:
                case FelicaAICVendor_SEGA:
                case FelicaAICVendor_Konami:
                default:
                    lpCard->m_FeliCa.m_PMm[1] = FelicaChipType_LiteS;
                    break;
            }
            break;
        case FelicaEmulationMode_BNG:
        default:
            lpCard->m_FeliCa.m_PMm[1] = FelicaChipType_STD_RC_S915;
            lpCard->m_FeliCaData.m_SystemBlock_SYS_C.m_SYS_C = _byteswap_ushort(FelicaSystem_NDEF);
            lpCard->m_FeliCaData.m_SystemBlock_ID.m_DFC[0] = DFC_BNG_EDY_0;
            lpCard->m_FeliCaData.m_SystemBlock_ID.m_DFC[1] = DFC_BNG_EDY_1;
            break;
    }
    memcpy(&(lpCard->m_FeliCa.m_PMm[2]), PMmTiming, sizeof PMmTiming);

    if (lpAccessCode) {
        BYTE block0[16];
        memcpy(&(block0[6]), lpAccessCode->m_AC, 10);
        Spad0Encrypt(block0, block0);
        memcpy(lpCard->m_FeliCaData.m_UserBlocks[0].bytes, block0, 16);
    }

    memset(lpCard->m_FeliCaData.m_UserBlocks[0x0e].bytes, 0xFF, 16);

    memcpy(&lpCard->m_FeliCaData.m_SystemBlock_ID.m_FelicaId, lpFelicaId, sizeof *lpFelicaId);
    memcpy(lpCard->m_FeliCaData.m_SystemBlock_ID.m_Arbitrary, ID_Arbitrary, sizeof ID_Arbitrary);

    memcpy(&lpCard->m_FeliCaData.m_SystemBlock_D_ID.m_FelicaId, lpFelicaId, sizeof *lpFelicaId);
    memcpy(lpCard->m_FeliCaData.m_SystemBlock_D_ID.m_PMm, lpCard->m_FeliCa.m_PMm,
           sizeof lpCard->m_FeliCa.m_PMm);

#if 0
    FelicaDumpCard(lpCard);
#endif
}

void FelicaPoll(PFelicaEncapRequest lpReq, PFelicaEncapResponse lpResponse, PNFC_CARD lpCard) {
    FelicaPollRequest request = lpReq->m_Poll.m_Request;

    lpResponse->m_FelicaLen = FelicaRespSize(lpResponse->m_Poll);
    lpResponse->m_FelicaCommand = FelicaCommand_Poll_Response;
    memcpy(&lpResponse->m_IDm, &lpReq->m_IDm, sizeof lpReq->m_IDm);
    memcpy(lpResponse->m_Poll.m_PMm, lpCard->m_FeliCa.m_PMm, sizeof lpCard->m_FeliCa.m_PMm);

    switch (request) {
        case FelicaPollRequest_SystemCode:
            lpResponse->m_Poll.m_Value = lpCard->m_FeliCaData.m_SystemBlock_SYS_C.m_SYS_C;
            break;
        default:
            log_error(plfAime, "Unhnaldled poll request: %02x", request);
            lpResponse->m_FelicaLen -= 2;
            break;
    }
}

void FelicaReadNoEnc(PFelicaEncapRequest lpReq, PFelicaEncapResponse lpResponse, PNFC_CARD lpCard) {
    BYTE readIndex = 0;
    BYTE serviceN = lpReq->m_ReadNoEnc.m_Rest[readIndex++];
    for (BYTE i = 0; i < serviceN; i++) {
        WORD service = *(LPWORD)(&lpReq->m_ReadNoEnc.m_Rest[readIndex++]);
        readIndex++;
    }
    BYTE blockCount = lpReq->m_ReadNoEnc.m_Rest[readIndex++];

    lpResponse->m_FelicaLen = FelicaRespSize(lpResponse->m_ReadNoEnc) + 16 * blockCount;
    lpResponse->m_FelicaCommand = FelicaCommand_ReadNoEnc_Response;
    memcpy(&lpResponse->m_IDm, &lpReq->m_ReadNoEnc.m_IDm, sizeof lpReq->m_ReadNoEnc.m_IDm);
    lpResponse->m_ReadNoEnc.m_Status1 = FelicaStatus1_Success;
    lpResponse->m_ReadNoEnc.m_Status2 = FelicaStatus2_Success;
    lpResponse->m_ReadNoEnc.m_BlockCount = blockCount;
    BYTE writeIndex = 0;

    for (BYTE i = 0; i < blockCount; i++) {
        BYTE blHeader = lpReq->m_ReadNoEnc.m_Rest[readIndex++];
        BOOL isExtended = !(blHeader & 0x80);
        BYTE serviceCodeOrder = blHeader & 0x0f;
        BYTE accessMode = (blHeader & 0x70) >> 4;

        WORD blockNumber;
        if (isExtended) {
            blockNumber = *(LPWORD)(&lpReq->m_ReadNoEnc.m_Rest[readIndex++]);
            readIndex++;
        } else {
            blockNumber = lpReq->m_ReadNoEnc.m_Rest[readIndex++];
        }

        if (blockNumber < 0xf) {
            memcpy(&lpResponse->m_ReadNoEnc.m_BlockData[writeIndex],
                   lpCard->m_FeliCaData.m_UserBlocks[blockNumber].bytes, 16);

            log_warning(plfAime, "Felica read: s:%x block:%04x", serviceCodeOrder, blockNumber);
            writeIndex += 16;
        } else if (blockNumber >= 0x80 && blockNumber < 0x89) {
            if (blockNumber - 0x80 == FelicaSystemBlock_MAC) {
                log_info(plfAime, "MAC challenge requested");
                DWORD64 ID[2] = { 0 };
                memcpy(ID, &lpCard->m_FeliCaData.m_SystemBlock_ID, 16);

                KEY_PAIR CK, SK;
                DWORD64* RC = (DWORD64*)&lpCard->m_FeliCaData.m_SystemBlock_RC.m_RC;
                FelicaMacGenerateCK(ID, CK);
                FelicaMacGenerateSK(RC, CK, SK);
                DWORD64 mac = FelicaMacGenerateMAC(ID, SK, RC);

                mac = _byteswap_uint64(mac);

                memcpy(&lpResponse->m_ReadNoEnc.m_BlockData[writeIndex], &mac, 8);
                memset(&lpResponse->m_ReadNoEnc.m_BlockData[writeIndex + 8], 0, 8);
                log_warning(plfAime, "Felica read: s:%x block:%04x", serviceCodeOrder, blockNumber);
                writeIndex += 16;
            } else {
                memcpy(&lpResponse->m_ReadNoEnc.m_BlockData[writeIndex],
                       lpCard->m_FeliCaData.m_SystemBlocks[blockNumber - 0x80].bytes, 16);
                log_warning(plfAime, "Felica read: s:%x block:%04x", serviceCodeOrder, blockNumber);
                writeIndex += 16;
            }
        } else {
            // Invalid block, but we're already committed to the success state because I'm lazy,
            // so just write a bunch of nulls.
            memset(&lpResponse->m_ReadNoEnc.m_BlockData[writeIndex], 0, 16);
            log_warning(plfAime, "Felica read: s:%x block:%04x", serviceCodeOrder, blockNumber);
            writeIndex += 16;
        }
    }
}
void FelicaWriteNoEnc(PFelicaEncapRequest lpReq, PFelicaEncapResponse lpResponse,
                      PNFC_CARD lpCard) {
    BYTE readIndex = 0;
    BYTE serviceN = lpReq->m_WriteNoEnc.m_Rest[readIndex++];
    for (BYTE i = 0; i < serviceN; i++) {
        WORD service = *(LPWORD)(&lpReq->m_WriteNoEnc.m_Rest[readIndex++]);
        readIndex++;
    }
    BYTE blockCount = lpReq->m_WriteNoEnc.m_Rest[readIndex++];

    for (BYTE i = 0; i < blockCount; i++) {
        BYTE blHeader = lpReq->m_WriteNoEnc.m_Rest[readIndex++];
        BOOL isExtended = !(blHeader & 0x80);
        BYTE serviceCodeOrder = blHeader & 0x0f;
        BYTE accessMode = (blHeader & 0x70) >> 4;

        WORD blockNumber;
        if (isExtended) {
            blockNumber = *(LPWORD)(&lpReq->m_WriteNoEnc.m_Rest[readIndex++]);
            readIndex++;
        } else {
            blockNumber = lpReq->m_WriteNoEnc.m_Rest[readIndex++];
        }

        log_warning(plfAime, "Felica write: s:%x block:%04x", serviceCodeOrder, blockNumber);

        // TODO: Access controls!
        if (blockNumber < 0xf) {
            memcpy(lpCard->m_FeliCaData.m_UserBlocks[blockNumber].bytes,
                   &lpReq->m_WriteNoEnc.m_Rest[readIndex], 16);
            readIndex += 16;
        } else if (blockNumber >= 0x80 && blockNumber < 0x89) {
            memcpy(lpCard->m_FeliCaData.m_SystemBlocks[blockNumber - 0x80].bytes,
                   &lpReq->m_WriteNoEnc.m_Rest[readIndex], 16);
            readIndex += 16;
        } else {
            // Invalid block, but I'd rather just steamroll through it for the sake of
            // compatability, so don't error.
            readIndex += 16;
        }
    }

    lpResponse->m_FelicaLen = FelicaRespSize(lpResponse->m_WriteNoEnc);
    lpResponse->m_FelicaCommand = FelicaCommand_WriteNoEnc_Response;
    memcpy(&lpResponse->m_IDm, &lpReq->m_WriteNoEnc.m_IDm, sizeof lpReq->m_WriteNoEnc.m_IDm);
    lpResponse->m_WriteNoEnc.m_Status1 = FelicaStatus1_Success;
    lpResponse->m_WriteNoEnc.m_Status2 = FelicaStatus2_Success;
}
void FelicaSystemCode(PFelicaEncapRequest lpReq, PFelicaEncapResponse lpResponse,
                      PNFC_CARD lpCard) {
    lpResponse->m_FelicaLen = FelicaRespSize(lpResponse->m_SystemCode);
    lpResponse->m_FelicaCommand = FelicaCommand_RequestSystemCode_Response;
    memcpy(&lpResponse->m_IDm, &lpReq->m_SystemCode.m_IDm, sizeof lpReq->m_SystemCode.m_IDm);
    lpResponse->m_SystemCode.m_SystemsCount = 1;
    lpResponse->m_SystemCode.m_SystemCode = lpCard->m_FeliCaData.m_SystemBlock_SYS_C.m_SYS_C;
}
void FelicaMobileActive2(PFelicaEncapRequest lpReq, PFelicaEncapResponse lpResponse,
                         PNFC_CARD lpCard) {
    lpResponse->m_FelicaLen = FelicaRespSize(lpResponse->m_Active2);
    lpResponse->m_FelicaCommand = FelicaCommand_MobileActive2_Response;

    switch (lpReq->m_Active2.m_Val) {
        case 0:  // Alive check
            lpResponse->m_Active2.m_Data = 0;
            break;
        case 1:  // OS Version
            switch (lpCard->m_FeliCa.m_PMm[1]) {
                case FelicaChipType_Mobile_Ver10:
                    lpResponse->m_Active2.m_Data = 1;
                    break;
                case FelicaChipType_Mobile_Ver20:
                    lpResponse->m_Active2.m_Data = 0;
                    break;
                default:
                    // I have no idea what other chips return
                    lpResponse->m_Active2.m_Data = 0;
                    break;
            }
            break;
        case 2:
            lpResponse->m_Active2.m_Data = 0xff;
            break;
    }
}

void FelicaEncap(com_device_t* dev, comio_recv_head_t* req, LPBYTE lpPacket, PNFC_CARD lpCard) {
    if (lpCard->m_Header.m_Type != CardType_FeliCa) {
        log_error(plfAime, "Reader %02x requested a FeliCa encap on a non-FeliCa card!", req->dst);
        comio_reply(dev, req, TN32Error_CardError, 0, NULL);
        return;
    }

    PFelicaEncapRequest lpReq = (PFelicaEncapRequest)lpPacket;
    FelicaEncapResponse fResponse;

    FelicaCommand felicaCmd = lpReq->m_FelicaCommand;
    fResponse.m_FelicaCommand = felicaCmd + 1;
    fResponse.m_FelicaLen = 0;

    /**
     * Suica devices will:
     * - FelicaCommand_Poll
     * - FelicaCommand_RequestSystemCode
     * - FelicaCommand_MobileActive2
     *
     * Others (maimai does not appear to disambiguate?) will:
     * - FelicaCommand_Poll
     * - FelicaCommand_ReadNoEnc  (block 82)
     * - FelicaCommand_WriteNoEnc (block 80)
     * - FelicaCommand_ReadNoEnc  (block 82,21)
     * - FelicaCommand_ReadNoEnc  (block 0D)
     */

    log_game(plfAime, "FeliCa command: %02x", felicaCmd);
    switch (felicaCmd) {
        case FelicaCommand_Poll:
            FelicaPoll(lpReq, &fResponse, lpCard);
            break;
        case FelicaCommand_ReadNoEnc:
            FelicaReadNoEnc(lpReq, &fResponse, lpCard);
            break;
        case FelicaCommand_WriteNoEnc:
            FelicaWriteNoEnc(lpReq, &fResponse, lpCard);
            break;
        case FelicaCommand_RequestSystemCode:
            FelicaSystemCode(lpReq, &fResponse, lpCard);
            break;
        case FelicaCommand_MobileActive2:
            FelicaMobileActive2(lpReq, &fResponse, lpCard);
            break;
        default:
            log_error(plfAime, "Unknown FeliCa command: %02x", felicaCmd);
            break;
    }

    comio_reply(dev, req, TN32Error_OK, fResponse.m_FelicaLen, &fResponse);
}
