
#include "spawn.h"

#include "../lib/mice/mice.h"

spawn_t mxspawns[3] = {
    // { "keychip", SPAWN_DUMMY, MdkThreadProc, MxkThreadProc },
    // { "master", SPAWN_DUMMY, MdmThreadProc, NULL },
    // { "installer", SPAWN_DUMMY, MdiThreadProc, NULL },
    { "keychip", SPAWN_NONE, MdkThreadProc, MxkThreadProc },
    { "master", SPAWN_NONE, MdmThreadProc, NULL },
    { "installer", SPAWN_NONE, MdiThreadProc, NULL },
};

int mxkMain();
int miceDummyKeychip(unsigned short textPort, unsigned short binaryPort, bool global);
int miceDummyMaster(unsigned short textPort, unsigned short binaryPort, bool global);
int miceDummyInstaller(unsigned short textPort, unsigned short binaryPort, bool global);
DWORD WINAPI MdkThreadProc(LPVOID lpParameter) {
    miceDummyKeychip(40106, 40107, false);
    return 0;
};
DWORD WINAPI MxkThreadProc(LPVOID lpParameter) {
    mxkMain();
    return 0;
};
DWORD WINAPI MdmThreadProc(LPVOID lpParameter) {
    miceDummyMaster(40100, 40101, false);
    return 0;
};
DWORD WINAPI MdiThreadProc(LPVOID lpParameter) {
    miceDummyInstaller(40102, 40103, false);
    return 0;
};

static inline void spawn_one(spawn_t* spawn) {
    if (spawn->mode == SPAWN_NONE) {
        log_warning(plfBoot, "Not spawning %s", spawn->name);
        return;
    }
    if (spawn->mode == SPAWN_BOTH) {
        log_error(plfBoot, "Invalid spawn mode for %s! Only one of md or mx may be selected",
                  spawn->name);
        return;
    }
    if (spawn->mode == SPAWN_REAL) {
        if (spawn->real == NULL) {
            log_error(plfBoot, "mx mode for %s is not supported yet", spawn->name);
            return;
        }
        CreateThread(NULL, 0, spawn->real, NULL, 0, NULL);
    } else if (spawn->mode == SPAWN_DUMMY) {
        if (spawn->dummy == NULL) {
            log_error(plfBoot, "md mode for %s is not supported yet", spawn->name);
            return;
        }
        CreateThread(NULL, 0, spawn->dummy, NULL, 0, NULL);
    } else {
        log_error(plfBoot, "Unknown spawn mode %d for %s", spawn->mode, spawn->name);
        return;
    }
}

void spawn_pcp_processes(void) {
    for (int i = 0; i < _countof(mxspawns); i++) spawn_one(&(mxspawns[i]));
}
