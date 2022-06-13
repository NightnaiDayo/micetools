#include "pcpt.h"

#define PCPP_CLOSED 0
#define PCPP_OPEN 1

typedef enum e_pcpp {
    e_pcpp_ok = 0,
    e_pcpp_to = 1,
    e_pcpp_closed = 2,
    e_pcpp_no_client = 3,

    e_pcpp_not_open = -1,
    e_pcpp_cannot_open = -2,
    e_pcpp_already_open = -3,
    e_pcpp_param_invalid = -5,
    e_pcpp_timeout_closed = -6,
    e_pcpp_recv_unset = -10,
    e_pcpp_timeout_open = -12,
    e_pcpp_inval_addr = -15,
    e_pcpp_wsa_noinit = -16,
    e_pcpp_unknown = -17,
} e_pcpp_t;

e_pcpp_t _pcppGetErrorFromPcpt(int err);
#define _errT2P(err) _pcppGetErrorFromPcpt(err)

typedef struct pcpp {
    pcpt_t sock;
    pcpt_t data_sock;
    uint field_0xb8;  // Some sort of timeout
    int last_active;
    uint state;
    e_pcpp_t err;
    pcp_parse_data_t* recv_data_buffer;
    char read_bytes_buf[PCP_BUF_MAX];
    uint read_bytes_size;
    uint read_bytes_num;
    pcp_send_data_t* resp_buffer;
    uint resp_buffer_len;
    char send_buf[PCP_SEND_BUF_MAX];
    size_t send_buf_len;
    uint field_0x1e8;
    char* send_binary_buf;
    uint field_0x1f0;
    char* recv_binary_buf;
    size_t field_0x1f8;
    uint field_0x1fc;
    size_t recv_binary_buf_len;
    // All some sort of timeout I think
    uint field_0x204;
    uint field_0x208;
    uint field_0x20c;
    uint field_0x210;
    int field_0x214;
    int open;
} pcpp_t;

pcp_send_data_t* pcppAddSendPacket(pcp_send_data_t* stream, char* key, char* value);
pcp_parse_data_t* pcppChangeRequest(pcpp_t* stream, int* lenout);
bool pcppCheckPrompt(pcpp_t* stream);
e_pcpp_t pcppCheckRecvMsg(char* recv_data, size_t buf_len, int param_3);
uint pcppCheckStr(char* param_1, uint* pcp_len, int* pcp_overflow);
void pcppClose(pcpp_t* stream);
void pcppCloseBinary(pcpp_t* stream);
e_pcpp_t pcppGetBlockingTime(uint param_1, int param_2, pcpp_t* stream, int param_4, uint* blocking_time);
char* pcppGetCommand(pcp_parse_data_t* pcpa, char* command);
char* pcppGetKeyword(pcp_parse_data_t* recvData, uint keyword_num);
SOCKET pcppGetServerSocket(pcpp_t* stream, int which);
e_pcpp_t pcppInitStream(pcpp_t* stream);
e_pcpp_t pcppIsBusy(pcpp_t* stream, timeout_t timeout);
e_pcpp_t pcppOpenBinaryServer(pcpp_t* stream, int open_mode, ushort port);
e_pcpp_t pcppOpenServer(pcpp_t* stream, int open_mode, ushort port, undefined4 param_4);
e_pcpp_t pcppRecvAllMsg(uint param_1, pcpp_t* stream, bool* bReRecv);
e_pcpp_t pcppRecvBinary(pcpp_t* stream, char* recv_buf, size_t buf_len, uint param_4);
uint pcppRecvCheck(char* buf, int* found_at);
e_pcpp_t pcppRecvPrompt(pcpp_t* stream, undefined4 param_2, int param_3);
e_pcpp_t pcppRecvRequest(pcpp_t* stream, pcp_parse_data_t* recv_data, timeout_t timeout);
e_pcpp_t pcppRecvRequestMain(pcpp_t* stream, bool* bReRecv, undefined4 param_3);
e_pcpp_t pcppSendBinary(pcpp_t* stream, char* send_binary_buffer, size_t param_3, uint param_4);
e_pcpp_t pcppSendPrompt(pcpp_t* stream, uint param_2, int param_3);
e_pcpp_t pcppSendRequestMain(pcpp_t* stream, undefined4 param_2, int param_3);
e_pcpp_t pcppSendResponse(pcpp_t* stream, pcp_send_data_t* resp_buffer, size_t buf_len, timeout_t timeout_ms);
e_pcpp_t pcppSendResponseTable(pcpp_t* stream, pcp_send_data_t* data, timeout_t timeout_ms);
pcp_send_data_t* pcppSetSendPacket(pcp_send_data_t* send_data, char* keyword, char* value);
e_pcpp_t pcpp_something(amtime_t* now, timeout_t timeout, pcpp_t* stream, pcpt_t* sock, char* send_buf,
                        size_t* send_len, undefined4 param_7, undefined4 param_8, e_pcpp_t fallback_err);
void pcppResetRead(pcpp_t* param_1);
