#include "amInstall.h"

#include "../util/hex.h"

AM_LIB_C_HEADER(amInstall, AM_INSTALL)

/*********************************************************/
AM_INSTALL_STATUS amInstallResponseCheckQuerySlotStatus(AM_INSTALL_SLOT_STATUS *slotStatus) {
    return AM_INSTALL_STATUS_OK;
}
int amInstallResponseCheckQuerySemStatus(void) { return -1; }
int amInstallResponseCheckQueryAppStatus(void) { return -1; }
AM_INSTALL_STATUS amInstallResponseCheckQuerySpd(void *param_1) { return AM_INSTALL_STATUS_OK; }
AM_INSTALL_STATUS amInstallResponseCheckQueryAppdataStatus(int *param_1) {
    return AM_INSTALL_STATUS_OK;
}
AM_INSTALL_STATUS amInstallSuicideSub(char *keyword) { return AM_INSTALL_STATUS_OK; }
/*********************************************************/

bool amInstallPcpaGetUlonglong(unsigned long long *dest, char *keyword) {
    if (keyword == NULL || dest == NULL) return false;
    char *command = pcpaGetCommand(&amInstall.m_pcp, keyword);
    if (command == NULL) return false;

    errno = 0;
    char *end = NULL;
    unsigned long long value = strtoull(command, &end, 0);
    if (errno == ERANGE || end == NULL || end[0] != '\0') return false;
    *dest = value;
    return true;
}

AM_INSTALL_STATUS amInstallResponseCheckQueryBr(AM_INSTALL_BOOT_RECORD *bootRecord) {
    if (bootRecord == NULL) return AM_INSTALL_STATUS_ERR_INVALID_PARAM;

    ZeroMemory(bootRecord, sizeof bootRecord);

    amInstallPcpaGetUlonglong(&bootRecord->epbr, "epbr");
    amInstallPcpaGetUlonglong(&bootRecord->original0, "original0");
    amInstallPcpaGetUlonglong(&bootRecord->original1, "original1");
    amInstallPcpaGetUlonglong(&bootRecord->patch0, "patch0");
    amInstallPcpaGetUlonglong(&bootRecord->patch1, "patch1");
    amInstallPcpaGetUlonglong(&bootRecord->os, "os");

    return AM_INSTALL_STATUS_OK;
}

/*********************************************************/

AM_INSTALL_STATUS amInstallInit(void) {
    if (amInstall.m_init) return AM_INSTALL_STATUS_ERR_ALREADY_INIT;

    amInstall.m_seq = AM_INSTALL_SEQ_NONE;
    amInstall.m_nextSeq = AM_INSTALL_SEQ_INVALID;
    amInstall.m_requestCode = 0;
    amInstall.m_result = AM_INSTALL_STATUS_OK;
    amInstall.m_sendBufferLen = 0;
    amInstall.m_semid = 0;
    amInstall.m_cmdPort = 40102;
    amInstall.m_dataPort = 40103;

    e_pcpa_t err = pcpaInitStream(&amInstall.m_pcp);
    if (err == e_pcpa_wsa_noinit) {
        if (amInstallDebugLevel > 0) amiDebugLog("Error: Winsocket2 Library is not ready.");
        return AM_INSTALL_STATUS_ERR_PRECONDITION;
    }
    if (err != e_pcpa_ok) {
        if (amInstallDebugLevel > 0)
            amiDebugLog("Error: System error happened in pcp stream. ErrorCode = %d.", err);
        return AM_INSTALL_STATUS_ERR_SYS;
    }

    amInstall.m_init = true;
    return AM_INSTALL_STATUS_OK;
}

