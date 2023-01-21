#include "amDongle.h"

#include "../libpcp/pcpa.h"
#include "../util/hex.h"

AM_LIB_C_HEADER(amDongle, AM_DONGLE)

AM_DONGLE_STATUS amDongleInit(void) {
    if (amDongle.m_init) return AM_DONGLE_STATUS_ERR_ALREADY_INIT;

    memset(&amDongle, 0, sizeof amDongle);
    amDongle.version = 0;
    amDongle.seq = -1;
    amDongle.field19_0x40 = -1;
    amDongle.done_init = 0;
    amDongle.available = 0;
    //   amDongle.field14_0x2c = 5;
    amDongle.develop = 0;
    amDongle.authCondition = 1;
    amDongle.auth_ready = 0;
    //   amDongle.field6_0x18 = 0;
    //   amDongle.field7_0x1c = 0;
    //   amDongle.field8_0x20 = 0;
    //   amDongle.field13_0x28 = 1;
    amDongle.setupSeq = AM_DONGLE_SETUP_SEQ_START;
    amDongle.setupSeqNext = AM_DONGLE_SETUP_SEQ_NONE;
    amDongle.setupRetries = 0;
    amDongle.requestCode = 0;
    amDongle.result = 0;
    //   amDongle.field21_0x48 = 1;
    //   amDongle.field22_0x4c = 0;
    amDongle.challengeNum = 0;
    amDongle.ctrl_port = 40106;
    amDongle.data_port = 40107;
    amDongle.valueBuffer = NULL;
    amDongle.dataBuffer = NULL;
    //   amDongle.field51_0x3440 = 0;
    //   amDongle.field53_0x3448 = 0;

    e_pcpa_t pcpa_err = pcpaInitStream(&amDongle.pcpa);
    if (pcpa_err == e_pcpa_wsa_noinit) {
        if (amDongleDebugLevel > 0) amiDebugLog("Error: Winsocket2 Library is not ready.");
        return AM_DONGLE_STATUS_ERR_PRECONDITION;
    }
    if (pcpa_err != e_pcpa_ok) {
        if (amDongleDebugLevel > 0) {
            amiDebugLog("Error: System error happened in pcp stream. ErrorCode = %d.",
                            pcpa_err);
        }
        return AM_DONGLE_STATUS_ERR_SYS;
    }
    amDongle.m_init = 1;
    return AM_DONGLE_STATUS_OK;
}

AM_DONGLE_STATUS amDongleOpen(void) {
    if (!amDongle.m_init) {
        return AM_DONGLE_STATUS_ERR_NO_INIT;
    }
    if (!amDongle.done_init || !amDongle.available) {
        return AM_DONGLE_STATUS_NG;
    }

    e_pcpa_t pcpa_err = pcpaOpenClient(&amDongle.pcpa, "127.0.0.1", amDongle.ctrl_port, 90000, 0);
    if (pcpa_err == e_pcpa_ok) {
        amDongle.seq = AM_DONGLE_SEQ_OPEN;
        amDongle.result = 1;
        return AM_DONGLE_STATUS_OK;
    }
    if (pcpa_err == e_pcpa_to) {
        amDongle.seq = AM_DONGLE_SEQ_WAIT_OPEN;
        amDongle.field19_0x40 = 2;
        amDongle.result = 1;
        return AM_DONGLE_STATUS_OK;
    }
    if (amDongleDebugLevel > 0) {
        amiDebugLog("Error : pcpaOpenClient(%d)", pcpa_err);
    }
    amDongle.seq = AM_DONGLE_SEQ_DONE;
    return AM_DONGLE_STATUS_ERR_PCP;
}

AM_DONGLE_STATUS amDongleOpenEx(void) {
    if (amDongle.m_init == 0) return AM_DONGLE_STATUS_ERR_NO_INIT;

    if (!amDongle.done_init || !amDongle.available) return AM_DONGLE_STATUS_NG;

    e_pcpa_t pcpa_err;
    while (true) {
        pcpa_err =
            pcpaOpenClient(&amDongle.pcpa, "127.0.0.1", amDongle.ctrl_port, 90000, TIMEOUT_NONE);
        if (pcpa_err == e_pcpa_ok) return AM_DONGLE_STATUS_OK;
        if (pcpa_err != e_pcpa_timeout_open) break;
        pcpaClose(&amDongle.pcpa);
    }
    if (amDongleDebugLevel > 0) amiDebugLog("Error : pcpaOpenClient(%d)", pcpa_err);
    amDongle.seq = AM_DONGLE_SEQ_DONE;
    amDongle.field19_0x40 = -1;
    return AM_DONGLE_STATUS_ERR_NO_SERVER;
}

BOOL amDongleIsDevelop(void) {
    if (amDongle.done_init && amDongle.develop) {
        return TRUE;
    }
    return FALSE;
}

BOOL amDongleIsAvailable(void) {
    if (amDongle.done_init && amDongle.available) {
        return TRUE;
    }
    return FALSE;
}

AM_DONGLE_STATUS amDongleCodeToStatus(void) {
    char *codeStr = pcpaGetCommand(&amDongle.pcpa, "code");
    if (codeStr == NULL) return AM_DONGLE_STATUS_OK;

    int code = atoi(codeStr);
    if (true) {
        switch (code) {
            case 0:
                return AM_DONGLE_STATUS_OK;
            case 1:
            case 2:
                return AM_DONGLE_STATUS_ERR_KEYCHIP;
            case 50:
                return AM_DONGLE_STATUS_ERR_VERIFY;
            case 51:
                return AM_DONGLE_STATUS_ERR_LOG_FULL;
            case 52:
                return AM_DONGLE_STATUS_ERR_NO_LOG;
            case 53:
                return AM_DONGLE_STATUS_ERR_ONE_WRITE;
            case 54:
                return AM_DONGLE_STATUS_ERR_COMMAND;
            case 55:
                return AM_DONGLE_STATUS_ERR_KEYCHIP_DATA;
        }
    }
    return AM_DONGLE_STATUS_ERR_PCP;
}

