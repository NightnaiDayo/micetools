#include "../mxm.h"

void mxmPcpLogAvailable(pcpa_t *stream, MX_MASTER *mxMaster) {
    char *command = pcpaGetCommand(stream, MXM_LOG_AVAILALBE);
    if (command == NULL) {
        amiDebugLog("Error : Command is not set up.");
        pcpaSetSendPacket(stream, MXM_LOG_AVAILALBE, "?");
        return;
    }

    bool metadataChanged = false;
    if (strcmp(command, "?") != 0) {
        if (strcmp(command, "1") == 0) {
            mxMaster->m_loggingAvailable = true;
            mxmEventLogInit();
        } else if (strcmp(command, "0") == 0) {
            mxMaster->m_loggingAvailable = false;
            if (MxmEventLog.m_filenamesSet) {
                MxmEventLog.m_filenamesSet = false;
                MxmEventLog.m_logDriveFound = false;
            }
        } else {
            pcpaSetSendPacket(stream, MXM_LOG_AVAILALBE, "?");
            pcpaAddSendPacket(stream, "code", "1");
            return;
        }

        command = pcpaGetCommand(stream, "backup_count");
        if (command != NULL) {
            mxMaster->m_backupCount = strtol(command, NULL, 10);
            metadataChanged = true;
        }
        command = pcpaGetCommand(stream, "clear_count");
        if (command != NULL) {
            mxMaster->m_clearCount = strtol(command, NULL, 10);
            metadataChanged = true;
        }
        command = pcpaGetCommand(stream, "interval");
        if (command != NULL) {
            mxMaster->m_interval = strtol(command, NULL, 10);
            metadataChanged = true;
        }
        mxMaster->m_backupMetadataChanged = metadataChanged;
    }

    char buffer[256];
    pcpaSetSendPacket(stream, MXM_LOG_AVAILALBE, mxMaster->m_loggingAvailable ? "1" : "0");

    _itoa_s(mxMaster->m_backupCount, buffer, sizeof buffer, 10);
    pcpaAddSendPacket(stream, "backup_count", buffer);
    _itoa_s(mxMaster->m_clearCount, buffer, sizeof buffer, 10);
    pcpaAddSendPacket(stream, "clear_count", buffer);
    _itoa_s(mxMaster->m_interval, buffer, sizeof buffer, 10);
    pcpaAddSendPacket(stream, "interval", buffer);
}

void mxmPcpOutputLog(pcpa_t *stream, MX_MASTER *mxMaster) {
    char logName[12];
    char path[260];
    ZeroMemory(path, sizeof path);
    ZeroMemory(logName, sizeof logName);

    if (stream == NULL || mxMaster == NULL) {
        amiDebugLog("Error : CallbackFuncOutputLog Invalid error");
        return;
    }

    char *command = pcpaGetCommand(stream, pcpaGetKeyword(stream, 0));
    char *sType = pcpaGetCommand(stream, "type");

    if (command == NULL || sType == NULL) {
        amiDebugLog("Error : Command error");
        if (pcpaSetSendPacket(stream, MXM_OUTPUT_LOG, "?") == NULL)
            amiDebugLog("Error : pcpaSetSendPacket return NULL");
        if (pcpaAddSendPacket(stream, "code", "1") == NULL)
            amiDebugLog("Error : pcpaAddSendPacket return NULL");
        return;
    }
    if (strlen(sType) == 0) {
        amiDebugLog("Error : Command length error");

        if (pcpaSetSendPacket(stream, MXM_OUTPUT_LOG, "?") == NULL)
            amiDebugLog("Error : pcpaSetSendPacket return NULL");

        if (pcpaAddSendPacket(stream, "code", "1") == NULL)
            amiDebugLog("Error : pcpaAddSendPacket return NULL");
        return;
    }

    if (strcmp(command, "0") != 0) {
        amiDebugLog("Error : Command type error");
        if (pcpaSetSendPacket(stream, MXM_OUTPUT_LOG, "?") == NULL)
            amiDebugLog("Error : pcpaSetSendPacket return NULL\n");
        if (pcpaAddSendPacket(stream, "code", "1") == NULL)
            amiDebugLog("Error : pcpaAddSendPacket return NULL");
        return;
    }

    if (strcmp(sType, "0") != 0 && strcmp(sType, "1") != 0) {
        amiDebugLog("Error : Type error");
        if (pcpaSetSendPacket(stream, MXM_OUTPUT_LOG, "?") == NULL)
            amiDebugLog("Error : pcpaSetSendPacket return NULL\n");
        if (pcpaAddSendPacket(stream, "code", "1") == NULL)
            amiDebugLog("Error : pcpaAddSendPacket return NULL");
        return;
    }

    char *endPtr;
    int nType = strtoul(sType, &endPtr, 10);

    bool logValid = false;
    if (!mxmEventLogCheckLog(&logValid)) {
        if (pcpaSetSendPacket(stream, MXM_OUTPUT_LOG, "1") == NULL)
            amiDebugLog("Error : pcpaSetSendPacket return NULL");
        return;
    }
    if (!logValid) {
        if (pcpaSetSendPacket(stream, MXM_OUTPUT_LOG, "2") == NULL)
            amiDebugLog("Error : pcpaSetSendPacket return NULL");
        return;
    }

    if (strcmp(sType, "0") == 0) {
        strcpy_s(logName, sizeof logName, "application");
    } else if (strcmp(sType, "1") == 0) {
        strcpy_s(logName, sizeof logName, "system");
    }

    if (!mxmEventLogGetTempPath(logName, path)) {
        if (pcpaSetSendPacket(stream, MXM_OUTPUT_LOG, "1") == NULL)
            amiDebugLog("Error : pcpaSetSendPacket return NULL");
        return;
    }

    if (!mxmEventLogAccessBackup(path, nType)) {
        if (pcpaSetSendPacket(stream, MXM_OUTPUT_LOG, "1") == NULL)
            amiDebugLog("Error : pcpaSetSendPacket return NULL");
        return;
    }

    if (pcpaSetSendPacket(stream, MXM_OUTPUT_LOG, "0") == NULL)
        amiDebugLog("Error : pcpaSetSendPacket return NULL");

    if (pcpaAddSendPacket(stream, "path", path) == NULL)
        amiDebugLog("Error : pcpaAddSendPacket return NULL");
}

void mxmPcpEraseLog(pcpa_t *stream, MX_MASTER *mxMaster) {
    char *command = pcpaGetCommand(stream, MXM_ERASE_LOG);
    if (command == NULL) {
        amiDebugLog("Error : Command error");
        pcpaSetSendPacket(stream, MXM_ERASE_LOG, "1");
        return;
    }

    if (strcmp(command, "0") == 0) {
        if (!mxmEventLogEraseLog()) {
            amiDebugLog("Error : EraseLog error");
            pcpaSetSendPacket(stream, MXM_ERASE_LOG, "1");
        } else {
            pcpaSetSendPacket(stream, MXM_ERASE_LOG, "0");
        }
    } else {
        amiDebugLog("Error : Command error");
        pcpaSetSendPacket(stream, MXM_ERASE_LOG, "1");
    }
}