AM_INSTALL_STATUS amInstallUpdate(void) {
    AM_INSTALL_STATUS status;

    if (!amInstall.m_init) return AM_INSTALL_STATUS_ERR_NO_INIT;

    switch (amInstall.m_seq) {
        case AM_INSTALL_SEQ_BUSY:
            status = amInstallBusy();
            break;
        case AM_INSTALL_SEQ_SEND_REQUEST:
            status = amInstallSendRequest();
            if (status != AM_INSTALL_STATUS_OK) {
                amInstall.m_result = status;
                return AM_INSTALL_STATUS_OK;
            }
            return AM_INSTALL_STATUS_OK;
        case AM_INSTALL_SEQ_RECV_RESPONSE:
            status = amInstallRecvResponse();
            break;
        case AM_INSTALL_SEQ_CHECK_RESPONSE:
            status = amInstallResponseCheck();
            if (status != AM_INSTALL_STATUS_OK) {
                amInstall.m_result = status;
                return AM_INSTALL_STATUS_OK;
            }
            return AM_INSTALL_STATUS_OK;
        case AM_INSTALL_SEQ_OPEN_BINARY:
            status = amInstallOpenBinary();
            break;
        case AM_INSTALL_SEQ_SEND_BINARY:
            status = amInstallSendBinaryBuffer(1);
            if (status != AM_INSTALL_STATUS_OK) {
                amInstall.m_result = status;
                return AM_INSTALL_STATUS_OK;
            }
            return AM_INSTALL_STATUS_OK;
        case AM_INSTALL_SEQ_EXIT:
            amInstallExit();
            amInstall.m_result = AM_INSTALL_STATUS_OK;
            return AM_INSTALL_STATUS_OK;
        default:
            return AM_INSTALL_STATUS_OK;
    }

    if (status != AM_INSTALL_STATUS_OK) {
        amInstall.m_result = status;
        return AM_INSTALL_STATUS_OK;
    }
    return AM_INSTALL_STATUS_OK;
}

AM_INSTALL_STATUS amInstallBusy(void) {
    e_pcpa_t err = pcpaIsBusy(&amInstall.m_pcp, 0);
    if (err == e_pcpa_ok) {
        amInstall.m_seq = amInstall.m_nextSeq;
        amInstall.m_nextSeq = AM_INSTALL_SEQ_INVALID;
    } else if (err != e_pcpa_to) {
        if (amInstallDebugLevel > 0)
            amiDebugLog("pcpaIsBusy() Error(%d), NextStatus = %d", err, amInstall.m_nextSeq);
        amInstall.m_nextSeq = AM_INSTALL_SEQ_INVALID;
        amInstall.m_seq = AM_INSTALL_SEQ_EXIT;

        if (err == e_pcpa_no_server) return AM_INSTALL_STATUS_ERR_NO_SERVER;
        return AM_INSTALL_STATUS_ERR_PCP;
    }
    return AM_INSTALL_STATUS_OK;
}

AM_INSTALL_STATUS amInstallSendRequest(void) {
    e_pcpa_t err = pcpaSendRequest(&amInstall.m_pcp, 0);
    if (err == e_pcpa_ok) {
        amInstall.m_seq = AM_INSTALL_SEQ_RECV_RESPONSE;
        return AM_INSTALL_STATUS_OK;
    }
    if (err == e_pcpa_to) {
        amInstall.m_seq = AM_INSTALL_SEQ_BUSY;
        amInstall.m_nextSeq = AM_INSTALL_SEQ_RECV_RESPONSE;
        return AM_INSTALL_STATUS_OK;
    }

    if (amInstallDebugLevel > 0) amiDebugLog("pcpaSendRequest() Error!!");
    amInstall.m_seq = AM_INSTALL_SEQ_EXIT;
    return AM_INSTALL_STATUS_ERR_PCP;
}

AM_INSTALL_STATUS amInstallRecvResponse(void) {
    e_pcpa_t err = pcpaRecvResponse(&amInstall.m_pcp, 0);
    if (err == e_pcpa_ok) {
        amInstall.m_seq = AM_INSTALL_SEQ_CHECK_RESPONSE;
        return AM_INSTALL_STATUS_OK;
    }
    if (err == e_pcpa_to) {
        amInstall.m_seq = AM_INSTALL_SEQ_BUSY;
        amInstall.m_nextSeq = AM_INSTALL_SEQ_CHECK_RESPONSE;
        return AM_INSTALL_STATUS_OK;
    }
    amInstall.m_seq = AM_INSTALL_SEQ_EXIT;
    return AM_INSTALL_STATUS_ERR_PCP;
}

