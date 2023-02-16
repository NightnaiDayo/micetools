#include <Windows.h>
#include <process.h>
#include <stdbool.h>

#include "lib/libpcp/libpcp.h"

typedef enum appLauncherUser_ {
    appLauncherUserSystemUser = 0,
    appLauncherUserAppUser,
} appLauncherUser_t;

// TODO: Rename the segaboots
typedef enum appLauncherLaunch_ {
    appLauncherLaunchSegabootR = 1,
    appLauncherLaunchGame,
    appLauncherLaunchSegabootTR,
    appLauncherLaunchGameTest,
    appLauncherLaunchSegabootD,
    appLauncherLaunchSegaboot,
#ifdef MXM_APM2
    appLauncherLaunchAPM2 = 10,
#endif
} appLauncherLaunch_t;

struct appLauncher_;
typedef struct {
    struct appLauncher_* m_appLauncher;
    appLauncherLaunch_t m_mode;
    char m_path[64];
#ifdef MXM_APM2
    // mxmaster seems to be doing m_appLauncher[1] which is weird and so we're not gonna do that
    PROCESS_INFORMATION m_nxAuthProcInfo;
#endif
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

#ifdef MXM_APM2
void nxAuthStart(LPPROCESS_INFORMATION procInfo);
#endif
int appLauncherCreateThread(appLauncherAppInfo_t* appInfo, appLauncher_t* appLauncher);
