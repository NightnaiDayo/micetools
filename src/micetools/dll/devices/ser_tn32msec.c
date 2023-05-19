/*
FeliCa Lite:
    AIC cards starting with 300x, 302x, and 303x contain their AIC number in block 0D
*/

#include "ser_tn32msec.h"

#include "../../lib/mice/solitaire.h"
#include "_devices.h"
#include "nfc/nfc.h"

static BYTE extra[0xff];

BYTE cardLED_RGB_P1[3];
BYTE cardLED_RGB_P2[3];

static char* OpcodeNames[256];

typedef enum TN32_FwVer {
    TN32_FwVer_10 = 0x10,  // 1.0
    TN32_FwVer_12 = 0x12,  // 1.2
} TN32_FwVer;

TN32_FwVer FIRMWARE_VERSION = TN32_FwVer_10;

static const char FWVer00[] = "TN32MSEC003S F/W Ver0.0";  // Not a real firmware version
static const char FWVer10[] = "TN32MSEC003S F/W Ver1.0";
static const char FWVer12[] = "TN32MSEC003S F/W Ver1.2";
static const char HWVer[] = "TN32MSEC003S H/W Ver3.0";

NFC_CARD nfcCardReaderP1;
NFC_CARD nfcCardReaderP2;
#define READER_P1_ADDR 0x00
#define READER_P2_ADDR 0x01
#define READER_P1_LED_ADDR 0x08
#define READER_P2_LED_ADDR 0x09

CHAR cardIdentifier1P[32];
CHAR cardIdentifier2P[32];
static LPCSTR GetCardIdentifier(DWORD player) {
    LPCSTR card = player == 0 ? MiceConfig.aime.player1_card : MiceConfig.aime.player2_card;
    LPCSTR cardFile =
        player == 0 ? MiceConfig.aime.player1_cardfile : MiceConfig.aime.player2_cardfile;
    LPSTR cardIdentifier = player == 0 ? cardIdentifier1P : cardIdentifier2P;

    if (_PathFileExistsA(cardFile)) {
        log_info(plfAime, "Reading P%d card from %s", player + 1, cardFile);

        HANDLE hFile =
            _CreateFileA(cardFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            log_warning(plfAime, "Failed to open %s for reading", cardFile);
        } else {
            DWORD nRead;
            if (!_ReadFile(hFile, cardIdentifier, 32, &nRead, NULL)) {
                log_error(plfAime, "Failed to read from %s", cardFile);
            } else {
                card = cardIdentifier;
            }
            _CloseHandle(hFile);
        }
    } else if (card == NULL || strlen(card) == 0) {
        log_error(plfAime,
                  "No card specified for Player %d. Either specify a card in settings, or place a "
                  "card ID into %s",
                  player + 1, cardFile);
    }

    return card == NULL ? "" : card;
}

/**
 * @brief Convert a card identification string to either an access code or felica ID
 *
 * Supported card formats:
 *   Access codes (AiMe Mifare cards):
 *     "1234-1234-1234-1234-1234"
 *     "1234 1234 1234 1234 1234"
 *     "12341234123412341234"
 *   FeliCa IDm (AIC cards):
 *     "0134567890123456"
 *     "01 34 56 78 90 12 34 56"
 */
