#pragma once

#include <Windows.h>

#include "../_am.h"

AM_LIB_H_HEADER(amLog, AM_LOG)

typedef struct AM_LOG_OPTION {
    LPCSTR m_EventSource;
    DWORD m_EventType;
    WORD m_EventId;
    WORD m_EventCategory;
} AM_LOG_OPTION;

typedef struct AM_LOG {
    BOOL m_Init;
    AM_LOG_OPTION m_Option;
} AM_LOG;

typedef enum {
    AM_LOG_STATUS_OK = 0,
    AM_LOG_STATUS_NG = -1,
    AM_LOG_STATUS_ERR_INVALID_PARAM = -2,
    AM_LOG_STATUS_ERR_NO_INIT = -3,
    AM_LOG_STATUS_ERR_ALREADY_INIT = -4,
    AM_LOG_STATUS_ERR_REGISTER = -5,
    AM_LOG_STATUS_ERR_REPORT = -6,
} AM_LOG_STATUS;

int amLogInit(void);
int amLogFinalize(void);
int amLogSetEventOption(AM_LOG_OPTION* lpOption);
int amLogReportEvent(LPCSTR lpMsg);