AM_DONGLE_STATUS amDongleSetupKeychip(void) {
    if (amDongle.m_init == 0) return AM_DONGLE_STATUS_ERR_NO_INIT;

    e_pcpa_t pcpa_err;
    pcp_send_data_t *send_data;
    switch (amDongle.setupSeq) {
        case AM_DONGLE_SETUP_SEQ_START:
            send_data = pcpaSetSendPacket(&amDongle.pcpa, "keychip.appboot.systemflag", "?");
            if (send_data == NULL && amDongleDebugLevel > 0)
                amiDebugLog("Error: pcpaSetSendPacket return NULL");

            send_data = pcpaAddSendPacket(&amDongle.pcpa, "cache", "0");
            if (send_data == NULL && amDongleDebugLevel > 0)
                amiDebugLog("Error: pcpaAddSendPacket return NULL");

            pcpa_err = pcpaOpenClient(&amDongle.pcpa, "127.0.0.1", amDongle.ctrl_port, 90000, 0);
            if (pcpa_err != e_pcpa_ok) {
                if (pcpa_err != e_pcpa_to) {
                    if (amDongleDebugLevel > 0)
                        amiDebugLog("Error : pcpaOpenClient(%d)", pcpa_err);

                    amDongle.setupSeq = AM_DONGLE_SETUP_SEQ_ERR;
                    return AM_DONGLE_STATUS_PENDING;
                }

                amDongle.setupSeq = AM_DONGLE_SETUP_SEQ_WAIT;
                amDongle.setupSeqNext = AM_DONGLE_SETUP_SEQ_SEND;
                return AM_DONGLE_STATUS_PENDING;
            }

            amDongle.setupSeq = AM_DONGLE_SETUP_SEQ_SEND;
            return AM_DONGLE_STATUS_PENDING;

        case AM_DONGLE_SETUP_SEQ_WAIT:
            pcpa_err = pcpaIsBusy(&amDongle.pcpa, 0);
            if (pcpa_err == e_pcpa_ok) {
                AM_DONGLE_SETUP_SEQ next = amDongle.setupSeqNext;
                amDongle.setupSeqNext = AM_DONGLE_SETUP_SEQ_NONE;
                amDongle.setupSeq = next;
                return AM_DONGLE_STATUS_PENDING;
            }

            if (pcpa_err == e_pcpa_timeout_open) {
                if (amDongleDebugLevel > 0) {
                    amiDebugLog("Error : PCPA_RESULT_PROMPT_TIME_OUT_ERROR", 0xfffffff2);
                }
                pcpaClose(&amDongle.pcpa);
                amDongle.setupSeq = 1;
                amDongle.setupSeqNext = AM_DONGLE_SETUP_SEQ_NONE;
                return AM_DONGLE_STATUS_PENDING;
            }

            if (pcpa_err == e_pcpa_to) return AM_DONGLE_STATUS_PENDING;

            if (amDongleDebugLevel > 0) amiDebugLog("Error : pcpaIsBusy(%d)", pcpa_err);
            amDongle.setupSeq = AM_DONGLE_SETUP_SEQ_RETRY;
            return AM_DONGLE_STATUS_PENDING;

        case AM_DONGLE_SETUP_SEQ_SEND:
            pcpa_err = pcpaSendRequest(&amDongle.pcpa, 0);
            if (pcpa_err == e_pcpa_ok) {
                amDongle.setupSeq = AM_DONGLE_SETUP_SEQ_RECV;
                return AM_DONGLE_STATUS_PENDING;
            }
            if (pcpa_err == e_pcpa_to) {
                amDongle.setupSeq = AM_DONGLE_SETUP_SEQ_WAIT;
                amDongle.setupSeqNext = AM_DONGLE_SETUP_SEQ_RECV;
                return AM_DONGLE_STATUS_PENDING;
            }

            if (amDongleDebugLevel > 0) amiDebugLog("Error : pcpaSendRequest(%d)", pcpa_err);
            amDongle.setupSeq = AM_DONGLE_SETUP_SEQ_RETRY;
            return AM_DONGLE_STATUS_PENDING;

        case AM_DONGLE_SETUP_SEQ_RECV:
            pcpa_err = pcpaRecvResponse(&amDongle.pcpa, 0);
            if (pcpa_err == e_pcpa_ok) {
                amDongle.setupSeq = AM_DONGLE_SETUP_SEQ_READ_RESPONSE;
                return AM_DONGLE_STATUS_PENDING;
            }
            if (pcpa_err != e_pcpa_to) {
                if (amDongleDebugLevel > 0) {
                    amiDebugLog("Error : pcpaRecvResponse(%d)", pcpa_err);
                }
                amDongle.setupSeq = AM_DONGLE_SETUP_SEQ_RETRY;
                return AM_DONGLE_STATUS_PENDING;
            }
            amDongle.setupSeq = AM_DONGLE_SETUP_SEQ_WAIT;
            amDongle.setupSeqNext = AM_DONGLE_SETUP_SEQ_READ_RESPONSE;
            return AM_DONGLE_STATUS_PENDING;

        case AM_DONGLE_SETUP_SEQ_READ_RESPONSE:
            AM_DONGLE_STATUS code = amDongleCodeToStatus();
            if (code == AM_DONGLE_STATUS_OK) {
                char *keyword = pcpaGetKeyword(&amDongle.pcpa, 0);
                if (keyword == NULL) {
                    if (amDongleDebugLevel > 0) {
                        amiDebugLog("Error No Response!!");
                    }
                    amDongle.setupSeq = AM_DONGLE_SETUP_SEQ_RETRY;
                    return AM_DONGLE_STATUS_PENDING;
                }

                if (strcmp(keyword, "keychip.appboot.systemflag") == 0) {
                    unsigned char systemflag;
                    hex_to_byte(pcpaGetCommand(&amDongle.pcpa, keyword), &systemflag);
                    if (systemflag & 1) amDongle.develop = 1;

                    send_data = pcpaSetSendPacket(&amDongle.pcpa, "keychip.version", "?");
                    if (send_data == NULL && amDongleDebugLevel > 0)
                        amiDebugLog("Error: pcpaSetSendPacket return NULL");

                    send_data = pcpaAddSendPacket(&amDongle.pcpa, "cache", "0");
                    if (send_data == NULL && amDongleDebugLevel > 0)
                        amiDebugLog("Error: pcpaAddSendPacket return NULL");

                    amDongle.setupSeq = AM_DONGLE_SETUP_SEQ_SEND;
                    return AM_DONGLE_STATUS_PENDING;
                } else if (strcmp(keyword, "keychip.version") == 0) {
                    ushort version;
                    hex_to_bin(pcpaGetCommand(&amDongle.pcpa, keyword), (unsigned char *)&version,
                               4);
                    amDongle.version = ((version << 8) & 0xffff) | (version >> 8);
                    amDongle.setupSeq = AM_DONGLE_SETUP_SEQ_CLOSE;
                    amDongle.setupSeqNext = AM_DONGLE_SETUP_SEQ_DONE;
                    return AM_DONGLE_STATUS_PENDING;
                } else if (amDongleDebugLevel > 0)
                    amiDebugLog("Error Response!!");
            } else if (code == AM_DONGLE_STATUS_ERR_KEYCHIP) {
                amDongle.setupSeq = AM_DONGLE_SETUP_SEQ_CLOSE;
                amDongle.setupSeqNext = AM_DONGLE_SETUP_SEQ_ERR;
                return AM_DONGLE_STATUS_PENDING;
            }
            amDongle.setupSeq = AM_DONGLE_SETUP_SEQ_START;
            return AM_DONGLE_STATUS_PENDING;

        case AM_DONGLE_SETUP_SEQ_CLOSE:
            pcpaClose(&amDongle.pcpa);
            AM_DONGLE_SETUP_SEQ next = amDongle.setupSeqNext;
            amDongle.setupSeq = next;
            amDongle.setupSeqNext = AM_DONGLE_SETUP_SEQ_NONE;
            if (next != AM_DONGLE_SETUP_SEQ_DONE) return AM_DONGLE_STATUS_PENDING;

            amDongle.available = 1;
            amDongle.done_init = 1;
            amDongle.seq = AM_DONGLE_SEQ_NONE;
            amDongle.result = 0;
            return AM_DONGLE_STATUS_PENDING;

        case AM_DONGLE_SETUP_SEQ_RETRY:
            pcpaClose(&amDongle.pcpa);
            amDongle.setupRetries++;
            amDongle.setupSeqNext = AM_DONGLE_SETUP_SEQ_NONE;
            if (amDongle.setupRetries > 2) {
                amDongle.setupSeq = AM_DONGLE_SETUP_SEQ_ERR;
                return AM_DONGLE_STATUS_PENDING;
            }

            amDongle.setupSeq = AM_DONGLE_SETUP_SEQ_START;
            return AM_DONGLE_STATUS_PENDING;

        case AM_DONGLE_SETUP_SEQ_DONE:
            return AM_DONGLE_STATUS_OK;

        default:
            return AM_DONGLE_STATUS_NG;
    }
}