static CardType TN32GetCardInfo(LPCSTR lpIdentifier, PACCESS_CODE lpAccessCode,
                                PFELICA_ID lpFelicaId) {
    DWORD nChars;
    DWORD len = strlen(lpIdentifier);

    if (len == 24) {
        if (sscanf_s(lpIdentifier,
                     "%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx%n",
                     &lpAccessCode->m_AC[0], &lpAccessCode->m_AC[1], &lpAccessCode->m_AC[2],
                     &lpAccessCode->m_AC[3], &lpAccessCode->m_AC[4], &lpAccessCode->m_AC[5],
                     &lpAccessCode->m_AC[6], &lpAccessCode->m_AC[7], &lpAccessCode->m_AC[8],
                     &lpAccessCode->m_AC[9], &nChars) == 10 &&
            nChars == 24) {
            return CardType_Mifare;
        }
        if (sscanf_s(lpIdentifier,
                     "%02hhx%02hhx %02hhx%02hhx %02hhx%02hhx %02hhx%02hhx %02hhx%02hhx%n",
                     &lpAccessCode->m_AC[0], &lpAccessCode->m_AC[1], &lpAccessCode->m_AC[2],
                     &lpAccessCode->m_AC[3], &lpAccessCode->m_AC[4], &lpAccessCode->m_AC[5],
                     &lpAccessCode->m_AC[6], &lpAccessCode->m_AC[7], &lpAccessCode->m_AC[8],
                     &lpAccessCode->m_AC[9], &nChars) == 10 &&
            nChars == 24) {
            return CardType_Mifare;
        }
    } else if (len == 20) {
        if (sscanf_s(lpIdentifier, "%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%n",
                     &lpAccessCode->m_AC[0], &lpAccessCode->m_AC[1], &lpAccessCode->m_AC[2],
                     &lpAccessCode->m_AC[3], &lpAccessCode->m_AC[4], &lpAccessCode->m_AC[5],
                     &lpAccessCode->m_AC[6], &lpAccessCode->m_AC[7], &lpAccessCode->m_AC[8],
                     &lpAccessCode->m_AC[9], &nChars) == 10 &&
            nChars == 20)
            return CardType_Mifare;
    } else if (len == 16) {
        if (sscanf_s(lpIdentifier, "%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%n",
                     &lpFelicaId->m_ID[0], &lpFelicaId->m_ID[1], &lpFelicaId->m_ID[2],
                     &lpFelicaId->m_ID[3], &lpFelicaId->m_ID[4], &lpFelicaId->m_ID[5],
                     &lpFelicaId->m_ID[6], &lpFelicaId->m_ID[7], &nChars) == 8 &&
            nChars == 16)
            return CardType_FeliCa;
    } else if (len == 23) {
        if (sscanf_s(lpIdentifier, "%02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx%n",
                     &lpFelicaId->m_ID[0], &lpFelicaId->m_ID[1], &lpFelicaId->m_ID[2],
                     &lpFelicaId->m_ID[3], &lpFelicaId->m_ID[4], &lpFelicaId->m_ID[5],
                     &lpFelicaId->m_ID[6], &lpFelicaId->m_ID[7], &nChars) == 8 &&
            nChars == 23)
            return CardType_FeliCa;
    }
    return CardType_Unknown;
}

#define ACKEY_ACCESSCODE_LEN 20
#define ACKEY_ACCESSHEADER_ID_LEN 5
#define ACKEY_SERIAL_LEN 8

static DWORD AccessCodeToSerial(PACCESS_CODE lpAccessCode) {
    char accessCode[ACKEY_ACCESSCODE_LEN + 1];
    sprintf_s(accessCode, sizeof accessCode, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
              lpAccessCode->m_AC[0], lpAccessCode->m_AC[1], lpAccessCode->m_AC[2],
              lpAccessCode->m_AC[3], lpAccessCode->m_AC[4], lpAccessCode->m_AC[5],
              lpAccessCode->m_AC[6], lpAccessCode->m_AC[7], lpAccessCode->m_AC[8],
              lpAccessCode->m_AC[9]);

    char hashed_id_wrk[ACKEY_SERIAL_LEN + 1];
    char id_wrk[ACKEY_SERIAL_LEN + 1];

    memcpy_s(hashed_id_wrk, sizeof(hashed_id_wrk), &accessCode[ACKEY_ACCESSHEADER_ID_LEN],
             ACKEY_SERIAL_LEN);
    hashed_id_wrk[ACKEY_SERIAL_LEN] = '\0';

    SolitaireCipherDecode(&accessCode[ACKEY_ACCESSHEADER_ID_LEN + ACKEY_SERIAL_LEN], hashed_id_wrk,
                          id_wrk);
    return atoi(id_wrk);
}

