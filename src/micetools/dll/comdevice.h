#include "com.h"
#include "files.h"

typedef struct com_device {
    com_hook_t* com;
    file_hook_t* file;

    BYTE queue[2048];
    DWORD filled;
    HANDLE event;
} com_device_t;