// TODO: FUN_009f6940 !!!
AM_DONGLE_STATUS amDongleResponseCheck(void) {
    char cVar1;
    byte bVar2;
    int *piVar4;
    AM_DONGLE_STATUS AVar7;
    int iVar9;
    uint *puVar11;
    uint local_8;

    char *keyword = pcpaGetKeyword(&amDongle.pcpa, 0);
    char *command = pcpaGetCommand(&amDongle.pcpa, keyword);
    char *sSize = pcpaGetCommand(&amDongle.pcpa, "size");
    char *sAddress = pcpaGetCommand(&amDongle.pcpa, "address");

    amDongle.seq = AM_DONGLE_SEQ_DONE;

    if (keyword == NULL) {
        if (amDongleDebugLevel > 0) amiDebugLog("Response Error!!! No Keyword.");
        return AM_DONGLE_STATUS_ERR_PCP;
    }
    if (keyword[0] == '?') return AM_DONGLE_STATUS_ERR_NO_COMMAND;

    AVar7 = amDongleCodeToStatus();
    if (AVar7 != AM_DONGLE_STATUS_OK) {
        if (AVar7 == AM_DONGLE_STATUS_ERR_KEYCHIP) {
            if (amDongleDebugLevel > 0) amiDebugLog("Error Response KeychipError !!!");
            amDongle.available = 0;
            amDongle.develop = 0;
        }
        return AVar7;
    }

    if ((amDongle.requestCode & 0x80) != 0) {
        char *sPort = pcpaGetCommand(&amDongle.pcpa, "port");
        if (sPort == NULL)
            amDongle.data_port = 40107;
        else
            amDongle.data_port = atoi(sPort) & 0xffff;
        amDongle.seq = 5;
    }

    local_8 = 0;

    // TODO: ??
    if (false) {
        return AM_DONGLE_STATUS_OK;
    }

    switch (amDongle.requestCode) {
        case AM_DONGLE_REQUEST_DECRYPT:
            if (strcmp(keyword, "keychip.decrypt")) break;
            hex_to_bin(command, amDongle.valueBuffer, 32);
            return AM_DONGLE_STATUS_OK;
        case AM_DONGLE_REQUEST_ENCRYPT:
            if (strcmp(keyword, "keychip.encrypt") || command == NULL) break;
            hex_to_bin(command, amDongle.valueBuffer, 32);
            return AM_DONGLE_STATUS_OK;
        case AM_DONGLE_REQUEST_GET_GAME_ID:
            if (strcmp(keyword, "keychip.appboot.gameid") || command == NULL) break;
            strcpy_s((char *)amDongle.valueBuffer, 5, (char *)command);
            return AM_DONGLE_STATUS_OK;
        case AM_DONGLE_REQUEST_GET_SYSTEMFLAG:
            if (strcmp(keyword, "keychip.appboot.systemflag") || command == NULL) break;
            hex_to_bin(command, amDongle.valueBuffer, 2);
            return AM_DONGLE_STATUS_OK;
        case AM_DONGLE_REQUEST_GET_MODEL_TYPE:
            if (strcmp(keyword, "keychip.appboot.modeltype") || command == NULL) break;
            hex_to_bin(command, amDongle.valueBuffer, 2);
            return AM_DONGLE_STATUS_OK;
        case AM_DONGLE_REQUEST_GET_REGION:
            if (strcmp(keyword, "keychip.appboot.region") || command == NULL) break;
            hex_to_bin(command, amDongle.valueBuffer, 2);
            return AM_DONGLE_STATUS_OK;
        case AM_DONGLE_REQUEST_GET_PLATFORM_ID:
            if (strcmp(keyword, "keychip.appboot.platformid") || command == NULL) break;
            strcpy_s(amDongle.valueBuffer, 4, command);
            return AM_DONGLE_STATUS_OK;
        case AM_DONGLE_REQUEST_GET_NETWORK_ADDRESS:
            if (strcmp(keyword, "keychip.appboot.networkaddr") || command == NULL) break;
            if (amDongle.valueBuffer != NULL) *(uint *)amDongle.valueBuffer = inet_addr(command);
            return AM_DONGLE_STATUS_OK;
        case AM_DONGLE_REQUEST_GET_VERSION:
            if (strcmp(keyword, "keychip.version") || command == NULL) break;
            ushort version;
            hex_to_bin(command, (unsigned char *)&version, 4);
            *(ushort *)amDongle.valueBuffer = ((version << 8) & 0xffff) | version >> 8;
            return AM_DONGLE_STATUS_OK;
        case AM_DONGLE_REQUEST_BILLING_GET_KEYCHIP_ID:
            if (strcmp(keyword, "keychip.billing.keyid") || command == NULL) break;
            strcpy_s(amDongle.valueBuffer, 0x11, command);
            return AM_DONGLE_STATUS_OK;
        case AM_DONGLE_REQUEST_BILLING_GET_MAIN_ID:
            if (strcmp(keyword, "keychip.billing.mainid") || command == NULL) break;
            strcpy_s(amDongle.valueBuffer, 0xc, command);
            return AM_DONGLE_STATUS_OK;
        case AM_DONGLE_REQUEST_BILLING_GET_PLAYCOUNT:
            if (strcmp(keyword, "keychip.billing.playcount") || command == NULL) break;
            hex_to_int(amDongle.valueBuffer, command);
            return AM_DONGLE_STATUS_OK;
        case AM_DONGLE_REQUEST_BILLING_ADD_PLAYCOUNT:
            if (strcmp(keyword, "keychip.billing.playcount") || command == NULL) break;
            hex_to_int(amDongle.valueBuffer, command);
            return AM_DONGLE_STATUS_OK;
        case AM_DONGLE_REQUEST_BILLING_GET_PLAYLIMIT:
            if (strcmp(keyword, "keychip.billing.playlimit") || command == NULL) break;
            hex_to_int(amDongle.valueBuffer, command);
            return AM_DONGLE_STATUS_OK;
        case AM_DONGLE_REQUEST_BILLING_GET_NEARFULL:
            if (strcmp(keyword, "keychip.billing.nearfull") || command == NULL) break;
            hex_to_int(amDongle.valueBuffer, command);
            return AM_DONGLE_STATUS_OK;
        case AM_DONGLE_REQUEST_BILLING_TD_RESTORE:
            if (strcmp(keyword, "keychip.tracedata.restore") || command == NULL) break;
            if (strcmp(command, "1") == 0) {
                pcp_send_data_t *packet =
                    pcpaSetSendPacket(&amDongle.pcpa, "keychip.tracedata.restore", "1");
                if (packet == NULL && amDongleDebugLevel > 0)
                    amiDebugLog("Error: pcpaSetSendPacket return NULL");
                packet = pcpaAddSendPacket(&amDongle.pcpa, "cache", "0");
                if (packet == NULL && amDongleDebugLevel > 0)
                    amiDebugLog("Error: pcpaAddSendPacket return NULL");
                packet = pcpaAddSendPacket(&amDongle.pcpa, "update", "1");
                if (packet == NULL && amDongleDebugLevel > 0)
                    amiDebugLog("Error: pcpaAddSendPacket return NULL");
                packet = pcpaAddSendPacket(&amDongle.pcpa, "restart", "0");
                if (packet == NULL && amDongleDebugLevel > 0)
                    amiDebugLog("Error: pcpaAddSendPacket return NULL");

                amDongle.seq = AM_DONGLE_SETUP_SEQ_WAIT;
                return AM_DONGLE_STATUS_OK;
            }
            if (amDongle.valueBuffer != NULL) *(int *)amDongle.valueBuffer = command[0] - '0';
            return AM_DONGLE_STATUS_OK;
        case AM_DONGLE_REQUEST_BILLING_TD_GET_LOG_NUM:
            iVar9 = strcmp(keyword, "keychip.tracedata.get");
            if (((iVar9 == 0) && (command != NULL)) && (keyword = command, sAddress != NULL)) {
                do {
                    bVar2 = *keyword;
                    keyword = keyword + 1;
                } while (bVar2 != 0);
                // FUN_009f6940();
                sSize = sAddress;
                do {
                    cVar1 = *sSize;
                    sSize = sSize + 1;
                } while (cVar1 != '\0');
                // FUN_009f6940();
                return AM_DONGLE_STATUS_OK;
            }
            break;
        case AM_DONGLE_REQUEST_BILLING_TD_GET_FREE_LOG_NUM:
            iVar9 = strcmp(keyword, "keychip.tracedata.put");
            if ((iVar9 == 0) && (keyword = command, command != NULL)) {
                do {
                    bVar2 = *keyword;
                    keyword = keyword + 1;
                } while (bVar2 != 0);
                // FUN_009f6940();
                return AM_DONGLE_STATUS_OK;
            }
            break;
        case AM_DONGLE_REQUEST_BILLING_TD_ERASE_USED_SECTOR:
            iVar9 = strcmp(keyword, "keychip.tracedata.sectorerase");
            // goto LAB_009f7ff7;
        case AM_DONGLE_REQUEST_BILLING_TD_ERASE_ALL:
            iVar9 = strcmp(keyword, "keychip.tracedata.sectorerase");
            if ((iVar9 == 0) && (command != NULL)) {
                piVar4 = (int *)amDongle.valueBuffer;
                if (piVar4 == NULL) {
                    return AM_DONGLE_STATUS_OK;
                }
                *piVar4 = 0;
                *piVar4 = 0;
                *piVar4 = (char)*command + -0x30;
                return AM_DONGLE_STATUS_OK;
            }
            break;
        case AM_DONGLE_REQUEST_BILLING_TD_RESERVE_ERASABLE_SECTOR:
            iVar9 = strcmp(keyword, "keychip.tracedata.sectorerase");
            if (iVar9 != 0) {
                return AM_DONGLE_STATUS_ERR_PCP;
            }
            if (command != NULL) {
                piVar4 = (int *)amDongle.valueBuffer;
                if (piVar4 == NULL) {
                    return AM_DONGLE_STATUS_OK;
                }
                *piVar4 = 0;
                *piVar4 = 0;
                *piVar4 = (char)*command + -0x30;
                return AM_DONGLE_STATUS_OK;
            }
            return AM_DONGLE_STATUS_ERR_PCP;
        case AM_DONGLE_REQUEST_GET_DVDFLAG:
            iVar9 = strcmp(keyword, "keychip.appboot.dvdflag");
            // goto LAB_009f7a26;
        case AM_DONGLE_REQUEST_GET_DS_COMPUTE:
            iVar9 = strcmp(keyword, "keychip.ds.compute");
            if ((iVar9 == 0) && (command != NULL)) {
                keyword = command;
                do {
                    bVar2 = *keyword;
                    keyword = keyword + 1;
                } while (bVar2 != 0);
                local_8 = (int)keyword - (int)(command + 1);
                // FUN_009f6b40(command, amDongle.valueBuffer);
                return AM_DONGLE_STATUS_OK;
            }
            break;
        case AM_DONGLE_REQUEST_GET_SSD_PROOF:
            iVar9 = strcmp(keyword, "keychip.ssd.proof");
            if ((iVar9 == 0) && (command != NULL)) {
                keyword = command;
                do {
                    bVar2 = *keyword;
                    keyword = keyword + 1;
                } while (bVar2 != 0);
                local_8 = (int)keyword - (int)(command + 1);
                // FUN_009f6b40(command, amDongle.valueBuffer);
                return AM_DONGLE_STATUS_OK;
            }
            break;
        case AM_DONGLE_REQUEST_GET_FORMAT_TYPE:
            iVar9 = strcmp(keyword, "keychip.appboot.formattype");
            if ((iVar9 == 0) && (command != NULL)) {
                local_8 = 2;
                // FUN_009f6b40(command, amDongle.valueBuffer);
                return AM_DONGLE_STATUS_OK;
            }
            break;
        case AM_DONGLE_WTF_81:
            if (sSize == NULL) {
            LAB_009f82b7:
                amDongle.seq = 8;
                return AM_DONGLE_STATUS_ERR_PCP;
            }
            do {
                cVar1 = *sSize;
                sSize = sSize + 1;
            } while (cVar1 != '\0');
            goto LAB_009f82d7;
        case AM_DONGLE_REQUEST_BILLING_GET_TRACEDATA:
            if ((command == NULL) || (sSize == NULL)) goto LAB_009f82b7;
            do {
                cVar1 = *sSize;
                sSize = sSize + 1;
            } while (cVar1 != '\0');
            // FUN_009f6940();
            puVar11 = amDongle.field_0x3444;
            goto LAB_009f82f2;
        case AM_DONGLE_REQUEST_BILLING_GET_SIGNATURE_PK:
            if (sSize == NULL) goto LAB_009f82b7;
            do {
                cVar1 = *sSize;
                sSize = sSize + 1;
            } while (cVar1 != '\0');
            goto LAB_009f82d7;
        case AM_DONGLE_REQUEST_BILLING_GET_CA_CERT:
            if (sSize == NULL) goto LAB_009f82b7;
            do {
                cVar1 = *sSize;
                sSize = sSize + 1;
            } while (cVar1 != '\0');
            // FUN_009f6940();
            puVar11 = (uint *)amDongle.dataBuffer;
            goto LAB_009f82f2;
        case AM_DONGLE_WTF_85:
            if (sSize == NULL) goto LAB_009f82b7;
            do {
                cVar1 = *sSize;
                sSize = sSize + 1;
            } while (cVar1 != '\0');
            // FUN_009f6940();
            puVar11 = (uint *)amDongle.dataBuffer;
            goto LAB_009f82f2;
        case AM_DONGLE_WTF_C0:
            if (sSize == NULL) goto LAB_009f82b7;
            do {
                cVar1 = *sSize;
                sSize = sSize + 1;
            } while (cVar1 != '\0');
            // FUN_009f6940();
            puVar11 = (uint *)amDongle.dataBuffer;
            goto LAB_009f82f2;
        case AM_DONGLE_WTF_C3:
            if (sSize == NULL) goto LAB_009f82b7;
            do {
                cVar1 = *sSize;
                sSize = sSize + 1;
            } while (cVar1 != '\0');
        LAB_009f82d7:
            // FUN_009f6940();
            puVar11 = (uint *)amDongle.dataBuffer;
        LAB_009f82f2:
            if (local_8 < *puVar11) {
                *puVar11 = local_8;
            }
            return AM_DONGLE_STATUS_OK;
        default:
            return AM_DONGLE_STATUS_OK;
    }
    return AM_DONGLE_STATUS_ERR_PCP;
    // LAB_009f7ff2:
    //     iVar9 = (1 - (uint)bVar13) - (uint)(bVar13 != 0);
    //     goto LAB_009f7ff7;
    // LAB_009f7ce1:
    //     iVar9 = (1 - (uint)bVar13) - (uint)(bVar13 != 0);
    //     goto LAB_009f7ce6;
    // LAB_009f7c82:
    //     iVar9 = (1 - (uint)bVar13) - (uint)(bVar13 != 0);
    //     goto LAB_009f7c87;
    // LAB_009f7a21:
    //     iVar9 = (1 - (uint)bVar13) - (uint)(bVar13 != 0);
    //     goto LAB_009f7a26;
}

