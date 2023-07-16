#include "amLog.h"

AM_LIB_C_HEADER(amLog, AM_LOG)

int amLogInit(void) {
    if (amLog.m_Init) return AM_LOG_STATUS_ERR_ALREADY_INIT;

    amLog.m_Option.m_EventSource = "am";
    amLog.m_Option.m_EventType = 1;
    amLog.m_Option.m_EventCategory = 0;
    amLog.m_Option.m_EventId = 0;
    amLog.m_Init = TRUE;
    return AM_LOG_STATUS_OK;
}

int amLogFinalize(void) {
    if (!amLog.m_Init) return AM_LOG_STATUS_ERR_INVALID_PARAM;

    amLog.m_Option.m_EventSource = NULL;
    amLog.m_Option.m_EventType = 0;
    amLog.m_Option.m_EventCategory = 0;
    amLog.m_Option.m_EventId = 0;
    amLog.m_Init = FALSE;
    return AM_LOG_STATUS_OK;
}

int amLogSetEventOption(AM_LOG_OPTION* lpOption) {
    if (!amLog.m_Init) return AM_LOG_STATUS_ERR_INVALID_PARAM;
    if (lpOption == NULL) return AM_LOG_STATUS_ERR_NO_INIT;
    memcpy_s(&amLog.m_Option, sizeof amLog.m_Option, lpOption, sizeof *lpOption);
    return AM_LOG_STATUS_OK;
}

int amLogReportEvent(LPCSTR lpMsg) {
    if (!amLog.m_Init) return AM_LOG_STATUS_ERR_INVALID_PARAM;
    if (lpMsg == NULL) return AM_LOG_STATUS_ERR_NO_INIT;

    HANDLE hEventLog = RegisterEventSourceA(NULL, amLog.m_Option.m_EventSource);
    if (hEventLog == INVALID_HANDLE_VALUE) return AM_LOG_STATUS_ERR_REGISTER;

    WORD wType;
    switch (amLog.m_Option.m_EventType) {
        case 0:
            wType = 0;
            break;
        default:
            wType = 1;
            break;
        case 2:
            wType = 2;
            break;
        case 3:
            wType = 4;
            break;
        case 4:
            wType = 8;
            break;
        case 5:
            wType = 16;
    }
    BOOL suc = ReportEventA(hEventLog, wType, amLog.m_Option.m_EventCategory,
                            amLog.m_Option.m_EventId, NULL, 1, 0, &lpMsg, NULL);
    if (!suc) {
        DeregisterEventSource(hEventLog);
        return AM_LOG_STATUS_ERR_REPORT;
    }
    DeregisterEventSource(hEventLog);
    return AM_LOG_STATUS_OK;
}
