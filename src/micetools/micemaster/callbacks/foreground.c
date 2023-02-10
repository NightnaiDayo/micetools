#include <io.h>

#include "../mxm.h"

void mxmPcpCurrentFgprocess(pcpa_t* stream, MX_MASTER* mxMaster) {
    char current[8];
    sprintf_s(current, sizeof current, "%d", mxMaster->m_current);
    pcpaSetSendPacket(stream, MXM_FG_CURRENT, current);

    char* command = pcpaGetCommand(stream, MXM_FG_CURRENT);
    if (command == NULL || strcmp(command, "?") != 0) pcpaAddSendPacket(stream, "code", "1");
}

void mxmPcpNextFgprocess(pcpa_t* stream, MX_MASTER* mxMaster) {
    char next[8];
    char workStr[256];

    char* command = pcpaGetCommand(stream, pcpaGetKeyword(stream, 0));
    char* path = pcpaGetCommand(stream, "path");
    char* errormsg = pcpaGetCommand(stream, "errormsg");
    char* size = pcpaGetCommand(stream, "size");

    if (command == NULL) {
        amiDebugLog("Error pcpaGetCommand");
        pcpaSetSendPacket(stream, MXM_FG_NEXT, "?");
        pcpaAddSendPacket(stream, "code", "1");
        return;
    }

    char* endPtr;
    long lCommand = strtol(command, &endPtr, 10);
    if (endPtr == command) {
        sprintf_s(next, sizeof next, "%d", mxMaster->m_next);
        pcpaSetSendPacket(stream, MXM_FG_NEXT, next);

        if (strcmp(command, "?") != 0) {
            pcpaAddSendPacket(stream, "code", "1");
            return;
        }

        if (mxMaster->m_message[0] != '\0')
            pcpaAddSendPacket(stream, "errormsg", mxMaster->m_message);
        if (size == NULL) return;

        unsigned long lSize = strtoul(size, &endPtr, 10);
        if (endPtr == size) {
            pcpaAddSendPacket(stream, "code", "1");
            return;
        }

        if (mxMaster->m_binaryMessageLen > lSize) {
            pcpaAddSendPacket(stream, "code", "5");
            return;
        }
        if (mxMaster->m_binaryMessageLen == 0) return;

        _itoa_s(mxMaster->m_binaryMessageLen, workStr, sizeof workStr, 10);
        pcpaAddSendPacket(stream, "size", workStr);
        _itoa_s(mxMaster->m_dataPort, workStr, sizeof workStr, 10);
        pcpaAddSendPacket(stream, "port", workStr);

        pcpaSetBeforeBinaryModeCallBackFunc(stream, &mxmBeforeBinaryCallback, mxMaster);
        pcpaSetAfterBinaryModeCallBackFunc(stream, &mxmAfterBinaryCallback, mxMaster);
        pcpaSetBinaryMode(stream, binary_mode_send);
        return;
    }

    if (lCommand >= 0 && lCommand <= 5) {
        mxMaster->m_next = lCommand;

        if (lCommand == 2) {
            if (path == NULL)
                mxMaster->m_nextPath[0] = '\0';
            else {
                struct _finddata_t next_file;
                intptr_t hFile = _findfirst(path, &next_file);
                if (hFile == -1) {
                    amiDebugLog("Error : Process does not exist: %s", path);
                    mxMaster->m_next = 0;

                    pcpaSetSendPacket(stream, MXM_FG_NEXT, "?");
                    pcpaAddSendPacket(stream, "code", "1");
                    return;
                }
                sprintf_s(mxMaster->m_nextPath, sizeof mxMaster->m_nextPath, "%s", path);
            }
        }

        sprintf_s(next, sizeof next, "%d", mxMaster->m_next);
        pcpaSetSendPacket(stream, MXM_FG_NEXT, next);

        if (lCommand != 1) return;

        if (errormsg != NULL) {
            if (!mxMaster->m_develop) {
                if (mxMaster->m_message[0] == '\0') {
                    sprintf_s(mxMaster->m_message, sizeof mxMaster->m_message, "%s", errormsg);
                } else {
                    amiDebugLog("Error : There is Already System message");
                    pcpaAddSendPacket(stream, "code", "4");
                }
            } else {
                if (mxMaster->m_message[0] != '\0') {
                    ZeroMemory(mxMaster->m_message, sizeof mxMaster->m_message);
                    // amiDebugReportEvent(&eventSource, "mxmaster : System message was
                    // overwritten");
                }
            }
        }
        if (size == NULL) return;

        if (!mxMaster->m_develop) {
            if (mxMaster->m_binaryMessageLen != 0) {
                amiDebugLog("Error : There is Already System message binnary\n");
                pcpaAddSendPacket(stream, "code", "4");
                return;
            }
        } else if (mxMaster->m_binaryMessageLen != 0) {
            mxMaster->m_binaryMessageLen = 0;
            ZeroMemory(mxMaster->m_binaryMessage, sizeof mxMaster->m_binaryMessage);
            // amiDebugReportEvent(&eventSource, "mxmaster : System message binary was
            // overwritten");
        }

        long recvLength = strtoul(size, &endPtr, 10);
        if (endPtr == size) {
            pcpaAddSendPacket(stream, "code", "1");
            return;
        }

        if (recvLength > sizeof mxMaster->m_binaryMessage) {
            pcpaAddSendPacket(stream, "code", "5");
            return;
        }

        pcpaAddSendPacket(stream, "size", size);
        pcpaAddSendPacket(stream, "port", "40101");
        pcpaSetRecvBinaryBuffer(stream, mxMaster->m_binaryMessage, recvLength);
        mxMaster->m_binaryMessageLen = recvLength;
        pcpaSetBinaryMode(stream, binary_mode_recv);
        return;
    }

    pcpaSetSendPacket(stream, MXM_FG_NEXT, "?");
    pcpaAddSendPacket(stream, "code", "1");
}