static void MifarePopulate(PNFC_CARD lpCard, PACCESS_CODE lpAccessCode) {
    lpCard->m_Header.m_IdLen = sizeof(lpCard->m_Mifare);
    DWORD nSerial = AccessCodeToSerial(lpAccessCode);
    lpCard->m_Mifare.m_Uid = nSerial;

    printf("Serial: %d\n", nSerial);

    // Block 0: Manufacture data (only used by BNG)

    // Block 1: Game ID information (kCARD_DETECT_KEY_B_READ_1)
    const char GAME_ID[4] = { 'S', 'B', 'S', 'D' };
    memcpy_s(&lpCard->m_MifareData.sectors[0].blocks[1].bytes[0],
             sizeof lpCard->m_MifareData.sectors[0].blocks[1].bytes, GAME_ID, 4);
    lpCard->m_MifareData.sectors[0].blocks[1].bytes[12] = (nSerial >> 24) & 0xff;
    lpCard->m_MifareData.sectors[0].blocks[1].bytes[13] = (nSerial >> 16) & 0xff;
    lpCard->m_MifareData.sectors[0].blocks[1].bytes[14] = (nSerial >> 8) & 0xff;
    lpCard->m_MifareData.sectors[0].blocks[1].bytes[15] = nSerial & 0xff;

    // Block 2: Access code (kCARD_DETECT_KEY_B_READ_2)
    memcpy_s(&lpCard->m_MifareData.sectors[0].blocks[2].bytes[6],
             sizeof lpCard->m_MifareData.sectors[0].blocks[2].bytes - 6, lpAccessCode->m_AC,
             sizeof lpAccessCode->m_AC);
}
void AmiiboPopulate(PNFC_CARD lpCard) {
    // What _is_ the ID though?
    lpCard->m_Header.m_IdLen = sizeof(lpCard->m_Mifare);

    // Write the card
    for (BYTE i = 0; i < 10; i++) {
        lpCard->m_MifareData.sectors[0].blocks[21].bytes[i] = 0;
    }
}

BYTE TN32PopulateCard(PNFC_CARD lpCard, LPCSTR lpIdentifier) {
    ACCESS_CODE accessCode;
    FELICA_ID felicaId;

    CardType type = TN32GetCardInfo(lpIdentifier, &accessCode, &felicaId);
    if (type == CardType_Unknown) {
        log_error(plfAime, "Unable to parse card: %s", lpIdentifier);
        return 0;
    }

    lpCard->m_Header.m_Type = type;
    if (type == CardType_Mifare) {
        log_info(plfAime,
                 "Inserting card: %02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx",
                 accessCode.m_AC[0], accessCode.m_AC[1], accessCode.m_AC[2], accessCode.m_AC[3],
                 accessCode.m_AC[4], accessCode.m_AC[5], accessCode.m_AC[6], accessCode.m_AC[7],
                 accessCode.m_AC[8], accessCode.m_AC[9]);

        MifarePopulate(lpCard, &accessCode);
        return NFC_MifareSize;
    } else if (type == CardType_FeliCa) {
        log_info(plfAime, "Inserting card: %02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
                 felicaId.m_ID[0], felicaId.m_ID[1], felicaId.m_ID[2], felicaId.m_ID[3],
                 felicaId.m_ID[4], felicaId.m_ID[5], felicaId.m_ID[6], felicaId.m_ID[7]);

        // TODO: FeliCa AIC cards can take an access code in their first block,
        // and we support this. Figure out a nice way to allow this to be
        // specified by users. That said, I'm not sure if anything in the Ring
        // series actually uses this.

        FelicaPopulate(lpCard, &felicaId, NULL);
        return NFC_FeliCaSize;
    }

    return 0;
}

