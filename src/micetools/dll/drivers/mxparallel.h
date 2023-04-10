#pragma once

#include "../../lib/mxk/mxk.h"
#include "mx.h"

#pragma pack(push, 1)
typedef struct {
    uint32_t crc;
    uint32_t playlimit;
    uint8_t playlimit_sig[128];
    uint32_t nearfull;
    uint8_t nearfull_sig[128];
} billing_t;
typedef struct {
    uint32_t length;
    uint8_t data[0x400 - 4];
} nvram_data_block_t;
#pragma pack(pop)


void micexkTransportSend(unsigned char* send_data, int nbytes);
void micexkSendPacket(unsigned char* send_data);
void micexkTransportRecv(unsigned char* buffer, int nbytes);
void micexkRecvPacket(unsigned char* packet);

static AM_APPBOOT_256 APPBOOT;
static billing_t BILLING;
static uint32_t BILLING_PLAYCOUNT;

// Parallel port emulation
static BYTE parallel_flags;
static BYTE parallel_ctrl;
static BYTE parallel_status;
static BYTE parallel_data;

#define setStatus(mask)          \
    do {                         \
        parallel_status |= mask; \
    } while (0)
#define clearStatus(mask)         \
    do {                          \
        parallel_status &= ~mask; \
    } while (0)
#define getControl(mask) (parallel_ctrl & mask)

#define setError setStatus(0x08)
#define clearError clearStatus(0x08)
#define setSelect setStatus(0x10)
#define clearSelect clearStatus(0x10)
#define setPo setStatus(0x20)
#define clearPo clearStatus(0x20)
#define setAck setStatus(0x40)
#define clearAck clearStatus(0x40)
#define setBusy setStatus(0x80)
#define clearBusy clearStatus(0x80)

#define _Strobe getControl(0x01)
#define _LF getControl(0x02)
#define _Reset getControl(0x04)
#define _SelPrint getControl(0x08)
#define _IsDirWrite getControl(0x20)