AM_DONGLE_STATUS amDongleSendBinary(void) {
    void *buffer;
    uint bufferLength;

    switch (amDongle.requestCode) {
        case AM_DONGLE_WTF_C0:
            bufferLength = *(uint *)amDongle.dataBuffer;
            buffer = amDongle.valueBuffer;
            break;
        case AM_DONGLE_REQUEST_BILLING_UPDATE_PLAYLIMIT:
            buffer = amDongle.dataBuffer;
            bufferLength = 128;
            break;
        case AM_DONGLE_REQUEST_BILLING_UPDATE_NEARFULL:
            buffer = amDongle.dataBuffer;
            bufferLength = 128;
            break;
        case AM_DONGLE_WTF_C3:
            bufferLength = *(uint *)amDongle.dataBuffer;
            buffer = amDongle.valueBuffer;
            break;
        default:
            amDongle.seq = AM_DONGLE_SEQ_DONE;
            amDongle.field19_0x40 = -1;
            return AM_DONGLE_STATUS_ERR_PCP;
    }

    pcpaSetSendBinaryBuffer(&amDongle.pcpa, buffer, bufferLength);
    if (pcpaSendBinary(&amDongle.pcpa, TIMEOUT_NONE) != e_pcpa_ok) {
        amDongle.seq = AM_DONGLE_SEQ_DONE;
        return AM_DONGLE_STATUS_ERR_PCP;
    }

    amDongle.seq = AM_DONGLE_SEQ_DONE;
    return AM_DONGLE_STATUS_OK;
}