void mxmPcpActiveFgprocess(pcpa_t* stream, MX_MASTER* mxMaster) {
    char* command = pcpaGetCommand(stream, MXM_FG_ACTIVE);
    if (command == NULL) {
        amiDebugLog("Error pcpaGetCommand");
        pcpaSetSendPacket(stream, MXM_FG_ACTIVE, "?");
        pcpaAddSendPacket(stream, "code", "1");
        return;
    }

    if (strcmp(command, "1") == 0) {
        if (mxMaster->m_current != 0) {
            amiDebugLog("Error : There is Already Active process");
            pcpaSetSendPacket(stream, MXM_FG_ACTIVE, "1");
            pcpaAddSendPacket(stream, "code", "2");
            return;
        }
        if (mxMaster->m_next != 0) {
            pcpaSetSendPacket(stream, MXM_FG_ACTIVE, "1");
            mxMaster->m_current = mxMaster->m_next;
            mxMaster->m_next = 0;
            mxMaster->m_changeIsSpawn = 1;
            mxMaster->m_processStateChange = true;
            return;
        }

        pcpaSetSendPacket(stream, MXM_FG_ACTIVE, "0");
        pcpaAddSendPacket(stream, "code", "3");
        return;
    } else if (strcmp(command, "0") == 0) {
        pcpaSetSendPacket(stream, MXM_FG_ACTIVE, "0");
        if (mxMaster->m_develop == 0) {
            if (mxMaster->m_next != 0) {
                mxMaster->m_current = mxMaster->m_next;
                mxMaster->m_next = 0;
                mxMaster->m_processStateChange = true;
                return;
            }

            mxMaster->m_fault = 1;
            mxMaster->m_current = 1;
            mxMaster->m_processStateChange = true;
            return;
        }

        mxMaster->m_next = 0;
        mxMaster->m_current = 0;
        mxMaster->m_processStateChange = true;
        return;
    } else {
        pcpaSetSendPacket(stream, MXM_FG_ACTIVE, mxMaster->m_current == 0 ? "0" : "1");
        if (strcmp(command, "?") != 0) pcpaAddSendPacket(stream, "code", "1");
    }
}

