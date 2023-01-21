#include <process.h>
#include <stdbool.h>

#include "lib/libpcp/libpcp.h"

struct appLauncher_;
typedef struct {
    struct appLauncher_* m_appLauncher;
    unsigned int m_mode;
    char m_path[64];
} appLauncherAppInfo_t;

typedef struct appLauncher_ {
    pcpa_t m_pcp;
    HANDLE m_hThread;
    appLauncherAppInfo_t m_appInfo;
    bool m_createdThread;
    DWORD m_threadAddr;
    wchar_t m_username[64];
    wchar_t m_password[64];
} appLauncher_t;

int appLauncherCreateThread(appLauncherAppInfo_t* appInfo, appLauncher_t* appLauncher);