AM_DONGLE_STATUS amDongleRecvBinary(void) {
    void *buffer;
    uint bufferLength;

    switch (amDongle.requestCode) {
        case AM_DONGLE_WTF_80:
            buffer = amDongle.valueBuffer;
            bufferLength = 0x10;
            break;
        case AM_DONGLE_WTF_81:
        case AM_DONGLE_REQUEST_BILLING_GET_CA_CERT:
            bufferLength = *(uint *)amDongle.dataBuffer;
            buffer = amDongle.valueBuffer;
            break;
        case AM_DONGLE_REQUEST_BILLING_GET_TRACEDATA:
            bufferLength = *amDongle.field_0x3444;
            buffer = amDongle.dataBuffer;
            break;
        case AM_DONGLE_REQUEST_BILLING_GET_SIGNATURE_PK:
        case AM_DONGLE_WTF_85:
            bufferLength = *(uint *)amDongle.dataBuffer;
            buffer = amDongle.valueBuffer;
            break;
        default:
            amDongle.seq = AM_DONGLE_SEQ_DONE;
            amDongle.field19_0x40 = -1;
            return AM_DONGLE_STATUS_ERR_PCP;
    }
    pcpaSetRecvBinaryBuffer(&amDongle.pcpa, buffer, bufferLength);
    if (pcpaRecvBinary(&amDongle.pcpa, TIMEOUT_NONE) != e_pcpa_ok) {
        amDongle.seq = AM_DONGLE_SEQ_DONE;
        return AM_DONGLE_STATUS_ERR_PCP;
    }
    amDongle.seq = AM_DONGLE_SEQ_DONE;
    return AM_DONGLE_STATUS_OK;
}

AM_DONGLE_STATUS amDongleSendAndReceiveEx(void) {
    AM_DONGLE_STATUS AVar1;

    amDongle.result = 0;
    AVar1 = amDongleOpenEx();
    if (AVar1 != AM_DONGLE_STATUS_OK) amDongle.seq = AM_DONGLE_SEQ_DONE;

    while (amDongle.seq != AM_DONGLE_SEQ_DONE) {
        e_pcpa_t err = pcpaSendRequest(&amDongle.pcpa, TIMEOUT_NONE);
        if (err != e_pcpa_ok) {
            if (amDongleDebugLevel > 0) amiDebugLog("Error : pcpaSendRequest(%d)", err);

            amDongle.field19_0x40 = -1;
            AVar1 = AM_DONGLE_STATUS_ERR_PCP;
            amDongle.seq = AM_DONGLE_SEQ_DONE;
            break;
        }
        err = pcpaRecvResponse(&amDongle.pcpa, TIMEOUT_NONE);
        if (err != e_pcpa_ok) {
            amDongle.field19_0x40 = -1;
            AVar1 = AM_DONGLE_STATUS_ERR_PCP;
            amDongle.seq = AM_DONGLE_SEQ_DONE;
            break;
        }

        AVar1 = amDongleResponseCheck();
        if (amDongle.result != AM_DONGLE_STATUS_OK) AVar1 = amDongle.result;

        if (amDongle.seq == AM_DONGLE_SEQ_BINARY) {
            err = pcpaOpenBinaryClient(&amDongle.pcpa, amDongle.data_port, TIMEOUT_NONE);
            if (err == e_pcpa_ok) {
                if ((amDongle.requestCode & 0x40) == 0)
                    amDongle.seq = AM_DONGLE_SEQ_RECV_BINARY;
                else
                    amDongle.seq = AM_DONGLE_SEQ_SEND_BINARY;

                AVar1 = AM_DONGLE_STATUS_OK;
            } else {
                amDongle.seq = AM_DONGLE_SEQ_DONE;
                AVar1 = AM_DONGLE_STATUS_ERR_PCP;
            }
        }

        if (amDongle.seq == AM_DONGLE_SEQ_SEND_BINARY) AVar1 = amDongleSendBinary();
        if (amDongle.seq == AM_DONGLE_SEQ_RECV_BINARY) AVar1 = amDongleRecvBinary();
    }

    pcpaCloseBinary(&amDongle.pcpa);
    pcpaClose(&amDongle.pcpa);
    amDongle.seq = AM_DONGLE_SEQ_NONE;
    amDongle.result = 0;
    return AVar1;
}

AM_DONGLE_STATUS amDongleExit(void) {
    if (!amDongle.m_init) return AM_DONGLE_STATUS_ERR_NO_INIT;

    pcpaClose(&amDongle.pcpa);
    memset(&amDongle, 0, sizeof amDongle);
    amDongle.m_init = FALSE;
    return AM_DONGLE_STATUS_OK;
}

// TODO: These two functions for loading tables
int amDongleLoadDsTable(FILE *fp) { return 0; }
int amDongleLoadSsdTable(FILE *fp) { return 0; }

