#pragma once
#include "common.h"
#include "hooks/com.h"
#include "hooks/files.h"

#define NUM_COM_PORTS 8

typedef struct com_device com_device_t;
typedef DWORD(WINAPI FnComDeviceThread)(com_device_t* com);

struct com_device {
    com_hook_t* com;
    file_hook_t* file;

    ring_buffer_t in;
    ring_buffer_t out;
    DWORD modemStatus;
    HANDLE dataInReady;
    HANDLE dataOutReady;
    HANDLE thread;
    FnComDeviceThread* thread_worker;
};
com_device_t* com_devices[NUM_COM_PORTS];

#pragma pack(push, 1)
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
#pragma pack(pop)

#define COMIO_SYNC 0xE0
#define COMIO_MARK 0xD0

#define COMIO_STATUS_OK 0
#define COMIO_STATUS_NG 1

short comdev_read_blocking(com_device_t* com, LPVOID buffer, short bytes);
short comdev_read(com_device_t* com, LPVOID buffer, short bytes);
bool comdev_write(com_device_t* com, LPCVOID buffer, short bytes);
short comdev_available(com_device_t* com);
BYTE comdev_peek(com_device_t* com);

void comio_read(com_device_t* com, LPVOID data, BYTE len);
void comio_write(com_device_t* com, LPCVOID data, BYTE len);

unsigned char comio_next_req(com_device_t* com, comio_recv_head_t* head,  LPVOID data);
void comio_reply(com_device_t* com, comio_recv_head_t* req, BYTE status, BYTE len, LPCVOID data);

com_device_t* new_com_device(BYTE port);
BOOL attach_com_device(BYTE port, FnComDeviceThread* thread);
void detach_com_device(BYTE port);
void detach_all_com_devices(void);
void init_com_devices(void);