static void nfc_poll(com_device_t* dev, comio_recv_head_t* req) {
    BYTE data[256];
    data[0] = 0;
    BYTE nbytes = 1;

    BOOL p1 = req->dst == READER_P1_ADDR;

    PNFC_CARD lpCard = p1 ? &nfcCardReaderP1 : &nfcCardReaderP2;

    SHORT key = GetAsyncKeyState(p1 ? 'P' : 'L');
    if (key < 0) {
        LPCSTR cardId = GetCardIdentifier(p1 ? 0 : 1);
        BYTE nCardBytes = TN32PopulateCard(lpCard, cardId);
        if (nCardBytes) {
            memcpy(data + nbytes, lpCard, nCardBytes);
            nbytes += nCardBytes;
            data[0]++;
        }
    }
    comio_reply(dev, req, TN32Error_OK, nbytes, data);
}

// Games don't expect this to be preloaded, but we might as well
BYTE AIME_KEY[6] = { 0x57, 0x43, 0x43, 0x46, 0x76, 0x32 };
BYTE BANA_KEY[6] = { 0x60, 0x90, 0xd0, 0x06, 0x32, 0xf5 };

void MifareBlockRead(com_device_t* dev, comio_recv_head_t* req, LPBYTE lpPacket, PNFC_CARD lpCard) {
    // langth = 4 bytes uid + 1 byte block
    if (req->length != 5) {
        log_warning(plfAime, "Mifare read packet bad: %d", req->length);
        comio_reply(dev, req, TN32Error_InvalidData, 0, NULL);
        return;
    }

    if (lpCard->m_Header.m_Type != CardType_Mifare) {
        log_error(plfAime, "Reader %02x requested a Mifare read on a non-Mifare card!", req->dst);
        comio_reply(dev, req, TN32Error_CardError, 0, NULL);
        return;
    }

    DWORD uid = *((LPDWORD)(&lpPacket[0]));
    BYTE blockNo = lpPacket[4];

    if (blockNo > 3) {
        log_warning(plfAime, "Mifare block out of range: %d", blockNo);
        comio_reply(dev, req, TN32Error_CardError, 0, NULL);
        return;
    }
    if (lpCard->m_Mifare.m_Uid != uid) {
        log_error(plfAime,
                  "Reader %02x requested to read card %08x. Currently present card is %08x.", uid,
                  lpCard->m_Mifare.m_Uid);
        comio_reply(dev, req, TN32Error_CardError, 0, NULL);
        return;
    }

    log_misc(plfAime, "Read mifare block: %d", blockNo);
    MifareBlock_t* block = &(lpCard->m_MifareData.sectors[0].blocks[blockNo]);
    comio_reply(dev, req, TN32Error_OK, sizeof *block, block->bytes);
}