AM_DONGLE_STATUS amDongleSetAuthConfig(char *authConfig) {
    if (amDongle.m_init == 0) return AM_DONGLE_STATUS_ERR_NO_INIT;
    if (authConfig == NULL) return AM_DONGLE_STATUS_ERR_INVALID_PARAM;

    if (strcmp(authConfig, "toolmode") != 0) {
        HANDLE hFile = CreateFileA(authConfig, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) return AM_DONGLE_STATUS_ERR_INVALID_PARAM;
        CloseHandle(hFile);

        FILE *fp;
        fopen_s(&fp, authConfig, "rb");
        if (fp == NULL) return AM_DONGLE_STATUS_ERR_INVALID_PARAM;

        if (!amDongleLoadDsTable(fp) || !amDongleLoadSsdTable(fp)) {
            fclose(fp);
            return AM_DONGLE_STATUS_ERR_INVALID_PARAM;
        }
        fclose(fp);
    }

    amDongle.auth_ready = 1;
    amDongle.result = 0;
    return AM_DONGLE_STATUS_OK;
}

// PCP wrappers
AM_DONGLE_STATUS amDongleBillingGetKeychipId(char *keychipId, AM_DONGLE_BLOCKING blocking) {
    if (!amDongle.m_init) return AM_DONGLE_STATUS_ERR_NO_INIT;
    if (!amDongle.auth_ready) return AM_DONGLE_STATUS_ERR_AUTH_READY;
    if (!amDongle.done_init || !amDongle.available) return AM_DONGLE_STATUS_NG;
    if (amDongle.seq != AM_DONGLE_SEQ_NONE) return AM_DONGLE_STATUS_BUSY;

    if (keychipId == NULL) return AM_DONGLE_STATUS_ERR_INVALID_PARAM;

    pcp_send_data_t *packet = pcpaSetSendPacket(&amDongle.pcpa, "keychip.billing.keyid", "?");
    if (packet == NULL && amDongleDebugLevel > 0)
        amiDebugLog("Error: pcpaSetSendPacket return NULL");
    packet = pcpaAddSendPacket(&amDongle.pcpa, "cache", "1");
    if (packet == NULL && amDongleDebugLevel > 0)
        amiDebugLog("Error: pcpaAddSendPacket return NULL");

    amDongle.valueBuffer = keychipId;
    amDongle.requestCode = AM_DONGLE_REQUEST_BILLING_GET_KEYCHIP_ID;

    if (blocking == AM_DONGLE_BLOCK) return amDongleSendAndReceiveEx();
    return amDongleOpen();
}
AM_DONGLE_STATUS amDongleGetGameId(char *gameId, AM_DONGLE_BLOCKING blocking) {
    if (!amDongle.m_init) return AM_DONGLE_STATUS_ERR_NO_INIT;
    if (!amDongle.auth_ready) return AM_DONGLE_STATUS_ERR_AUTH_READY;
    if (!amDongle.done_init || !amDongle.available) return AM_DONGLE_STATUS_NG;
    if (amDongle.seq != AM_DONGLE_SEQ_NONE) return AM_DONGLE_STATUS_BUSY;

    if (gameId == NULL) return AM_DONGLE_STATUS_ERR_INVALID_PARAM;

    pcp_send_data_t *packet = pcpaSetSendPacket(&amDongle.pcpa, "keychip.appboot.gameid", "?");
    if (packet == NULL && amDongleDebugLevel > 0)
        amiDebugLog("Error: pcpaSetSendPacket return NULL");
    packet = pcpaAddSendPacket(&amDongle.pcpa, "cache", "1");
    if (packet == NULL && amDongleDebugLevel > 0)
        amiDebugLog("Error: pcpaAddSendPacket return NULL");

    amDongle.valueBuffer = gameId;
    amDongle.requestCode = AM_DONGLE_REQUEST_GET_GAME_ID;

    if (blocking == AM_DONGLE_BLOCK) return amDongleSendAndReceiveEx();
    return amDongleOpen();
}
AM_DONGLE_STATUS amDongleGetPlatformId(char *platformId, AM_DONGLE_BLOCKING blocking) {
    if (!amDongle.m_init) return AM_DONGLE_STATUS_ERR_NO_INIT;
    if (!amDongle.auth_ready) return AM_DONGLE_STATUS_ERR_AUTH_READY;
    if (!amDongle.done_init || !amDongle.available) return AM_DONGLE_STATUS_NG;
    if (amDongle.seq != AM_DONGLE_SEQ_NONE) return AM_DONGLE_STATUS_BUSY;

    if (platformId == NULL) return AM_DONGLE_STATUS_ERR_INVALID_PARAM;

    pcp_send_data_t *packet = pcpaSetSendPacket(&amDongle.pcpa, "keychip.appboot.platformid", "?");
    if (packet == NULL && amDongleDebugLevel > 0)
        amiDebugLog("Error: pcpaSetSendPacket return NULL");
    packet = pcpaAddSendPacket(&amDongle.pcpa, "cache", "1");
    if (packet == NULL && amDongleDebugLevel > 0)
        amiDebugLog("Error: pcpaAddSendPacket return NULL");

    amDongle.valueBuffer = platformId;
    amDongle.requestCode = AM_DONGLE_REQUEST_GET_PLATFORM_ID;

    if (blocking == AM_DONGLE_BLOCK) return amDongleSendAndReceiveEx();
    return amDongleOpen();
}
AM_DONGLE_STATUS amDongleGetSystemFlag(unsigned char *systemFlag, AM_DONGLE_BLOCKING blocking) {
    if (!amDongle.m_init) return AM_DONGLE_STATUS_ERR_NO_INIT;
    if (!amDongle.auth_ready) return AM_DONGLE_STATUS_ERR_AUTH_READY;
    if (!amDongle.done_init || !amDongle.available) return AM_DONGLE_STATUS_NG;
    if (amDongle.seq != AM_DONGLE_SEQ_NONE) return AM_DONGLE_STATUS_BUSY;

    if (systemFlag == NULL) return AM_DONGLE_STATUS_ERR_INVALID_PARAM;

    pcp_send_data_t *packet = pcpaSetSendPacket(&amDongle.pcpa, "keychip.appboot.systemflag", "?");
    if (packet == NULL && amDongleDebugLevel > 0)
        amiDebugLog("Error: pcpaSetSendPacket return NULL");
    packet = pcpaAddSendPacket(&amDongle.pcpa, "cache", "1");
    if (packet == NULL && amDongleDebugLevel > 0)
        amiDebugLog("Error: pcpaAddSendPacket return NULL");

    amDongle.valueBuffer = systemFlag;
    amDongle.requestCode = AM_DONGLE_REQUEST_GET_SYSTEMFLAG;

    if (blocking == AM_DONGLE_BLOCK) return amDongleSendAndReceiveEx();
    return amDongleOpen();
}
AM_DONGLE_STATUS amDongleGetModelType(unsigned char *modelType, AM_DONGLE_BLOCKING blocking) {
    if (!amDongle.m_init) return AM_DONGLE_STATUS_ERR_NO_INIT;
    if (!amDongle.auth_ready) return AM_DONGLE_STATUS_ERR_AUTH_READY;
    if (!amDongle.done_init || !amDongle.available) return AM_DONGLE_STATUS_NG;
    if (amDongle.seq != AM_DONGLE_SEQ_NONE) return AM_DONGLE_STATUS_BUSY;

    if (modelType == NULL) return AM_DONGLE_STATUS_ERR_INVALID_PARAM;

    pcp_send_data_t *packet = pcpaSetSendPacket(&amDongle.pcpa, "keychip.appboot.modeltype", "?");
    if (packet == NULL && amDongleDebugLevel > 0)
        amiDebugLog("Error: pcpaSetSendPacket return NULL");
    packet = pcpaAddSendPacket(&amDongle.pcpa, "cache", "1");
    if (packet == NULL && amDongleDebugLevel > 0)
        amiDebugLog("Error: pcpaAddSendPacket return NULL");

    amDongle.valueBuffer = modelType;
    amDongle.requestCode = AM_DONGLE_REQUEST_GET_MODEL_TYPE;

    if (blocking == AM_DONGLE_BLOCK) return amDongleSendAndReceiveEx();
    return amDongleOpen();
}
AM_DONGLE_STATUS amDongleGetRegion(unsigned char *region, AM_DONGLE_BLOCKING blocking) {
    if (!amDongle.m_init) return AM_DONGLE_STATUS_ERR_NO_INIT;
    if (!amDongle.auth_ready) return AM_DONGLE_STATUS_ERR_AUTH_READY;
    if (!amDongle.done_init || !amDongle.available) return AM_DONGLE_STATUS_NG;
    if (amDongle.seq != AM_DONGLE_SEQ_NONE) return AM_DONGLE_STATUS_BUSY;

    if (region == NULL) return AM_DONGLE_STATUS_ERR_INVALID_PARAM;

    pcp_send_data_t *packet = pcpaSetSendPacket(&amDongle.pcpa, "keychip.appboot.region", "?");
    if (packet == NULL && amDongleDebugLevel > 0)
        amiDebugLog("Error: pcpaSetSendPacket return NULL");
    packet = pcpaAddSendPacket(&amDongle.pcpa, "cache", "1");
    if (packet == NULL && amDongleDebugLevel > 0)
        amiDebugLog("Error: pcpaAddSendPacket return NULL");

    amDongle.valueBuffer = region;
    amDongle.requestCode = AM_DONGLE_REQUEST_GET_REGION;

    if (blocking == AM_DONGLE_BLOCK) return amDongleSendAndReceiveEx();
    return amDongleOpen();
}
AM_DONGLE_STATUS amDongleGetNetworkAddress(unsigned int *networkAddress,
                                           AM_DONGLE_BLOCKING blocking) {
    if (!amDongle.m_init) return AM_DONGLE_STATUS_ERR_NO_INIT;
    if (!amDongle.auth_ready) return AM_DONGLE_STATUS_ERR_AUTH_READY;
    if (!amDongle.done_init || !amDongle.available) return AM_DONGLE_STATUS_NG;
    if (amDongle.seq != AM_DONGLE_SEQ_NONE) return AM_DONGLE_STATUS_BUSY;

    if (networkAddress == NULL) return AM_DONGLE_STATUS_ERR_INVALID_PARAM;

    pcp_send_data_t *packet = pcpaSetSendPacket(&amDongle.pcpa, "keychip.appboot.networkaddr", "?");
    if (packet == NULL && amDongleDebugLevel > 0)
        amiDebugLog("Error: pcpaSetSendPacket return NULL");
    packet = pcpaAddSendPacket(&amDongle.pcpa, "cache", "1");
    if (packet == NULL && amDongleDebugLevel > 0)
        amiDebugLog("Error: pcpaAddSendPacket return NULL");

    amDongle.valueBuffer = networkAddress;
    amDongle.requestCode = AM_DONGLE_REQUEST_GET_NETWORK_ADDRESS;

    if (blocking == AM_DONGLE_BLOCK) return amDongleSendAndReceiveEx();
    return amDongleOpen();
}
AM_DONGLE_STATUS amDongleGetVersion(unsigned short *version, AM_DONGLE_BLOCKING blocking) {
    if (!amDongle.m_init) return AM_DONGLE_STATUS_ERR_NO_INIT;
    if (!amDongle.auth_ready) return AM_DONGLE_STATUS_ERR_AUTH_READY;
    if (!amDongle.done_init || !amDongle.available) return AM_DONGLE_STATUS_NG;
    if (amDongle.seq != AM_DONGLE_SEQ_NONE) return AM_DONGLE_STATUS_BUSY;

    if (version == NULL) return AM_DONGLE_STATUS_ERR_INVALID_PARAM;

    pcp_send_data_t *packet = pcpaSetSendPacket(&amDongle.pcpa, "keychip.version", "?");
    if (packet == NULL && amDongleDebugLevel > 0)
        amiDebugLog("Error: pcpaSetSendPacket return NULL");
    packet = pcpaAddSendPacket(&amDongle.pcpa, "device", "n2");
    if (packet == NULL && amDongleDebugLevel > 0)
        amiDebugLog("Error: pcpaAddSendPacket return NULL");
    packet = pcpaAddSendPacket(&amDongle.pcpa, "cache", "1");
    if (packet == NULL && amDongleDebugLevel > 0)
        amiDebugLog("Error: pcpaAddSendPacket return NULL");

    amDongle.valueBuffer = version;
    amDongle.requestCode = AM_DONGLE_REQUEST_GET_VERSION;

    if (blocking == AM_DONGLE_BLOCK) return amDongleSendAndReceiveEx();
    return amDongleOpen();
}
AM_DONGLE_STATUS amDongleGetPicVersion(unsigned short *picVersion, AM_DONGLE_BLOCKING blocking) {
    if (!amDongle.m_init) return AM_DONGLE_STATUS_ERR_NO_INIT;
    if (!amDongle.auth_ready) return AM_DONGLE_STATUS_ERR_AUTH_READY;
    if (!amDongle.done_init || !amDongle.available) return AM_DONGLE_STATUS_NG;
    if (amDongle.seq != AM_DONGLE_SEQ_NONE) return AM_DONGLE_STATUS_BUSY;

    if (picVersion == NULL) return AM_DONGLE_STATUS_ERR_INVALID_PARAM;

    pcp_send_data_t *packet = pcpaSetSendPacket(&amDongle.pcpa, "keychip.version", "?");
    if (packet == NULL && amDongleDebugLevel > 0)
        amiDebugLog("Error: pcpaSetSendPacket return NULL");
    packet = pcpaAddSendPacket(&amDongle.pcpa, "cache", "0");
    if (packet == NULL && amDongleDebugLevel > 0)
        amiDebugLog("Error: pcpaAddSendPacket return NULL");

    amDongle.valueBuffer = picVersion;
    amDongle.requestCode = AM_DONGLE_REQUEST_GET_VERSION;

    if (blocking == AM_DONGLE_BLOCK) return amDongleSendAndReceiveEx();
    return amDongleOpen();
}