AM_INSTALL_STATUS amInstallCodeToStatus(void) {
    char *sResult = pcpaGetCommand(&amInstall.m_pcp, "result");
    char *sCode = pcpaGetCommand(&amInstall.m_pcp, "code");
    if (sResult == NULL) {
        return AM_INSTALL_STATUS_ERR_PCP;
    }
    if (sCode != NULL) {
        switch (atoi(sCode)) {
            case 0:
                return AM_INSTALL_STATUS_OK;
            case 1:
                return AM_INSTALL_STATUS_ERR_NO_STORAGE;
            case 2:
                return AM_INSTALL_STATUS_ERR_FORMAT;
            case 4:
                return AM_INSTALL_STATUS_ERR_EX_PARTITION;
            case 5:
                return AM_INSTALL_STATUS_ERR_READ_STORAGE;
            case 6:
                return AM_INSTALL_STATUS_ERR_WRITE_STORAGE;
            case 7:
                return AM_INSTALL_STATUS_ERR_SPD_CRC;
            case 8:
                return AM_INSTALL_STATUS_ERR_SPD_VERSION;
            case 20:
                return AM_INSTALL_STATUS_ERR_SEMAPHORE_ID;
            case 21:
                return AM_INSTALL_STATUS_ERR_GET_SEMAPHORE;
            case 22:
                return AM_INSTALL_STATUS_ERR_INVALID_LOCK_ID;
            case 40:
                return AM_INSTALL_STATUS_ERR_NO_SLOT;
            case 41:
                return AM_INSTALL_STATUS_ERR_READ_SLOT;
            case 42:
                return AM_INSTALL_STATUS_ERR_WRITE_SLOT;
            case 60:
                return AM_INSTALL_STATUS_ERR_DIFF_APPLICATION;
            case 61:
                return AM_INSTALL_STATUS_ERR_APPLICATION_SIZE;
            case 62:
                return AM_INSTALL_STATUS_ERR_SEGMENT_OFFSET;
            case 63:
                return AM_INSTALL_STATUS_ERR_UPDATE_STATUS;
            case 64:
                return AM_INSTALL_STATUS_ERR_REQUEST;
        }
    }
    if (strcmp(sResult, "success") == 0) return AM_INSTALL_STATUS_OK;

    if (strcmp(sResult, "invalid_parameter") != 0) {
        if (strcmp(sResult, "invalid_request") != 0) {
            return AM_INSTALL_STATUS_ERR_PCP;
        }
    }
    return AM_INSTALL_STATUS_ERR_INVALID_COMMAND;
}

bool amInstallPcpaGetInt(char *keyword, unsigned int *dest) {
    if (keyword == NULL || dest == NULL) return false;
    char *command = pcpaGetCommand(&amInstall.m_pcp, keyword);
    if (command == NULL) return false;

    errno = 0;
    char *end = NULL;
    unsigned int value = strtoul(command, &end, 0);
    if (errno == ERANGE || end == NULL || end[0] != '\0') return false;
    *dest = value;
    return true;
}

AM_INSTALL_STATUS amInstallTaskRequest(char *keyword) {
    pcp_send_data_t *sendData;

    if (strcmp(keyword, "query_appdata_status") == 0) {
        int status;
        if (amInstallResponseCheckQueryAppdataStatus(&status) != AM_INSTALL_STATUS_OK)
            return AM_INSTALL_STATUS_ERR_PCP;

        bool failed = ((status == 1) || (status == 2));
        if (status == 0) {
            sendData = pcpaSetSendPacket(&amInstall.m_pcp, "request", "check_appdata");
            if (sendData == NULL && amInstallDebugLevel > 0)
                amiDebugLog("Error: pcpaSetSendPacket return NULL\n");
            sendData = pcpaAddSendPacket(&amInstall.m_pcp, "execute", "1");
            if (sendData == NULL && amInstallDebugLevel > 0)
                amiDebugLog("Error: pcpaAddSendPacket return NULL\n");

            amInstall.m_seq = AM_INSTALL_SEQ_SEND_REQUEST;
        }
        if (!failed) {
            return AM_INSTALL_STATUS_OK;
        }
    } else {
        if (strcmp(keyword, "check_appdata") != 0 && strcmp(keyword, "format_appdata") != 0)
            return AM_INSTALL_STATUS_ERR_PCP;
    }

    sendData = pcpaSetSendPacket(&amInstall.m_pcp, "request", "query_appdata_status");
    if (sendData == 0 && amInstallDebugLevel > 0)
        amiDebugLog("Error: pcpaSetSendPacket return NULL\n");

    amInstall.m_seq = AM_INSTALL_SEQ_SEND_REQUEST;
    return AM_INSTALL_STATUS_OK;
}