DWORD WINAPI aime_bd_thread(com_device_t* dev) {
    static int fwNumBytes = 0;

    log_info(plfAime, "%ls woke up", dev->com->wName);
    bool radio = false;

    while (1) {
        comio_recv_head_t req;
        unsigned char sum = comio_next_req(dev, &req, extra);

        log_trace(plfAime, "(%d) %02x(%d) = %s", req.dst, req.op, req.length, OpcodeNames[req.op]);

        // Proxy 1P reader to hardware COM1. Used for debugging; preserved because it might end up
        // useful again in the future. Allowing proxying to hardware a first-party feature could be
        // cool down the line, but would need much much nicer code than this :D
        /*
        if (req.dst == READER_P1_ADDR) {
            HANDLE hCom = _CreateFileA("\\\\.\\COM1", GENERIC_READ | GENERIC_WRITE, 0, NULL,
                                       OPEN_EXISTING, 0, NULL);
            DWORD nWrote;
            BYTE data = 0xE0;
            BYTE mark = 0xD0;
            _WriteFile(hCom, &data, 1, &nWrote, NULL);
            for (int i = 0; i < sizeof req; i++) {
                data = ((LPBYTE)&req)[i];
                if (data == 0xE0 || data == 0xD0) {
                    _WriteFile(hCom, &mark, 1, &nWrote, NULL);
                    data--;
                }
                _WriteFile(hCom, &data, 1, &nWrote, NULL);
            }
            for (int i = 0; i < req.length; i++) {
                data = extra[i];
                if (data == 0xE0 || data == 0xD0) {
                    _WriteFile(hCom, &mark, 1, &nWrote, NULL);
                    data--;
                }
                _WriteFile(hCom, &data, 1, &nWrote, NULL);
            }
            data = sum;
            if (data == 0xE0 || data == 0xD0) {
                _WriteFile(hCom, &mark, 1, &nWrote, NULL);
                data--;
            }
            _WriteFile(hCom, &data, 1, &nWrote, NULL);

            Sleep(250);
            BYTE readBuf[4096];
            DWORD nRead;
            _ReadFile(hCom, readBuf, 4096, &nRead, NULL);
            log_game(plfAime, "Transact read: %d", nRead);
            for (DWORD i = 0; i < nRead; i++) printf("%02x ", readBuf[i]);
            puts("");

            comdev_write(dev, readBuf, nRead);

            _CloseHandle(hCom);
            continue;
        }
        */

        if (req.dst == READER_P1_ADDR || req.dst == READER_P2_ADDR) {
            PNFC_CARD lpCard = req.dst == READER_P1_ADDR ? &nfcCardReaderP1 : &nfcCardReaderP2;

            // Aime readers
            switch (req.op) {
                // Core
                case TN32Op_GetFWVersion:
                    if (FIRMWARE_VERSION == TN32_FwVer_10)
                        comio_reply(dev, &req, TN32Error_OK, sizeof FWVer10 - 1, (LPBYTE)FWVer10);
                    else if (FIRMWARE_VERSION == TN32_FwVer_12)
                        comio_reply(dev, &req, TN32Error_OK, sizeof FWVer12 - 1, (LPBYTE)FWVer12);
                    else
                        comio_reply(dev, &req, TN32Error_OK, sizeof FWVer00 - 1, (LPBYTE)FWVer00);
                    break;
                case TN32Op_GetHWVersion:
                    comio_reply(dev, &req, TN32Error_OK, sizeof HWVer - 1, (LPBYTE)HWVer);
                    break;

                // Radio
                case TN32Op_RadioOn:
                    radio = true;
                    comio_reply(dev, &req, TN32Error_OK, 0, NULL);
                    break;
                case TN32Op_RadioOff:
                    radio = false;
                    comio_reply(dev, &req, TN32Error_OK, 0, NULL);
                    break;
                case TN32Op_CardDetect:
                    nfc_poll(dev, &req);
                    break;
                case TN32Op_CardHalt:
                    // TODO: What's this?
                    comio_reply(dev, &req, TN32Error_OK, 0, NULL);
                    break;
                case TN32Op_CardSelect:
                    // TODO: Card stacks, for games that need those
                    comio_reply(dev, &req, TN32Error_OK, 0, NULL);
                    break;

                // Mifare
                case TN32Op_MifareKeySetA:
                    if (req.length != sizeof BANA_KEY) {
                        comio_reply(dev, &req, TN32Error_InvalidData, 0, NULL);
                    } else {
                        memcpy(BANA_KEY, extra, sizeof BANA_KEY);
                        comio_reply(dev, &req, TN32Error_OK, 0, NULL);

                        log_misc(plfAime, "Bana key: %02x %02x %02x %02x %02x %02x", BANA_KEY[0],
                                 BANA_KEY[1], BANA_KEY[2], BANA_KEY[3], BANA_KEY[4], BANA_KEY[5]);
                    }
                    break;
                case TN32Op_MifareKeySetB:
                    if (req.length != sizeof AIME_KEY) {
                        comio_reply(dev, &req, TN32Error_InvalidData, 0, NULL);
                    } else {
                        memcpy(AIME_KEY, extra, sizeof AIME_KEY);
                        comio_reply(dev, &req, TN32Error_OK, 0, NULL);

                        log_misc(plfAime, "Aime key: %02x %02x %02x %02x %02x %02x", AIME_KEY[0],
                                 AIME_KEY[1], AIME_KEY[2], AIME_KEY[3], AIME_KEY[4], AIME_KEY[5]);
                    }
                    break;
                case TN32Op_MifareRead:
                    /**
                     * All Mifare reads AiMeLib can perform:
                     *
                     * kCARD_DETECT_KEY_B_READ_0: [0]
                     * kCARD_DETECT_KEY_B_READ_1: [1]
                     * kCARD_DETECT_KEY_B_READ_2: [2]
                     * kCARD_DETECT_KEY_A_READ_0: [0]
                     * kCARD_DETECT_KEY_A_READ_1: [1]
                     * kCARD_DETECT_KEY_A_READ_2: [2]
                     *
                     * kCARD_DETECT_NAUTH_READ: [21]  Amiibo things
                     * https://www.reddit.com/r/amiibo/comments/38rdll/my_research_on_amiibo_id_this_time_about/
                     * > Pages 21 and 22 are character IDs
                     * https://www.reddit.com/r/amiibo/comments/38p4a0/my_research_on_amiibo_id_including_variants_types/
                     *
                     * Supporting Amiibos is probably not worth doing until we find a game that
                     * actually uses them!
                     */
                    MifareBlockRead(dev, &req, extra, lpCard);
                    break;
                case TN32Op_MifareWrite:
                    // TODO: Do we want to support writing?
                    comio_reply(dev, &req, TN32Error_InvalidCommand, 0, NULL);
                    break;
                case TN32Op_MifareAuthorizeA:
                case TN32Op_MifareAuthorizeB:
                    // TODO: We probably should?
                    if (req.length != 5) {
                    }
                    comio_reply(dev, &req, TN32Error_OK, 0, NULL);
                    break;

                // Firmware control
                case TN32Op_EnterUpdaterMode:
                    comio_reply(dev, &req, TN32Error_OK, 0, NULL);
                    break;
                case TN32Op_SendHex:
                    // null-terminated line of the firmware hex!
                    comio_reply(dev, &req, TN32Error_OK, 0, NULL);
                    log_info(plfAime, "Recv firmware: %s", extra);
                    break;
                case TN32Op_Reset:
                    comio_reply(dev, &req, TN32Error_OK, 0, NULL);
                    break;
                case TN32Op_SendBindata:
                    fwNumBytes = 0;
                    comio_reply(dev, &req, TN32Error_OK, 0, NULL);
                    break;
                case TN32Op_BindataExec:
                    // Raw binary firmware data
                    // [req.length == 256 -> wraps to 00] [256 bytes]
                    if (req.length == 0) {
                        unsigned char update_buffer[256];
                        update_buffer[0] = sum;
                        comio_read(dev, update_buffer + 1, 255);
                        // We need to account for the actual checksum at the end of this lot
                        comio_read(dev, &sum, 1);

                        fwNumBytes += 256;
                    } else {
                        fwNumBytes += req.length;
                    }

                    // nxAuth is segfaulting past this. Rather than figure it out, let's just
                    // protect it from itself for now.
                    if (fwNumBytes >= 0x3000)
                        ;
                    else
                        comio_reply(dev, &req, TN32Error_OK, 0, NULL);
                    break;

                // Felica
                case TN32Op_FelicaPush:
                    // TODO: Implement mail and url opening?
                    comio_reply(dev, &req, TN32Error_InvalidCommand, 0, NULL);
                    break;
                case TN32Op_FelicaEncap:
                    if (FIRMWARE_VERSION == TN32_FwVer_12) {
                        FelicaEncap(dev, &req, extra, lpCard);
                    } else {
                        /**
                         * Firmware version 1.0 has no support for FeliCa
                         * proxying, and just returns Invalid Command for 70h
                         * and 71h. maimai is absolutely happy with this, and
                         * will just use the IDm from the Poll request without
                         * querying the card further.
                         *
                         * In fact, maimai _cannot_ function with newer
                         * firmware, due to an outdated restriction within
                         * AiMeLib that only allows BNG cards.
                         */
                        comio_reply(dev, &req, TN32Error_InvalidCommand, 0, NULL);
                    }
                    break;
            }
        } else if (req.dst == READER_P1_LED_ADDR || req.dst == READER_P2_LED_ADDR) {
            // LED sub-boards
            switch (req.op) {
                case Led_Reset:
                    comio_reply(dev, &req, TN32Error_OK, 0, NULL);
                    break;
                case Led_GetInfo:
                    // Format:
                    // 0xff-delimited board info string
                    // 1 byte, board version
                    comio_reply(dev, &req, TN32Error_OK, 9, (BYTE*)"15084\xff\x10");
                    break;
                case Led_SetColour:
                    log_misc(plfAime, "Set LED: %02x @ %02x intensity", extra[0], extra[1]);
                    break;
                case Led_SetColourRGB:
                    log_misc(plfAime, "Set LED: #%02x%02x%02x", extra[0], extra[1], extra[2]);
                    // TODO: Bring this back, in a nicer way

                    if (req.dst == READER_P1_LED_ADDR) {
                        cardLED_RGB_P1[0] = extra[0];
                        cardLED_RGB_P1[1] = extra[1];
                        cardLED_RGB_P1[2] = extra[2];
                    } else {
                        cardLED_RGB_P2[0] = extra[0];
                        cardLED_RGB_P2[1] = extra[1];
                        cardLED_RGB_P2[2] = extra[2];
                    }
                    break;
            }
        }

        Sleep(50);
    }
}