AM_DONGLE_STATUS amDongleEncrypt(unsigned char *pt, unsigned char *ct,
                                 AM_DONGLE_BLOCKING blocking) {
    if (!amDongle.m_init) return AM_DONGLE_STATUS_ERR_NO_INIT;
    if (!amDongle.auth_ready) return AM_DONGLE_STATUS_ERR_AUTH_READY;
    if (!amDongle.done_init || !amDongle.available) return AM_DONGLE_STATUS_NG;
    if (amDongle.seq != AM_DONGLE_SEQ_NONE) return AM_DONGLE_STATUS_BUSY;

    if (pt == NULL || ct == NULL) return AM_DONGLE_STATUS_ERR_INVALID_PARAM;

    char ptHex[32];
    bin_to_hex(ptHex, pt, 16);
    pcp_send_data_t *packet = pcpaSetSendPacket(&amDongle.pcpa, "keychip.encrypt", ptHex);
    if (packet == NULL && amDongleDebugLevel > 0)
        amiDebugLog("Error: pcpaSetSendPacket return NULL");

    amDongle.valueBuffer = ct;
    amDongle.requestCode = AM_DONGLE_REQUEST_ENCRYPT;

    if (blocking == AM_DONGLE_BLOCK) return amDongleSendAndReceiveEx();
    return amDongleOpen();
}
AM_DONGLE_STATUS amDongleDecrypt(unsigned char *ct, unsigned char *pt,
                                 AM_DONGLE_BLOCKING blocking) {
    if (!amDongle.m_init) return AM_DONGLE_STATUS_ERR_NO_INIT;
    if (!amDongle.auth_ready) return AM_DONGLE_STATUS_ERR_AUTH_READY;
    if (!amDongle.done_init || !amDongle.available) return AM_DONGLE_STATUS_NG;
    if (amDongle.seq != AM_DONGLE_SEQ_NONE) return AM_DONGLE_STATUS_BUSY;

    if (ct == NULL || pt == NULL) return AM_DONGLE_STATUS_ERR_INVALID_PARAM;

    char ctHex[32];
    bin_to_hex(ctHex, ct, 16);
    pcp_send_data_t *packet = pcpaSetSendPacket(&amDongle.pcpa, "keychip.decrypt", ctHex);
    if (packet == NULL && amDongleDebugLevel > 0)
        amiDebugLog("Error: pcpaSetSendPacket return NULL");

    amDongle.valueBuffer = pt;
    amDongle.requestCode = AM_DONGLE_REQUEST_DECRYPT;

    if (blocking == AM_DONGLE_BLOCK) return amDongleSendAndReceiveEx();
    return amDongleOpen();
}