AM_INSTALL_STATUS amInstallResponseCheck(void) {
    char *realKeyword = pcpaGetKeyword(&amInstall.m_pcp, 0);
    char *keyword = pcpaGetCommand(&amInstall.m_pcp, "response");
    amInstall.m_seq = AM_INSTALL_SEQ_EXIT;
    if (realKeyword == NULL) {
        if (amInstallDebugLevel > 0) amiDebugLog("Response Error!!! No Keyword.");
        return AM_INSTALL_STATUS_ERR_PCP;
    }

    if (realKeyword[0] == '?') return AM_INSTALL_STATUS_ERR_NO_COMMAND;
    if (keyword == NULL) {
        if (amInstallDebugLevel > 0) amiDebugLog("Error : No Response");
        return AM_INSTALL_STATUS_ERR_PCP;
    }

    AM_INSTALL_STATUS response_error = amInstallCodeToStatus();
    if (response_error != AM_INSTALL_STATUS_OK) return response_error;

    switch (amInstall.m_requestCode) {
        case AM_INSTALL_REQUEST_QUERY_SLOT_STATUS:
            if (strcmp(keyword, "query_slot_status") != 0) {
                amInstall.m_result = AM_INSTALL_STATUS_ERR_PCP;
                return AM_INSTALL_STATUS_OK;
            }
            amInstallResponseCheckQuerySlotStatus((AM_INSTALL_SLOT_STATUS *)amInstall.m_value);
            return AM_INSTALL_STATUS_OK;
        case AM_INSTALL_REQUEST_INSTALL:
            if (strcmp(keyword, "install") != 0) return AM_INSTALL_STATUS_ERR_PCP;
            unsigned int port = 40103;
            amInstallPcpaGetInt("port", &port);
            amInstall = amInstall;
            amInstall.m_dataPort = port & 0xffff;
            amInstall.m_seq = AM_INSTALL_SEQ_OPEN_BINARY;
            return AM_INSTALL_STATUS_OK;
        case AM_INSTALL_REQUEST_UNINSTALL:
            if (strcmp(keyword, "uninstall") != 0) return AM_INSTALL_STATUS_ERR_PCP;
            return AM_INSTALL_STATUS_OK;
        case AM_INSTALL_REQUEST_CHECK:
            if (strcmp(keyword, "check") != 0) return AM_INSTALL_STATUS_ERR_PCP;
            return AM_INSTALL_STATUS_OK;
        case AM_INSTALL_REQUEST_QUERY_APPLICATION_STATUS:
            if (strcmp(keyword, "query_application_status") != 0) return AM_INSTALL_STATUS_ERR_PCP;

            *(unsigned int *)amInstall.m_value = amInstallResponseCheckQueryAppStatus();
            amInstallPcpaGetInt("busyslot", (unsigned int *)amInstall.Unk51c);
            return AM_INSTALL_STATUS_OK;
        case AM_INSTALL_REQUEST_SET_APPLICATION_STATUS:
            if (strcmp(keyword, "set_application_status") != 0) return AM_INSTALL_STATUS_ERR_PCP;

            if (pcpaGetCommand(&amInstall.m_pcp, "lockid") == NULL) return AM_INSTALL_STATUS_OK;
            amInstallPcpaGetInt("lockid", (unsigned int *)amInstall.m_value);
            return AM_INSTALL_STATUS_OK;
        case AM_INSTALL_REQUEST_QUERY_SBR_BOOTSLOT:
            if (strcmp(keyword, "query_sbr_bootslot") != 0) return AM_INSTALL_STATUS_ERR_PCP;
            unsigned int bootslot = 0;
            amInstallPcpaGetInt("bootslot", &bootslot);
            *(unsigned int *)amInstall.m_value = bootslot;
            return AM_INSTALL_STATUS_OK;
        case AM_INSTALL_REQUEST_SET_SBR_BOOTSLOT:
            if (strcmp(keyword, "set_sbr_bootslot") != 0) return AM_INSTALL_STATUS_ERR_PCP;
            return AM_INSTALL_STATUS_OK;
        case AM_INSTALL_REQUEST_QUERY_SEMAPHORE_STATUS:
            if (strcmp(keyword, "query_semaphore_status") != 0) return AM_INSTALL_STATUS_ERR_PCP;
            *(unsigned int *)amInstall.m_value = amInstallResponseCheckQuerySemStatus();
            return AM_INSTALL_STATUS_OK;
        case AM_INSTALL_REQUEST_GET_SEMAPHORE:
            if (strcmp(keyword, "get_semaphore") == 0) {
                unsigned int semid = 0;
                amInstallPcpaGetInt("semid", &semid);
                *(unsigned int *)amInstall.m_value = semid;
                return AM_INSTALL_STATUS_OK;
            }
            return AM_INSTALL_STATUS_ERR_PCP;
        case AM_INSTALL_REQUEST_RELEASE_SEMAPHORE:
            if (strcmp(keyword, "release_semaphore") == 0) return AM_INSTALL_STATUS_OK;
            return AM_INSTALL_STATUS_ERR_PCP;
        case AM_INSTALL_REQUEST_QUERY_SPD:
            if (strcmp(keyword, "query_spd") == 0) {
                amInstallResponseCheckQuerySpd(amInstall.m_value);
                return AM_INSTALL_STATUS_OK;
            }
            return AM_INSTALL_STATUS_ERR_PCP;
        case AM_INSTALL_REQUEST_QUERY_BR:
            if (strcmp(keyword, "query_br") == 0) {
                amInstallResponseCheckQueryBr(amInstall.m_value);
                return AM_INSTALL_STATUS_OK;
            }
            return AM_INSTALL_STATUS_ERR_PCP;
        case AM_INSTALL_REQUEST_QUERY_APPDATA_STATUS:
            if (strcmp(keyword, "query_appdata_status") != 0) return AM_INSTALL_STATUS_ERR_PCP;
            if (amInstallResponseCheckQueryAppdataStatus(amInstall.m_value) != AM_INSTALL_STATUS_OK)
                return AM_INSTALL_STATUS_ERR_PCP;

            if ((*(int *)amInstall.m_value != 4) && (*(int *)amInstall.m_value != 5))
                return AM_INSTALL_STATUS_OK;

            char *sId = pcpaGetCommand(&amInstall.m_pcp, "id");
            if (sId == NULL) return AM_INSTALL_STATUS_ERR_PCP;
            strcpy_s(amInstall.Unk51c, 4, sId);
            return AM_INSTALL_STATUS_OK;
        case AM_INSTALL_REQUEST_15:
        case AM_INSTALL_REQUEST_16:
            return amInstallTaskRequest(keyword);
        case AM_INSTALL_REQUEST_QUERY_VOLUME_NAME:
            if (strcmp(keyword, "query_volume_name") == 0) {
                char *sName = pcpaGetCommand(&amInstall.m_pcp, "name");
                if (sName == NULL) return AM_INSTALL_STATUS_OK;

                size_t hexLen = strlen(sName);
                if (hexLen > 512) hexLen = 512;
                hex_to_bin(sName, amInstall.m_value, hexLen);
                return AM_INSTALL_STATUS_OK;
            }
            return AM_INSTALL_STATUS_ERR_PCP;
        case AM_INSTALL_REQUEST_18:
            return amInstallSuicideSub(keyword);
        default:
            return AM_INSTALL_STATUS_OK;
    }
}

