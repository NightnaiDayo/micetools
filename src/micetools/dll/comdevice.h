#pragma once
#include "hooks/com.h"
#include "common.h"
#include "hooks/files.h"

typedef struct com_device {
    com_hook_t* com;
    file_hook_t* file;

    ring_buffer_t in;
    ring_buffer_t out;
    HANDLE event;
    HANDLE thread;
} com_device_t;

typedef DWORD(WINAPI FnComDeviceThread)(com_device_t* com);

short comdev_read(com_device_t* com, unsigned char* buffer, short bytes);
bool comdev_write(com_device_t* com, unsigned char* buffer, short bytes);
short comdev_available(com_device_t* com);

void com_device_thread(com_device_t* com, FnComDeviceThread* thread);
com_device_t* new_com_device(BYTE port);