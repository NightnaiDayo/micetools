#pragma once

#include "pcpp.h"

typedef enum e_pcpa {
    e_pcpa_ok = 0,
    e_pcpa_to = 1,
    e_pcpa_closed = 2,
    e_pcpa_no_client = 3,
    e_pcpa_not_open = -1,
    e_pcpa_cannot_open = -2,
    e_pcpa_generic = -3,
    e_pcpa_param_invalid = -4,
    e_pcpa_timeout_closed = -5,
    e_pcpa_no_table_space = -6,
    e_pcpa_cb_table_full = -9,
    e_pcpa_stream_unset = -10,
    e_pcpa_cb_table_unset = -11,
    e_pcpa_no_server = -13,
    e_pcpa_timeout_open = -14,
    e_pcpa_inval_addr = -17,
    e_pcpa_already_open = -18,
    e_pcpa_wsa_noinit = -20,
    e_pcpa_unknown = -21,
} e_pcpa_t;

typedef enum {
    pcpa_state_none = 0,
    pcpa_state_wait_prompt = 1,
    pcpa_state_send_request = 2,
    pcpa_state_wait_request = 3,
    pcpa_state_send_response = 4,
    pcpa_state_wait_response = 5,
    pcpa_state_open_binary = 6,
    pcpa_state_send_request_binary = 7,
    pcpa_state_wait_response_binary = 8,
    pcpa_state_wait_binary = 9,

    pcpa_state_send_request_table = 11,
    pcpa_state_12 = 12,
} pcpa_state_t;

e_pcpa_t _pcpaGetErrorFromPcpp(e_pcpp_t err);
#define _errP2A(err) _pcpaGetErrorFromPcpp(err)

struct pcpa;
typedef void(pcpa_callback)(struct pcpa* stream, void* data);

typedef struct pcpa_cb_table {
    char keyword[PCP_KEYWORD_MAX];
    void* data;
    pcpa_callback* callback;
} pcpa_cb_table_t;

typedef struct pcpa {
    pcpp_t pcpp;
    e_pcpa_t err;
    uint state;
    pcpa_cb_table_t* callback_table;
    uint callback_max;
    uint callback_count;
    unsigned char* recv_buffer;
    size_t recv_buffer_len;
    unsigned char* send_buffer;
    size_t send_buffer_len;
    binary_mode_t binary_mode;
    pcpa_callback* binary_mode_before_cb;
    void* binary_mode_before_data;
    pcpa_callback* binary_mode_after_cb;
    void* binary_mode_after_data;
    // char* field_0x254;
    pcp_parse_data_t recv_data;
    pcp_send_data_t send_data;

    // Our additions
    pcpa_callback* before_cb;
} pcpa_t;

void pcpaClose(pcpa_t* stream);
void pcpaCloseBinary(pcpa_t* stream);
char* pcpaGetKeyword(pcpa_t* stream, uint keyword_num);
e_pcpa_t pcpaInitStream(pcpa_t* stream);
e_pcpa_t pcpaOpenClient(pcpa_t* stream, char* ipAddr, ushort port, uint param_4, timeout_t timeout);
e_pcpa_t pcpaOpenServerWithBinary(pcpa_t* stream, int open_mode, ushort port, ushort binary_port, undefined4 param_5);
e_pcpa_t pcpaRecvBinary(pcpa_t* stream, undefined4 something);
e_pcpa_t pcpaSendBinary(pcpa_t* stream, undefined4 something);
e_pcpa_t pcpaServer(pcpa_t* stream, timeout_t timeout_ms);
e_pcpa_t pcpaSetAfterBinaryModeCallBackFunc(pcpa_t* stream, pcpa_callback* callback, void* data);
e_pcpa_t pcpaSetBeforeBinaryModeCallBackFunc(pcpa_t* stream, pcpa_callback* callback, void* data);
e_pcpa_t pcpaSetBinaryMode(pcpa_t* stream, binary_mode_t binary_mode);
e_pcpa_t pcpaSetCallbackFunc(pcpa_t* stream, char* keyword, pcpa_callback* callback, void* data);
e_pcpa_t pcpaSetCallbackFuncBuffer(pcpa_t* stream, pcpa_cb_table_t* callback_table, uint callbacks_max);
e_pcpa_t pcpaSetRecvBinaryBuffer(pcpa_t* stream, unsigned char* recv_buffer, size_t len);
e_pcpa_t pcpaSetSendBinaryBuffer(pcpa_t* stream, const unsigned char* send_buffer, size_t len);
pcp_send_data_t* pcpaSetSendPacket(pcpa_t* stream, char* keyword, char* value);
pcp_send_data_t *pcpaAddSendPacket(pcpa_t *stream, char *keyword, char *value);
char* pcpaGetCommand(pcpa_t* pcpa, char* command);
e_pcpa_t pcpaIsBusy(pcpa_t *pcpa, timeout_t timeout);
e_pcpa_t pcpaSendRequest(pcpa_t *stream, timeout_t timeout);
e_pcpa_t pcpaRecvResponse(pcpa_t *stream, timeout_t timeout);
e_pcpa_t pcpaOpenBinaryClient(pcpa_t *stream, ushort port, timeout_t timeout);

e_pcpa_t pcpaAccessClient(pcpa_t* stream, timeout_t timeout);
