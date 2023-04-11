#pragma once

#include "pcp.h"

typedef enum e_pcpt {
    e_pcpt_ok = 0,
    e_pcpt_to = 1,
    e_pcpt_closed = 2,
    e_pcpt_no_client = 3,

    e_pcpt_not_open = -1,
    // TODO: Check if these two are the wrong way round!
    e_pcpt_already_open = -2,
    e_pcpt_already_connected = -3,
    e_pcpt_pointer_unset = -4,

    e_pcpt_cannot_open = -8,
    e_pcpt_nobufs = -9,
    e_pcpt_wsa2_generic = -10,
    e_pcpt_inval_addr = -12,

    e_pcpt_NO_IDEA_WHAT_THIS_IS = -13,

    e_pcpt_recv_unset = -15,
    e_pcpt_wsa_noinit = -16,
    e_pcpt_unknown = -17,
} e_pcpt_t;

typedef enum {
    PCPT_CONFIG_0 = 0,
    PCPT_CONFIG_1 = 1,
    PCPT_SO_LINGER = 2,
    PCPT_TCP_NODELAY = 3,
} pcpt_config_t;

e_pcpt_t _pcptGetErrorFromWin(int err);
#define _errW2T(err) _pcptGetErrorFromWin(err)

typedef struct pcpt {
    SOCKET server_sock;
    SOCKET client_sock;
    int client_open;
    int open;
    e_pcpt_t err;
    timeout_t field_0x14;
    timeout_t config_0;
    uint config_1;
    uint so_linger;
    uint tcp_nodelay;
    WSAEVENT client_event;
    WSAEVENT server_event;
    const unsigned char* send_buf;
    size_t* send_buf_count;
    unsigned char* recv_buf;
    size_t* recv_buf_count;
    uint field_0x40;
    SOCKADDR_IN client_addr;
    int field_0x54;
    int field_0x58;
} pcpt_t;

// Confirmed with nrs.pdb
void pcptClose(pcpt_t* sock);
void pcptCloseDataSock(pcpt_t* sock);
int pcptGetConfig(pcpt_t* sock, pcpt_config_t config);
e_pcpt_t pcptInitStream(pcpt_t* sock);
e_pcpt_t pcptIsBusy(pcpt_t* sock, timeout_t timeout_ms);
e_pcpt_t pcptOpenClient(pcpt_t* stream, char* ipAddr, ushort port, timeout_t timeout);
e_pcpt_t pcptRecv(pcpt_t* sock, unsigned char* recv_buf, size_t* recv_buf_len,
                  timeout_t timeout_ms);
e_pcpt_t pcptSend(pcpt_t* sock, const unsigned char* send_buf, size_t* send_len, uint param_4,
                  timeout_t timeout_ms);
void pcptSetConfig(pcpt_t* sock, pcpt_config_t config, int value);

// Unconfirmed
e_pcpt_t pcptAcceptServer(pcpt_t* stream, timeout_t timeout_ms);
e_pcpt_t pcptCheckConnectAble(pcpt_t* sock, timeout_t timeout_ms);
e_pcpt_t pcptCheckEvent(HANDLE event, timeout_t timeout_ms, SOCKET socket, uint event_mask);
e_pcpt_t pcptOpenDataSockServer(pcpt_t* sock, timeout_t timeout_ms);
e_pcpt_t pcptSendAllMsg(pcpt_t* sock, timeout_t timeout_ms);
e_pcpt_t pcptOpenServer(pcpt_t* sock, int open_mode, ushort port);