AM_INSTALL_STATUS amInstallOpenBinary(void) {
    e_pcpa_t err = pcpaOpenBinaryClient(&amInstall.m_pcp, amInstall.m_dataPort, 0);
    if (err == e_pcpa_ok) {
        amInstall.m_seq = AM_INSTALL_SEQ_SEND_BINARY;
        return AM_INSTALL_STATUS_OK;
    }
    if (err == e_pcpa_to) {
        amInstall.m_seq = AM_INSTALL_SEQ_BUSY;
        amInstall.m_nextSeq = AM_INSTALL_SEQ_SEND_BINARY;
        return AM_INSTALL_STATUS_OK;
    }

    if (amInstallDebugLevel > 0) amiDebugLog("pcpaOpenBinaryClient() Error!!");
    amInstall.m_seq = AM_INSTALL_SEQ_EXIT;
    return AM_INSTALL_STATUS_ERR_PCP;
}

AM_INSTALL_STATUS amInstallSendBinaryBuffer(AM_INSTALL_BLOCKING blocking) {
    timeout_t timeout = blocking == AM_INSTALL_BLOCK ? TIMEOUT_NONE : 0;
    if (amInstall.m_requestCode != 2) {
        amInstall.m_seq = AM_INSTALL_SEQ_EXIT;
        amInstall.m_nextSeq = AM_INSTALL_SEQ_INVALID;
        return AM_INSTALL_STATUS_ERR_PCP;
    }

    pcpaSetSendBinaryBuffer(&amInstall.m_pcp, amInstall.m_value, amInstall.m_sendBufferLen);
    e_pcpa_t err = pcpaSendBinary(&amInstall.m_pcp, timeout);
    if (err == 0) {
        amInstall.m_seq = AM_INSTALL_SEQ_EXIT;
        return AM_INSTALL_STATUS_OK;
    }
    if (err == 1) {
        amInstall.m_seq = AM_INSTALL_SEQ_BUSY;
        amInstall.m_nextSeq = AM_INSTALL_SEQ_EXIT;
        return AM_INSTALL_STATUS_OK;
    }
    amInstall.m_seq = AM_INSTALL_SEQ_EXIT;
    return AM_INSTALL_STATUS_ERR_PCP;
}
AM_INSTALL_STATUS amInstallExit(void) {
    pcpaCloseBinary(&amInstall.m_pcp);
    pcpaClose(&amInstall.m_pcp);
    amInstall.m_seq = AM_INSTALL_SEQ_NONE;
    amInstall.m_result = AM_INSTALL_STATUS_OK;
    return AM_INSTALL_STATUS_OK;
}