AM_DONGLE_STATUS amDongleBillingGetPlayCount(unsigned int *playCount, AM_DONGLE_BLOCKING blocking) {
    if (!amDongle.m_init) return AM_DONGLE_STATUS_ERR_NO_INIT;
    if (!amDongle.auth_ready) return AM_DONGLE_STATUS_ERR_AUTH_READY;
    if (!amDongle.done_init || !amDongle.available) return AM_DONGLE_STATUS_NG;
    if (amDongle.seq != AM_DONGLE_SEQ_NONE) return AM_DONGLE_STATUS_BUSY;
    if (playCount == NULL) return AM_DONGLE_STATUS_ERR_INVALID_PARAM;

    pcp_send_data_t *send_data =
        pcpaSetSendPacket(&amDongle.pcpa, "keychip.billing.playcount", "?");
    if (send_data == NULL && amDongleDebugLevel > 0) {
        amiDebugLog("Error: pcpaSetSendPacket return NULL");
    }
    send_data = pcpaAddSendPacket(&amDongle.pcpa, "cache", "1");
    if (send_data == NULL && amDongleDebugLevel > 0) {
        amiDebugLog("Error: pcpaAddSendPacket return NULL");
    }
    amDongle.valueBuffer = playCount;
    amDongle.requestCode = AM_DONGLE_REQUEST_BILLING_GET_PLAYCOUNT;
    if (blocking == AM_DONGLE_BLOCK) {
        return amDongleSendAndReceiveEx();
    }
    return amDongleOpen();
}
AM_DONGLE_STATUS amDongleBillingGetPlayLimit(unsigned int *playLimit, AM_DONGLE_BLOCKING blocking) {
    if (!amDongle.m_init) return AM_DONGLE_STATUS_ERR_NO_INIT;
    if (!amDongle.auth_ready) return AM_DONGLE_STATUS_ERR_AUTH_READY;
    if (!amDongle.done_init || !amDongle.available) return AM_DONGLE_STATUS_NG;
    if (amDongle.seq != AM_DONGLE_SEQ_NONE) return AM_DONGLE_STATUS_BUSY;
    if (playLimit == NULL) return AM_DONGLE_STATUS_ERR_INVALID_PARAM;

    pcp_send_data_t *send_data =
        pcpaSetSendPacket(&amDongle.pcpa, "keychip.billing.playlimit", "?");
    if (send_data == NULL && amDongleDebugLevel > 0) {
        amiDebugLog("Error: pcpaSetSendPacket return NULL");
    }
    send_data = pcpaAddSendPacket(&amDongle.pcpa, "cache", "1");
    if (send_data == NULL && amDongleDebugLevel > 0) {
        amiDebugLog("Error: pcpaAddSendPacket return NULL");
    }
    amDongle.valueBuffer = playLimit;
    amDongle.requestCode = AM_DONGLE_REQUEST_BILLING_GET_PLAYLIMIT;
    if (blocking == AM_DONGLE_BLOCK) {
        return amDongleSendAndReceiveEx();
    }
    return amDongleOpen();
}
AM_DONGLE_STATUS amDongleBillingGetNearfull(unsigned int *nearfull, AM_DONGLE_BLOCKING blocking) {
    if (!amDongle.m_init) return AM_DONGLE_STATUS_ERR_NO_INIT;
    if (!amDongle.auth_ready) return AM_DONGLE_STATUS_ERR_AUTH_READY;
    if (!amDongle.done_init || !amDongle.available) return AM_DONGLE_STATUS_NG;
    if (amDongle.seq != AM_DONGLE_SEQ_NONE) return AM_DONGLE_STATUS_BUSY;
    if (nearfull == NULL) return AM_DONGLE_STATUS_ERR_INVALID_PARAM;

    pcp_send_data_t *send_data = pcpaSetSendPacket(&amDongle.pcpa, "keychip.billing.nearfull", "?");
    if (send_data == NULL && amDongleDebugLevel > 0) {
        amiDebugLog("Error: pcpaSetSendPacket return NULL");
    }
    send_data = pcpaAddSendPacket(&amDongle.pcpa, "cache", "1");
    if (send_data == NULL && amDongleDebugLevel > 0) {
        amiDebugLog("Error: pcpaAddSendPacket return NULL");
    }
    amDongle.valueBuffer = nearfull;
    amDongle.requestCode = AM_DONGLE_REQUEST_BILLING_GET_NEARFULL;
    if (blocking == AM_DONGLE_BLOCK) {
        return amDongleSendAndReceiveEx();
    }
    return amDongleOpen();
}

AM_DONGLE_STATUS amDongleBillingAddPlayCount(void *playCount, AM_DONGLE_BLOCKING blocking) {
    if (!amDongle.m_init) return AM_DONGLE_STATUS_ERR_NO_INIT;
    if (!amDongle.auth_ready) return AM_DONGLE_STATUS_ERR_AUTH_READY;
    if (!amDongle.done_init || !amDongle.available) return AM_DONGLE_STATUS_NG;
    if (amDongle.seq != AM_DONGLE_SEQ_NONE) return AM_DONGLE_STATUS_BUSY;
    if (playCount == NULL) return AM_DONGLE_STATUS_ERR_INVALID_PARAM;

    pcp_send_data_t *send_data =
        pcpaSetSendPacket(&amDongle.pcpa, "keychip.billing.playcount", "1");
    if (send_data == NULL && amDongleDebugLevel > 0) {
        amiDebugLog("Error: pcpaSetSendPacket return NULL");
    }
    send_data = pcpaAddSendPacket(&amDongle.pcpa, "cache", "1");
    if (send_data == NULL && amDongleDebugLevel > 0) {
        amiDebugLog("Error: pcpaAddSendPacket return NULL");
    }
    amDongle.valueBuffer = playCount;
    amDongle.requestCode = 0xf;
    if (blocking == AM_DONGLE_BLOCK) {
        return amDongleSendAndReceiveEx();
    }
    return amDongleOpen();
}