void mxmPcpFaultFgprocess(pcpa_t* stream, MX_MASTER* mxMaster) {
    pcpaSetSendPacket(stream, MXM_FG_FAULT, mxMaster->m_fault ? "1" : "0");

    char* command = pcpaGetCommand(stream, MXM_FG_FAULT);
    if (command == NULL) {
        amiDebugLog("Error pcpaGetCommand");
        pcpaSetSendPacket(stream, MXM_FG_FAULT, "?");
        pcpaAddSendPacket(stream, "code", "1");
        return;
    }
    if (strcmp(command, "?") != 0) pcpaAddSendPacket(stream, "code", "1");
}

void mxmPcpGetStartCount(pcpa_t* stream, MX_MASTER* mxMaster) {
    char* command = pcpaGetCommand(stream, MXM_FG_GETCOUNT);
    if (command == NULL) {
        amiDebugLog("Error pcpaGetCommand");
        pcpaSetSendPacket(stream, MXM_FG_GETCOUNT, "?");
        pcpaAddSendPacket(stream, "code", "1");
        return;
    }

    long lWhich = strtol(command, NULL, 10);
    int count = 0;
    bool valid = false;
    switch (lWhich) {
        case 1:
            count = mxMaster->m_countSystemBoot;
            valid = true;
            break;
        case 2:
            count = mxMaster->m_countGameBoot;
            valid = true;
            break;
        case 3:
            count = mxMaster->m_countSystemTest;
            valid = true;
            break;
        case 4:
            count = mxMaster->m_countGameTest;
            valid = true;
            break;
        default:
            valid = false;
            break;
    }

    pcpaSetSendPacket(stream, MXM_FG_GETCOUNT, command);
    char workBuf[256];
    if (valid) {
        sprintf_s(workBuf, sizeof workBuf, "%d", count);
        pcpaAddSendPacket(stream, "count", workBuf);
    } else {
        pcpaAddSendPacket(stream, "code", "1");
    }
}

void mxmPcpSetStartCount(pcpa_t* stream, MX_MASTER* mxMaster) {
    char* command = pcpaGetCommand(stream, MXM_FG_SETCOUNT);
    char* sCount = pcpaGetCommand(stream, "count");
    if (command == NULL) {
        amiDebugLog("Error pcpaGetCommand");
        pcpaSetSendPacket(stream, MXM_FG_SETCOUNT, "?");
        pcpaAddSendPacket(stream, "code", "1");
        return;
    }
    if (sCount == NULL) {
        pcpaSetSendPacket(stream, MXM_FG_SETCOUNT, "?");
        pcpaAddSendPacket(stream, "count", "?");
        pcpaAddSendPacket(stream, "code", "1");
        return;
    }

    long lWhich = strtol(command, NULL, 10);
    long lCount = strtol(sCount, NULL, 10);

    bool valid = false;
    switch (lWhich) {
        case 1:
            mxMaster->m_countSystemBoot = lCount;
            valid = true;
            break;
        case 2:
            mxMaster->m_countGameBoot = lCount;
            valid = true;
            break;
        case 3:
            mxMaster->m_countSystemTest = lCount;
            valid = true;
            break;
        case 4:
            mxMaster->m_countGameTest = lCount;
            valid = true;
            break;
        default:
            valid = false;
            break;
    }

    pcpaSetSendPacket(stream, MXM_FG_SETCOUNT, command);
    if (valid)
        pcpaAddSendPacket(stream, "count", sCount);
    else
        pcpaAddSendPacket(stream, "code", "1");
}