AM_INSTALL_STATUS amInstallOpenBinaryEx(void) {
    e_pcpa_t err = pcpaOpenBinaryClient(&amInstall.m_pcp, amInstall.m_dataPort, TIMEOUT_NONE);
    if (err != e_pcpa_ok) {
        pcpaCloseBinary(&amInstall.m_pcp);
        pcpaClose(&amInstall.m_pcp);
        amInstall.m_seq = AM_INSTALL_SEQ_NONE;
        amInstall.m_result = AM_INSTALL_STATUS_OK;
        amInstall.m_nextSeq = AM_INSTALL_SEQ_INVALID;
        return AM_INSTALL_STATUS_ERR_PCP;
    }
    amInstall.m_seq = AM_INSTALL_SEQ_SEND_BINARY;
    return AM_INSTALL_STATUS_OK;
}

AM_INSTALL_STATUS amInstallSendAndReceiveEx(void) {
    AM_INSTALL_STATUS err;

    amInstall.m_result = AM_INSTALL_STATUS_OK;
    err = amInstallOpenEx();
    if (err == AM_INSTALL_STATUS_OK) {
        do {
            err = amInstallSendAndRecvEx();
            if (err != AM_INSTALL_STATUS_OK) return err;

            err = amInstallResponseCheck();
            if (amInstall.m_result != AM_INSTALL_STATUS_OK) err = amInstall.m_result;

            if (amInstall.m_seq == AM_INSTALL_SEQ_OPEN_BINARY) {
                err = amInstallOpenBinaryEx();
                if (err != AM_INSTALL_STATUS_OK) return err;

                err = amInstallSendBinaryBuffer(AM_INSTALL_BLOCK);
                if (err != AM_INSTALL_STATUS_OK) break;
            }
        } while (amInstall.m_seq != AM_INSTALL_SEQ_EXIT);

        pcpaCloseBinary(&amInstall.m_pcp);
        pcpaClose(&amInstall.m_pcp);
        amInstall.m_seq = AM_INSTALL_SEQ_NONE;
        amInstall.m_result = AM_INSTALL_STATUS_OK;
    }
    return err;
}

