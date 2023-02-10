#pragma once
#include "common.h"
#include "hooks/com.h"
#include "hooks/files.h"

typedef struct com_device {
    com_hook_t* com;
    file_hook_t* file;

    ring_buffer_t in;
    ring_buffer_t out;
    HANDLE event;
    HANDLE thread;
} com_device_t;

typedef struct {
    BYTE frame_length;
    BYTE dst;
    BYTE seq;
    BYTE op;
    BYTE length;
} comio_recv_head_t;

typedef struct {
    BYTE frame_length;
    BYTE src;
    BYTE seq;
    BYTE op;
    BYTE status;
    BYTE length;
} comio_resp_head_t;

#define COMIO_SYNC 0xE0
#define COMIO_MARK 0xD0

#define COMIO_STATUS_OK 0
#define COMIO_STATUS_NG 1

typedef DWORD(WINAPI FnComDeviceThread)(com_device_t* com);

short comdev_read_blocking(com_device_t* com, unsigned char* buffer, short bytes);
short comdev_read(com_device_t* com, unsigned char* buffer, short bytes);
bool comdev_write(com_device_t* com, const unsigned char* buffer, short bytes);
short comdev_available(com_device_t* com);
BYTE comdev_peek(com_device_t* com);

void comio_read(com_device_t* com, BYTE* data, BYTE len);
void comio_write(com_device_t* com, BYTE* data, BYTE len);

unsigned char comio_next_req(com_device_t* com, comio_recv_head_t* head, BYTE* data);
void comio_reply(com_device_t* com, comio_recv_head_t* req, BYTE status, BYTE len, BYTE* data);

void com_device_thread(com_device_t* com, FnComDeviceThread* thread);
com_device_t* new_com_device(BYTE port);