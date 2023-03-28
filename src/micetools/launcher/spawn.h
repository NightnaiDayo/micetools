#include <Windows.h>

typedef struct {
    char* name;
    int mode;
    LPTHREAD_START_ROUTINE dummy;
    LPTHREAD_START_ROUTINE real;
} spawn_t;

#define SPAWN_NONE 0
#define SPAWN_DUMMY 1
#define SPAWN_REAL 2
#define SPAWN_BOTH (SPAWN_DUMMY | SPAWN_REAL)

extern spawn_t mxspawns[3];

DWORD WINAPI MxkThreadProc(LPVOID lpParameter);
DWORD WINAPI MdkThreadProc(LPVOID lpParameter);
DWORD WINAPI MdmThreadProc(LPVOID lpParameter);
DWORD WINAPI MdiThreadProc(LPVOID lpParameter);

void spawn_pcp_processes(void);