void install_aime_bd() {
    ZeroMemory(OpcodeNames, sizeof OpcodeNames);
    OpcodeNames[TN32Op_GetFWVersion] = "GetFWVersion";
    OpcodeNames[TN32Op_GetHWVersion] = "GetHWVersion";
    OpcodeNames[TN32Op_RadioOn] = "RadioOn";
    OpcodeNames[TN32Op_RadioOff] = "RadioOff";
    OpcodeNames[TN32Op_CardDetect] = "CardDetect";
    OpcodeNames[TN32Op_CardSelect] = "CardSelect";
    OpcodeNames[TN32Op_CardHalt] = "CardHalt";
    OpcodeNames[TN32Op_MifareKeySetA] = "MifareKeySetA";
    OpcodeNames[TN32Op_MifareAuthorizeA] = "MifareAuthorizeA";
    OpcodeNames[TN32Op_MifareRead] = "MifareRead";
    OpcodeNames[TN32Op_MifareWrite] = "MifareWrite";
    OpcodeNames[TN32Op_MifareKeySetB] = "MifareKeySetB";
    OpcodeNames[TN32Op_MifareAuthorizeB] = "MifareAuthorizeB";
    OpcodeNames[TN32Op_EnterUpdaterMode] = "EnterUpdaterMode";
    OpcodeNames[TN32Op_SendHex] = "SendHex";
    OpcodeNames[TN32Op_Reset] = "Reset";
    OpcodeNames[TN32Op_FelicaPush] = "FelicaPush";
    OpcodeNames[TN32Op_FelicaEncap] = "FelicaEncap";

    OpcodeNames[Led_Reset] = "Led_Reset";
    OpcodeNames[Led_GetInfo] = "Led_GetInfo";
    OpcodeNames[Led_SetColour] = "Led_SetColour";
    OpcodeNames[Led_SetColourRGB] = "Led_SetColourRGB";

    register_device("aime_tn32msec", aime_bd_thread);
}