AM_INSTALL_STATUS amInstallSendAndRecvEx(void) {
    e_pcpa_t err = pcpaSendRequest(&amInstall.m_pcp, TIMEOUT_NONE);
    if (err != e_pcpa_ok) {
        if (amInstallDebugLevel > 0) amiDebugLog("pcpaSendRequest Error!!");

        pcpaCloseBinary(&amInstall.m_pcp);
        pcpaClose(&amInstall.m_pcp);
        amInstall = amInstall;
        amInstall.m_seq = AM_INSTALL_SEQ_NONE;
        amInstall.m_result = AM_INSTALL_STATUS_OK;
        amInstall.m_nextSeq = AM_INSTALL_SEQ_INVALID;
        return AM_INSTALL_STATUS_ERR_PCP;
    }
    err = pcpaRecvResponse(&amInstall.m_pcp, TIMEOUT_NONE);
    if (err != e_pcpa_ok) {
        pcpaCloseBinary(&amInstall.m_pcp);
        pcpaClose(&amInstall.m_pcp);
        amInstall = amInstall;
        amInstall.m_seq = AM_INSTALL_SEQ_NONE;
        amInstall.m_result = AM_INSTALL_STATUS_OK;
        if (amInstallDebugLevel > 0) amiDebugLog("pcpaRecvResponse Error!!");

        return AM_INSTALL_STATUS_ERR_PCP;
    }
    return AM_INSTALL_STATUS_OK;
}

AM_INSTALL_STATUS amInstallOpenEx(void) {
    e_pcpa_t err =
        pcpaOpenClient(&amInstall.m_pcp, "127.0.0.1", amInstall.m_cmdPort, 60000, TIMEOUT_NONE);
    if (err == e_pcpa_ok) {
        return AM_INSTALL_STATUS_OK;
    }
    if (err == e_pcpa_timeout_open) {
        pcpaClose(&amInstall.m_pcp);
        return amInstallOpenEx();
    }
    if (amInstallDebugLevel > 0) amiDebugLog("pcpaOpenClient() Error(%d)", err);
    pcpaCloseBinary(&amInstall.m_pcp);
    pcpaClose(&amInstall.m_pcp);
    amInstall = amInstall;
    amInstall.m_seq = AM_INSTALL_SEQ_NONE;
    amInstall.m_result = AM_INSTALL_STATUS_OK;
    return AM_INSTALL_STATUS_ERR_NO_SERVER;
}

AM_INSTALL_STATUS amInstallOpen(void) {
    e_pcpa_t err = pcpaOpenClient(&amInstall.m_pcp, "127.0.0.1", amInstall.m_cmdPort, 60000, 0);
    amInstall = amInstall;
    if (err == e_pcpa_ok) {
        amInstall.m_seq = AM_INSTALL_SEQ_SEND_REQUEST;
        amInstall.m_result = AM_INSTALL_STATUS_BUSY;
        return AM_INSTALL_STATUS_OK;
    }
    if (err == e_pcpa_to) {
        amInstall.m_seq = AM_INSTALL_SEQ_BUSY;
        amInstall.m_nextSeq = AM_INSTALL_SEQ_SEND_REQUEST;
        amInstall.m_result = AM_INSTALL_STATUS_BUSY;
        return AM_INSTALL_STATUS_OK;
    }
    amInstall.m_seq = AM_INSTALL_SEQ_EXIT;
    return AM_INSTALL_STATUS_ERR_PCP;
}

AM_INSTALL_STATUS amInstallGetResult(void) {
    if (amInstall.m_init == 0) {
        amInstall.m_result = AM_INSTALL_STATUS_OK;
        return AM_INSTALL_STATUS_ERR_NO_INIT;
    }
    return amInstall.m_result;
}

AM_INSTALL_STATUS amInstallGetBr(AM_INSTALL_BOOT_RECORD *bootRecord, AM_INSTALL_BLOCKING blocking) {
    if (!amInstall.m_init) return AM_INSTALL_STATUS_ERR_NO_INIT;
    if (amInstall.m_seq != AM_INSTALL_SEQ_NONE) return AM_INSTALL_STATUS_BUSY;
    if (bootRecord == NULL) return AM_INSTALL_STATUS_ERR_INVALID_PARAM;

    pcp_send_data_t *send_data = pcpaSetSendPacket(&amInstall.m_pcp, "request", "query_br");
    if (send_data == NULL && amInstallDebugLevel > 0)
        amiDebugLog("Error: pcpaSetSendPacket return NULL");

    amInstall.m_value = bootRecord;
    amInstall.m_requestCode = AM_INSTALL_REQUEST_QUERY_BR;
    if (blocking == AM_INSTALL_BLOCK) return amInstallSendAndReceiveEx();
    return amInstallOpen();
}
