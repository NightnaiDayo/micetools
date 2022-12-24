#include "pcpp.h"

int FUN_00459d00(amtime_t* t1, amtime_t* t2) {
    if (t1 == NULL || t2 == NULL) return -1;
    return ((t2->seconds - t1->seconds) * 1000000 - t1->microseconds) + t2->microseconds;  // Swapped
}

// TODO: Invert these conditions to be clearer
int pcpp_isValidChar(char chr) {
    if (!isalnum(chr) && chr != '.' && chr != '_' && chr != '-' && chr != ':' && chr != '@' && chr != '%' &&
        chr != '/' && chr != '\\' && chr != '#' && chr != '{' && chr != '}') {
        return 0;
    }
    return 1;
}

e_pcpp_t _pcppGetErrorFromPcpt(int err) {
    switch (err) {
        case e_pcpt_ok:
            return e_pcpp_ok;
        case e_pcpt_to:
            return e_pcpp_to;
        case e_pcpt_closed:
            return e_pcpp_closed;
        case e_pcpt_no_client:
            return e_pcpp_no_client;
        case e_pcpt_wsa_noinit:
            return e_pcpp_wsa_noinit;
        case e_pcpt_recv_unset:
            return e_pcpp_recv_unset;
        case e_pcpt_NO_IDEA_WHAT_THIS_IS:
            return (e_pcpp_t)-7;
        case e_pcpt_inval_addr:
            return e_pcpp_inval_addr;
        case -11:
        case e_pcpt_wsa2_generic:
        case e_pcpt_nobufs:
        case e_pcpt_cannot_open:
            return e_pcpp_cannot_open;
        case -7:
        case e_pcpt_pointer_unset:
        case e_pcpt_already_connected:
            return e_pcpp_param_invalid;
        case -6:
        case e_pcpt_not_open:
            return e_pcpp_not_open;
        case -5:
        case e_pcpt_already_open:
            return e_pcpp_already_open;
        default:
            return e_pcpp_unknown;
    }
}

bool pcppCheckPrompt(pcpp_t* stream) {
    if (stream == NULL) {
        PCP_LOG("error PCPP stream isn\'t set\n");
        return false;
    }

    uint max = stream->read_bytes_num;
    bool found = false;

    for (uint cursor = 0; cursor < max; cursor++) {
        unsigned char* pos = stream->read_bytes_buf + cursor;
        if (stream->read_bytes_buf[cursor] == '>') {
            if (cursor < stream->read_bytes_num) {
                do {
                    *pos = pos[1];
                    pos++;
                } while (pos + (-204 - (int)stream) < (unsigned char*)stream->read_bytes_num);
            }

            stream->read_bytes_num--;
            stream->read_bytes_size++;
            found = true;
        }
    }

    return found;
}

e_pcpp_t pcppCheckRecvMsg(unsigned char* recv_data, size_t buf_len, int param_3) {
    bool bVar1 = true;
    int local_8 = 0;

    if (recv_data == NULL) {
        PCP_LOG("pointer error\n");
        return e_pcpp_param_invalid;
    }

    while (1) {
        local_8 = buf_len - local_8;
        uint recv_err = pcppRecvCheck(recv_data, &local_8);

        if (recv_err == 1) return e_pcpp_param_invalid;
        if (recv_err == 8) {
            if (!bVar1) return e_pcpp_param_invalid;
            if (param_3 != 0) return e_pcpp_param_invalid;
        } else {
            if (recv_err == 7 && !bVar1) return e_pcpp_param_invalid;
        }

        recv_data += local_8;
        local_8++;
        bVar1 = false;

        if (recv_err == 0 || recv_err == 6 || recv_err == 7 || recv_err == 8) {
            return e_pcpp_ok;
        }
    }
}

uint pcppCheckStr(unsigned char* recv_data, uint* found_at, int* find_failed) {
    bool end = false;
    bool bVar2 = false;
    uint idx = 0;

    if (recv_data == NULL || find_failed == NULL || found_at == NULL) {
        PCP_LOG("pointer error\n");
        return 0;
    }

    *find_failed = 0;
    if (*found_at == 0) return 0;

    do {
        char chr = recv_data[idx];
        if (PCP_WHITESPACE(chr)) {
            if (bVar2) end = true;
        } else {
            if (!pcpp_isValidChar(chr)) {
                if (recv_data[idx] != PCP_CHAR_UNKNOWN) {
                    *found_at = idx + 1;
                    switch (recv_data[idx]) {
                        case PCP_CHAR_SEP:
                            return 5;
                        case PCP_CHAR_CR:
                            if (recv_data[idx + 1] != '\n') return 3;
                            *found_at = idx + 2;
                            return 4;
                        case PCP_CHAR_LF:
                            return 2;
                        case PCP_CHAR_EQU:
                            return 1;
                        default:
                            return 0;
                    }
                }
                if (bVar2 || end) {
                    *found_at = idx + 1;
                    return 0;
                }

                *find_failed = 1;
            } else {
                if (end || *find_failed != 0) {
                    *found_at = idx + 1;
                    return 0;
                }
                bVar2 = true;
            }
        }

        idx++;
        if (*found_at <= idx) return 0;
    } while (true);
}

void pcppClose(pcpp_t* stream) {
    if (stream == NULL) {
        PCP_LOG("error PCPP stream isn\'t set\n");
        return;
    }

    pcptClose(&stream->sock);
    pcptClose(&stream->data_sock);

    stream->field_0xb8 = 0;
    stream->last_active = 0;
    stream->state = 0;
    stream->err = e_pcpp_unknown;
    stream->recv_data_buffer = NULL;
    memset(stream->read_bytes_buf, 0, sizeof stream->read_bytes_buf);
    stream->read_bytes_size = sizeof stream->read_bytes_buf;
    stream->read_bytes_num = 0;
    stream->resp_buffer = NULL;
    stream->resp_buffer_len = 0;
    ZERO(stream->send_buf);
    stream->send_buf_len = PCP_SEND_BUF_MAX;
    stream->field_0x1e8 = 0;
    stream->send_binary_buf = NULL;
    stream->field_0x1f0 = 0;
    stream->recv_binary_buf = NULL;
    stream->field_0x1f8 = 0;
    stream->recv_binary_buf_len = 0;
    stream->field_0x1fc = 0;
    stream->open = 0;
    stream->field_0x214 = 0;
}

void pcppCloseBinary(pcpp_t* stream) {
    if (stream == NULL) {
        PCP_LOG("error PCPP stream isn\'t set\n");
        return;
    }

    pcptCloseDataSock(&stream->data_sock);
    stream->read_bytes_size = sizeof stream->read_bytes_buf;
    stream->read_bytes_num = 0;
    memset(stream->read_bytes_buf, 0, sizeof stream->read_bytes_buf);
    stream->resp_buffer = NULL;
    stream->resp_buffer_len = 0;

    amtime_t time;
    amiTimerGet(&time);
    stream->last_active = _amTimeMs(time);
    stream->state = 0;
}

e_pcpp_t pcppGetBlockingTime(uint param_1, timeout_t timeout, pcpp_t* stream, int param_4, uint* blocking_time) {
    byte bVar1;
    uint uVar2;
    uint uVar3;
    amtime_t local_8;

    if (stream == NULL || blocking_time == NULL) {
        PCP_LOG("pointer error\n");
        return 0;
    }

    amiTimerGet(&local_8);
    uVar2 = (local_8.seconds * 1000 - stream->last_active) + local_8.microseconds / 1000;  // swapped
    if (param_1 <= uVar2) {
        *blocking_time = 0;
        return 1;
    }
    if ((timeout == TIMEOUT_NONE) || (param_1 - uVar2 < (uint)timeout)) {
        bVar1 = 1;
        uVar2 = param_1 - uVar2;
    } else {
        bVar1 = 0;
        uVar2 = timeout;
    }
    uVar3 = (local_8.seconds * 1000 - param_4) + local_8.microseconds / 1000;  // Swapped
    if (timeout != TIMEOUT_NONE) {
        if ((uint)timeout <= uVar3) {
            *blocking_time = 0;
            return 0;
        }
        uVar3 = timeout - uVar3;
        if (uVar3 < uVar2) {
            *blocking_time = uVar3;
            return 0;
        }
    }
    *blocking_time = uVar2;
    return (uint)bVar1;
}

char* pcppGetCommand(pcp_parse_data_t* recv_data, char* command) {
    if (recv_data == NULL || command == NULL) {
        PCP_LOG("error Recv Data isn\'t set\n");
        return NULL;
    }

    for (uint cmd = 0; cmd < recv_data->cmd_count; cmd++) {
        if (!strcmp(recv_data->strings + recv_data->keywords[cmd], command)) {
            if (recv_data->values[cmd] == 0) return NULL;
            return recv_data->strings + recv_data->values[cmd];
        }
    }
    return NULL;
}

char* pcppGetKeyword(pcp_parse_data_t* recvData, uint keywordNum) {
    if (recvData == NULL) {
        PCP_LOG("error Recv Data isn\'t set\n");
        return NULL;
    }

    if ((keywordNum >= PCP_CMDS_MAX) && (keywordNum > recvData->cmd_count)) {
        PCP_LOG("error keywordNum over\n");
        return NULL;
    }
    return recvData->strings + recvData->keywords[keywordNum];
}

SOCKET pcppGetServerSocket(pcpp_t* stream, int which) {
    if (stream == NULL) {
        PCP_LOG("error PCPP stream isn\'t set\n");
        return SOCKET_INVAL;
    }

    switch (which) {
        case 0:
            return (stream->sock).server_sock;
        case 1:
            return (stream->data_sock).server_sock;
        default:
            return SOCKET_INVAL;
    }
}

e_pcpp_t pcppInitStream(pcpp_t* stream) {
    if (stream == NULL) {
        PCP_LOG("error don\'t set stream\n");
        return e_pcpp_param_invalid;
    }

    e_pcpt_t err = pcptInitStream(&stream->sock);
    if (err != e_pcpt_ok) return _errT2P(err);

    err = pcptInitStream(&stream->data_sock);
    if (err != e_pcpt_ok) return _errT2P(err);

    pcptSetConfig(&stream->data_sock, PCPT_TCP_NODELAY, 1);

    stream->field_0xb8 = 0;
    stream->last_active = 0;
    stream->state = 0;
    stream->err = e_pcpp_unknown;
    stream->recv_data_buffer = NULL;
    ZERO(stream->read_bytes_buf);
    // memset(stream->read_bytes_buf, 0, sizeof stream->read_bytes_buf);
    stream->read_bytes_size = sizeof stream->read_bytes_buf;
    stream->read_bytes_num = 0;
    stream->resp_buffer = NULL;
    stream->resp_buffer_len = 0;
    ZERO(stream->send_buf);
    stream->field_0x1e8 = 0;
    stream->send_binary_buf = NULL;
    stream->field_0x1f0 = 0;
    stream->recv_binary_buf = NULL;
    stream->field_0x1f8 = 0;
    stream->recv_binary_buf_len = 0;
    stream->field_0x1fc = 0;
    stream->open = 0;
    stream->field_0x214 = 0;
    stream->field_0x20c = 10000;
    stream->field_0x208 = 10000;
    stream->send_buf_len = PCP_SEND_BUF_MAX;
    stream->field_0x204 = 1000;
    stream->field_0x210 = 60000;
    return e_pcpp_ok;
}

e_pcpp_t pcppOpenBinaryServer(pcpp_t* stream, int open_mode, ushort port) {
    if (stream == NULL) {
        PCP_LOG("error PCPP stream isn\'t set\n");
        return e_pcpp_param_invalid;
    }

    amtime_t time;
    amiTimerGet(&time);
    stream->last_active = _amTimeMs(time);
    stream->state = 0;
    if (open_mode != 0 && open_mode != 1) {
        PCP_LOG("error Open Mode isn\'t set\n");
        stream->state = 0;
        return stream->err = e_pcpp_param_invalid;
    }

    e_pcpp_t err = _errT2P(pcptOpenServer(&stream->data_sock, open_mode, port));
    stream->err = err;
    if (err != e_pcpp_ok) {
        PCP_LOG("error pcptOpenBinaryServer\n");
        return err;
    }

    amiTimerGet(&time);
    stream->last_active = _amTimeMs(time);
    return e_pcpp_ok;
}

e_pcpp_t pcppOpenServer(pcpp_t* stream, int open_mode, u_short port, undefined4 param_4) {
    if (stream == NULL) {
        PCP_LOG("error PCPP stream isn\'t set\n");
        return e_pcpp_param_invalid;
    }
    stream->field_0xb8 = param_4;
    if (open_mode != 0 && open_mode != 1) {
        PCP_LOG("error Open Mode isn\'t set\n");

        stream->err = e_pcpp_param_invalid;
        stream->state = 0;
        return e_pcpp_param_invalid;
    }

    e_pcpp_t err = _errT2P(pcptOpenServer(&stream->sock, open_mode, port));
    stream->err = err;
    if (err != e_pcpp_ok) {
        PCP_LOG("error pcppOpenServer error = %d\n", err);
        return err;
    }
    stream->open = 1;

    amtime_t time;
    amiTimerGet(&time);
    stream->last_active = _amTimeMs(time);
    return e_pcpp_ok;
}

pcp_send_data_t* pcppSetSendPacket(pcp_send_data_t* send_data, char* keyword, char* value) {
    if (send_data == NULL || keyword == NULL) {
        PCP_LOG("error Send Data isn\'t set\n");
        return NULL;
    }

    memset(send_data->data, 0, sizeof *send_data->data);
    send_data->length = 0;
    size_t kwLen = strlen(keyword);

    if (value == NULL) {
        if (kwLen + 2 < PCP_BUF_MAX) {
            sprintf_s((char*)send_data->data, PCP_BUF_MAX, "%s\r\n", keyword);
            send_data->length = kwLen & 0xff;
            return send_data;
        }
    } else {
        size_t valLen = strlen(value);
        if (kwLen + 3 + valLen < PCP_BUF_MAX) {
            sprintf_s((char*)send_data->data, PCP_BUF_MAX, "%s=%s\r\n", keyword, value);
            send_data->length = kwLen + 1 + valLen;
            return send_data;
        }
    }
    return NULL;
}

uint pcppRecvCheck(unsigned char* buf, int* found_at) {
    uint uVar1;
    uint uVar3;
    int find_failed;
    uint found_at_;
    int local_4;

    uVar3 = 0;
    find_failed = 0;
    if (buf == NULL || found_at == NULL) {
        PCP_LOG("pointer error\n");
        return 1;
    }

    local_4 = *found_at;
    *found_at = 0;
    for (int uVar2 = 0; uVar2 < 2; uVar2++) {
        found_at_ = local_4 - uVar3;
        uVar1 = pcppCheckStr(&buf[uVar3], &found_at_, &find_failed);
        *found_at = *found_at + found_at_;

        switch (uVar1) {
            case 0:
                return 1;
            case 1:
                if (uVar2) return 1;
                uVar3 = found_at_;
                if (find_failed) return 1;
                break;
            case 2:
            case 3:
            case 4:
                if (uVar2) return find_failed ? 7 : 0;
                return find_failed ? 8 : 6;
            case 5:
                if (uVar2) return find_failed ? 4 : 2;
                return find_failed ? 5 : 3;
        }
    }
    return 1;
}

e_pcpp_t pcppSendResponseTable(pcpp_t* stream, pcp_send_data_t* data, timeout_t timeout_ms) {
    if (stream == NULL) {
        PCP_LOG("error PCPP stream isn\'t set\n");
        return e_pcpp_param_invalid;
    }
    if (data == NULL) {
        PCP_LOG("error Response buffer isn\'t set\n");
        return e_pcpp_param_invalid;
    }

    return pcppSendResponse(stream, data, data->length + 2, timeout_ms);
}

e_pcpp_t pcppSendResponse(pcpp_t* stream, pcp_send_data_t* resp_buffer, size_t buf_len, timeout_t timeout_ms)

{
    e_pcpp_t err;
    amtime_t time;

    if (stream == NULL) {
        PCP_LOG("error PCPP stream isn\'t set\n");
        return e_pcpp_param_invalid;
    }
    if (resp_buffer == NULL) {
        PCP_LOG("error Response buffer isn\'t set\n");
        return e_pcpp_param_invalid;
    }

    if (buf_len > PCP_BUF_MAX - 1) {
        return stream->err = e_pcpp_param_invalid;
    }

    amiTimerGet(&time);
    stream->err = err = pcppCheckRecvMsg(resp_buffer->data, buf_len, 0);

    if (err != e_pcpp_ok) {
        stream->state = 0;
        pcppResetRead(stream);
        PCP_LOG("error Send message format error\n");
        return stream->err;
    }

    stream->resp_buffer = resp_buffer;
    stream->resp_buffer_len = buf_len;
    stream->state = 6;
    err = pcpp_something(&time, timeout_ms, stream, &stream->sock, resp_buffer->data, &stream->resp_buffer_len,
                         stream->field_0x204, stream->field_0x210, -6);
    stream->err = err;
    if (err != e_pcpp_to) {
        stream->state = 0;
        if (err != e_pcpp_ok) {
            pcptCloseDataSock(&stream->sock);
            pcppResetRead(stream);
        }
    }
    amiTimerGet(&time);
    stream->last_active = _amTimeMs(time);
    return stream->err;
}

void pcppResetRead(pcpp_t* stream) {
    stream->read_bytes_size = PCP_BUF_MAX;
    stream->read_bytes_num = 0;
    ZERO_BUF(stream->read_bytes_buf);
    stream->resp_buffer = NULL;
    stream->resp_buffer_len = 0;
}

e_pcpp_t pcppRecvRequest(pcpp_t* stream, pcp_parse_data_t* recv_data, timeout_t timeout) {
    e_pcpp_t err;
    uint timeout_ms = timeout;
    undefined4 uVar1;
    amtime_t now;
    amtime_t start;
    amtime_t now2;

    if (stream == NULL) {
        PCP_LOG("error PCPP stream isn\'t set\n");
        return e_pcpp_param_invalid;
    }
    if (recv_data == NULL) {
        PCP_LOG("error Request buffer isn\'t set\n");
        return stream->err = e_pcpp_param_invalid;
    }

    if ((stream->sock).open == PCPT_LISTENING) {
        uVar1 = stream->field_0x210;
    } else {
        uVar1 = stream->field_0xb8;
    }
    stream->recv_data_buffer = recv_data;
    amiTimerGet(&start);
    stream->state = 2;
    err = pcppSendPrompt(stream, uVar1, timeout_ms);
    stream->err = err;
    if (err != e_pcpp_to) {
        stream->state = 0;
    }
    if (err == e_pcpp_ok) {
        stream->open = 0;
        amiTimerGet(&now);
        stream->last_active = _amTimeMs(now);
        stream->state = 5;
        if (timeout_ms != TIMEOUT_NONE) {
            uint elapsed = _amTimeDelta(now, start);
            if (elapsed < timeout_ms) {
                timeout_ms = timeout_ms - elapsed;
            } else {
                timeout_ms = 0;
            }
        }

        bool bReRecv;
        err = pcppRecvRequestMain(stream, &bReRecv, timeout_ms);
        stream->err = err;
        if (err == e_pcpp_to) {
            if (bReRecv) {
                stream->state = 14;
                return e_pcpp_to;
            }
        } else {
            stream->state = 0;
            if (err == e_pcpp_ok) {
                amiTimerGet(&now2);
                stream->last_active = _amTimeMs(now2);
            }
        }
        err = stream->err;
    }
    return err;
}

e_pcpp_t pcppRecvRequestMain(pcpp_t* stream, bool* bReRecv, undefined4 param_3) {
    unsigned char* recv_data;

    if (stream == NULL) {
        PCP_LOG("error PCPP stream isn\'t set\n");
        return e_pcpp_param_invalid;
    }

    stream->field_0x214 = 0;

    e_pcpp_t err = pcppRecvAllMsg(param_3, stream, bReRecv);
    if (err != e_pcpp_ok) return err;

    recv_data = stream->read_bytes_buf;
    if (pcppCheckRecvMsg(recv_data, stream->read_bytes_num, 1) != e_pcpp_ok) {
        pcppResetRead(stream);
        return -9;
    }

    uint local_4 = 0;
    pcppChangeRequest(stream, &local_4);

    if (local_4 < stream->read_bytes_num) memcpy(recv_data + local_4, recv_data, stream->read_bytes_num);

    stream->read_bytes_num -= local_4;
    stream->read_bytes_size += local_4;
    stream->read_bytes_buf[stream->read_bytes_num] = '\0';
    return e_pcpp_ok;
}

e_pcpp_t pcppSendPrompt(pcpp_t* stream, uint param_2, timeout_t timeout_ms) {
    amtime_t now;

    bool has_timeout = false;
    if (stream == NULL) {
        PCP_LOG("error PCPP stream isn\'t set\n");
        return e_pcpp_param_invalid;
    }

    amiTimerGet(&now);
    uint since_open = _amTimeMs(now) - stream->last_active;
    if (since_open > param_2) {
        timeout_ms = 0;
    } else {
        if ((timeout_ms != -1) && ((uint)timeout_ms <= param_2 - since_open)) goto LAB_00456133;
        timeout_ms = param_2 - since_open;
    }
    has_timeout = true;
LAB_00456133:
    size_t* send_len = &stream->send_buf_len;
    stream->send_buf[0] = '>';
    *send_len = 1;
    if ((stream->sock).open == PCPT_LISTENING) stream->open = 1;

    e_pcpp_t err = _errT2P(pcptSend(&stream->sock, stream->send_buf, send_len, stream->field_0x204, timeout_ms));

    if (has_timeout && err == e_pcpp_to) {
        err = stream->open ? -12 : -6;
        pcptCloseDataSock(&stream->sock);
        pcppResetRead(stream);
        if (stream->open == 0) PCP_LOG("Error : Time out error\n");
    }
    amiTimerGet(&now);
    stream->last_active = _amTimeMs(now);
    return err;
}

e_pcpp_t pcppRecvAllMsg(uint param_1, pcpp_t* stream, bool* bReRecv) {
    uint* recv_buf_len;
    int iVar1;
    e_pcpt_t err;
    e_pcpp_t eVar2;
    undefined4 uVar3;
    uint local_14;
    amtime_t local_10;
    amtime_t local_8;

    local_14 = param_1;
    if (stream == NULL) {
        PCP_LOG("error PCPP stream isn\'t set\n");
        return e_pcpp_param_invalid;
    }

    if (bReRecv == NULL) {
        PCP_LOG("error bReRecv isn\'t set\n");
        return e_pcpp_param_invalid;
    }
    *bReRecv = false;
    amiTimerGet(&local_10);
    if (stream->read_bytes_num != 0) {
        for (size_t i = 0; i < stream->read_bytes_num; i++) {
            if (stream->read_bytes_buf[i] == '\r' || stream->read_bytes_buf[i] == '\n') {
                stream->field_0x214 = 0;
                return e_pcpp_ok;
            }
        }
    }
    uVar3 = stream->field_0x214 == 0 ? stream->field_0xb8 : stream->field_0x204;

    iVar1 = pcppGetBlockingTime(uVar3, param_1, stream, _amTimeMs(local_10), &local_14);
    recv_buf_len = &stream->read_bytes_size;
    *recv_buf_len = PCP_BUF_MAX - stream->read_bytes_num;
    err = pcptRecv(&stream->sock, (stream->read_bytes_buf + stream->read_bytes_num), recv_buf_len, local_14);
    eVar2 = _errT2P(err);
    if (eVar2 == e_pcpp_to) {
        if (iVar1 == 0) {
            return e_pcpp_to;
        }
        eVar2 = e_pcpp_timeout_closed - (uint)(stream->field_0x214 != 0);
        pcptCloseDataSock(&stream->sock);
        *recv_buf_len = PCP_BUF_MAX;
        stream->read_bytes_num = 0;
        ZERO_BUF(stream->read_bytes_buf);
        stream->resp_buffer = NULL;
        stream->resp_buffer_len = 0;
    }
    if (eVar2 != e_pcpp_ok) {
        return eVar2;
    }
    stream->read_bytes_num = stream->read_bytes_num + *recv_buf_len;
    *recv_buf_len = PCP_BUF_MAX - stream->read_bytes_num;
    amiTimerGet(&local_8);
    stream->last_active = _amTimeMs(local_8);
    stream->field_0x214 = 1;
    *bReRecv = true;

    if (stream->read_bytes_num != 0) {
        for (size_t i = 0; i < stream->read_bytes_num; i++) {
            if (stream->read_bytes_buf[i] == '\r' || stream->read_bytes_buf[i] == '\n') {
                *bReRecv = false;
                stream->field_0x214 = 0;
                return e_pcpp_ok;
            }
        }
    }
    return e_pcpp_to;
}

pcp_parse_data_t* pcppChangeRequest(pcpp_t* stream, uint* lenout) {
    byte bVar5 = 0;
    byte bVar6 = 0;

    if (stream == NULL || lenout == NULL) {
        PCP_LOG("pointer error\n");
        return NULL;
    }

    if (stream->recv_data_buffer == NULL) {
        PCP_LOG("don\'t set recvData buffer\n");
        return NULL;
    }

    if (stream->read_bytes_num > PCP_BUF_MAX) {
        PCP_LOG("Buffer size error\n");
        return NULL;
    }

    ZERO_BUF(stream->recv_data_buffer->strings);
    ZERO_BUF(stream->recv_data_buffer->keywords);
    ZERO_BUF(stream->recv_data_buffer->values);

    stream->recv_data_buffer->keywords[0] = 0;
    stream->recv_data_buffer->cmd_count = 1;

    if (stream->read_bytes_num == 0) {
        *lenout = 1;
        return stream->recv_data_buffer;
    }

    bool comment = false;

    size_t i;
    for (i = 0; i < stream->read_bytes_num; i++) {
        char chr = stream->read_bytes_buf[i];

        if (chr == PCP_CHAR_CR || chr == PCP_CHAR_LF) {
            stream->recv_data_buffer->strings[++bVar6] = PCP_CHAR_EOF;
            if (i + 1 < stream->read_bytes_num && stream->read_bytes_buf[i] == PCP_CHAR_CR &&
                stream->read_bytes_buf[i + 1] == PCP_CHAR_LF)
                i++;

            *lenout = i + 1;
            return stream->recv_data_buffer;
        }

        if (comment) {
            if (chr == PCP_CHAR_HASH) comment = false;
            continue;
        }

        switch (chr) {
            case PCP_CHAR_UNKNOWN:
                stream->recv_data_buffer->strings[bVar6++] = PCP_CHAR_UNKNOWN;
                break;
            case PCP_CHAR_SEP:
                if (stream->recv_data_buffer->cmd_count > PCP_CMDS_MAX) {
                    PCP_LOG("Buffer size error\n");
                    return NULL;
                }
                stream->recv_data_buffer->strings[++bVar6] = PCP_CHAR_EOF;
                stream->recv_data_buffer->keywords[stream->recv_data_buffer->cmd_count] = bVar6++;
                stream->recv_data_buffer->cmd_count++;
                bVar5 = bVar6;
                break;
            case PCP_CHAR_EQU:
                if (stream->recv_data_buffer->cmd_count > PCP_CMDS_MAX) {
                    PCP_LOG("Buffer size error\n");
                    return NULL;
                }
                stream->recv_data_buffer->strings[++bVar6] = PCP_CHAR_EOF;
                stream->recv_data_buffer->keywords[stream->recv_data_buffer->cmd_count + PCP_CMDS_MAX - 1] = bVar6++;
                bVar5 = bVar6;
                break;
            case PCP_CHAR_HASH:
                comment = true;
                break;
            default:
                bVar6 = bVar5;
                if (pcpp_isValidChar(chr)) {
                    stream->recv_data_buffer->strings[bVar5++] = stream->read_bytes_buf[i];
                }
        }
    }
    *lenout = i + 1;
    return stream->recv_data_buffer;
}

int pcppRecvPrompt(pcpp_t* stream, undefined4 param_2, int param_3) {
    uint* recv_buf_len;
    int iVar1;

    uint local_c = param_3;
    if (stream == NULL) {
        PCP_LOG("error PCPP stream isn\'t set\n");
        return e_pcpp_param_invalid;
    }

    e_pcpp_t err;
    amtime_t start;
    amiTimerGet(&start);
    while (true) {
        if (pcppCheckPrompt(stream)) {
            amiTimerGet(&start);
            stream->last_active = _amTimeMs(start);
            return e_pcpp_ok;
        }

        iVar1 = pcppGetBlockingTime(param_2, param_3, stream, _amTimeMs(start), &local_c);
        recv_buf_len = &stream->read_bytes_size;
        err = _errT2P(pcptRecv(&stream->sock, stream->read_bytes_buf + stream->read_bytes_num, recv_buf_len, local_c));
        if (err == e_pcpp_to) break;
        if (err != e_pcpp_ok) {
            return err;
        }
        stream->read_bytes_num = stream->read_bytes_num + *recv_buf_len;
        uint uVar3 = PCP_BUF_MAX - stream->read_bytes_num;
        *recv_buf_len = uVar3;
        if (uVar3 != 0) {
            stream->read_bytes_buf[stream->read_bytes_num] = '\0';
        }
    }

    if (iVar1 != 0) {
        err = stream->open ? e_pcpp_timeout_open : e_pcpp_timeout_closed;
        pcptCloseDataSock(&stream->sock);
        pcppResetRead(stream);
        return err;
    }
    return err;
}

e_pcpp_t pcppSendRequestMain(pcpp_t* stream, undefined4 param_2, timeout_t timeout_ms) {
    if (stream == NULL) {
        PCP_LOG("error PCPP stream isn\'t set\n");
        return e_pcpp_param_invalid;
    }

    if (stream->resp_buffer == NULL) {
        PCP_LOG("error Request buffer isn\'t set\n");
        return e_pcpp_param_invalid;
    }

    amtime_t now;
    amiTimerGet(&now);

    e_pcpp_t eVar1 =
        pcpp_something(&now, timeout_ms, stream, (pcpt_t*)stream, stream->resp_buffer->data, &stream->resp_buffer_len,
                       stream->field_0x204, param_2, stream->open ? e_pcpp_timeout_open : e_pcpp_timeout_closed);
    if (eVar1 != e_pcpp_to && eVar1 != e_pcpp_ok) {
        pcptCloseDataSock(&stream->sock);
        pcppResetRead(stream);
    }
    amiTimerGet(&now);
    stream->last_active = _amTimeMs(now);
    return eVar1;
}

// TODO: MINIMAL CLEANUP DONE HERE

e_pcpp_t pcppRecvBinary(pcpp_t* stream, unsigned char* recv_buf, size_t buf_len, uint param_4) {
    size_t* psVar1;
    unsigned char** send_buf;
    int iVar2;
    e_pcpt_t eVar3;
    e_pcpp_t eVar4;
    undefined4 uVar5;
    bool bVar6;
    uint recvb_local;
    amtime_t now;
    amtime_t local_10;
    amtime_t local_8;

    recvb_local = param_4;
    if (stream == NULL) {
        PCP_LOG("error PCPP stream isn\'t set\n");
        return e_pcpp_param_invalid;
    }
    if (recv_buf == NULL) {
        PCP_LOG("error Recv Bninary Buffer isn\'t set\n");
        return e_pcpp_param_invalid;
    }
    stream->field_0x214 = 0;
    amiTimerGet(&now);
    psVar1 = &stream->field_0x1f8;
    stream->recv_binary_buf = recv_buf;
    *psVar1 = buf_len;
    stream->recv_binary_buf_len = buf_len;
    stream->field_0x1fc = 0;
    stream->state = 11;
    memset(recv_buf, 0, buf_len);
    do {
        bVar6 = (stream->data_sock).open != PCPT_LISTENING;
        if (bVar6) {
            uVar5 = stream->field_0x208;
        } else {
            amiTimerGet(&local_8);
            uVar5 = stream->field_0x210;
            stream->last_active = _amTimeMs(local_8);
            ;
        }
        iVar2 = pcppGetBlockingTime(uVar5, param_4, stream, _amTimeMs(now), &recvb_local);
        stream->field_0x214 = 1;
        eVar3 = pcptRecv(&stream->data_sock, stream->recv_binary_buf + stream->field_0x1fc, psVar1, recvb_local);
        eVar4 = _errT2P(eVar3);
        stream->err = eVar4;
        if ((eVar4 == e_pcpp_to) && (iVar2 != 0)) {
            stream->err = bVar6 ? -13 : -11;
            pcptCloseDataSock(&stream->sock);
            stream->read_bytes_size = PCP_BUF_MAX;
            stream->read_bytes_num = 0;
            memset(stream->read_bytes_buf, 0, PCP_BUF_MAX);
            stream->resp_buffer = NULL;
            stream->resp_buffer_len = 0;
        }
        eVar4 = stream->err;
        if (eVar4 == e_pcpp_to) {
        LAB_00455e91:
            if (eVar4 != e_pcpp_ok) {
                return eVar4;
            }
        } else {
            if (eVar4 != e_pcpp_ok) {
                stream->state = 0;
                goto LAB_00455e91;
            }
        }
        stream->field_0x1fc = stream->field_0x1fc + *psVar1;
        *psVar1 = stream->recv_binary_buf_len - stream->field_0x1fc;
        stream->field_0x214 = 0;
        amiTimerGet(&local_10);
        stream->last_active = _amTimeMs(local_10);
        if (stream->recv_binary_buf_len < stream->field_0x1fc || stream->recv_binary_buf_len == stream->field_0x1fc) {
            send_buf = (unsigned char**)&stream->send_buf;
            psVar1 = &stream->send_buf_len;
            stream->state = 12;
            *send_buf[0] = PCP_CHAR_BINACK;
            *psVar1 = 1;
            iVar2 = pcppGetBlockingTime(stream->field_0x20c, param_4, stream, _amTimeMs(now), &recvb_local);
            eVar3 = pcptSend(&stream->sock, *send_buf, psVar1, stream->field_0x208, recvb_local);
            eVar4 = _errT2P(eVar3);
            stream->err = eVar4;
            if ((eVar4 == e_pcpp_to) && (iVar2 != 0)) {
                stream->err = -14;
                pcptCloseDataSock(&stream->sock);
                pcppResetRead(stream);
            }
            eVar4 = stream->err;
            if (eVar4 != e_pcpp_to) {
                stream->state = 0;
            }
            if (eVar4 == e_pcpp_ok) {
                amiTimerGet(&local_10);
                stream->last_active = _amTimeMs(local_10);
                stream->state = 13;
                iVar2 = pcppGetBlockingTime(stream->field_0x208, param_4, stream, _amTimeMs(now), &recvb_local);
                *psVar1 = PCP_SEND_BUF_MAX;
                ZERO(stream->send_buf);
                eVar3 = pcptRecv(&stream->data_sock, *send_buf, psVar1, recvb_local);
                eVar4 = _errT2P(eVar3);
                stream->err = eVar4;
                if ((iVar2 != 0) && (eVar4 == e_pcpp_to)) {
                    stream->err = -13;
                }
                if (stream->err != e_pcpp_to) {
                    stream->state = 0;
                }
                if (stream->err == e_pcpp_closed) {
                    stream->err = e_pcpp_ok;
                }
                eVar4 = stream->err;
            }
            return eVar4;
        }
    } while (true);
}

e_pcpp_t pcppSendBinary(pcpp_t* stream, unsigned char* send_binary_buffer, size_t param_3, uint param_4)

{
    size_t* psVar1;
    uint pVar2;
    e_pcpp_t eVar3;
    int iVar4;
    e_pcpt_t eVar5;
    uint uVar6;
    uint local_14;
    amtime_t local_10;
    amtime_t local_8;

    local_14 = param_4;
    if (stream == NULL) {
        PCP_LOG("error PCPP stream isn\'t set\n");
        return e_pcpp_param_invalid;
    }
    if (send_binary_buffer == NULL) {
        PCP_LOG("error Send Bninary Buffer isn\'t set\n");
        return stream->err = e_pcpp_param_invalid;
    }
    amiTimerGet(&local_8);
    pVar2 = (stream->data_sock).open;
    psVar1 = &stream->field_0x1f0;
    stream->state = 9;
    *psVar1 = param_3;
    stream->send_binary_buf = send_binary_buffer;
    if (pVar2 == PCPT_LISTENING) {
        amiTimerGet(&local_10);
        stream->last_active = _amTimeMs(local_10);
        eVar3 = pcpp_something(&local_8, param_4, stream, &stream->data_sock, stream->send_binary_buf, psVar1,
                               stream->field_0x208, stream->field_0x210, -11);
        stream->err = eVar3;
    } else {
        eVar5 = pcptSend(&stream->data_sock, send_binary_buffer, psVar1, stream->field_0x208, param_4);
        eVar3 = _errT2P(eVar5);
        iVar4 = (stream->data_sock).field_0x54;
        stream->err = eVar3;
        if (iVar4 != 0) {
            amiTimerGet(&local_10);
            stream->last_active = _amTimeMs(local_10);
        }
    }
    eVar3 = stream->err;
    if (eVar3 != e_pcpp_to) {
        stream->state = 0;
    }
    if (eVar3 == e_pcpp_ok) {
        amiTimerGet(&local_10);
        stream->last_active = _amTimeMs(local_10);
        stream->state = 10;
        psVar1 = &stream->send_buf_len;
        *psVar1 = 8;
        ZERO(stream->send_buf);
        stream->field_0x1e8 = 0;
        iVar4 = pcppGetBlockingTime(stream->field_0x20c, param_4, stream, _amTimeMs(local_8), &local_14);
        eVar5 = pcptRecv(&stream->sock, stream->send_buf, psVar1, local_14);
        eVar3 = _errT2P(eVar5);
        stream->err = eVar3;
        if ((iVar4 != 0) && (eVar3 == e_pcpp_to)) {
            stream->err = -14;
            pcptCloseDataSock(&stream->sock);
            pcppResetRead(stream);
        }
        eVar3 = stream->err;
        if (eVar3 != e_pcpp_to) {
            stream->state = 0;
        }
        if (eVar3 == e_pcpp_ok) {
            stream->field_0x1e8 = stream->field_0x1e8 + *psVar1;
            uVar6 = 0;
            *psVar1 = 8 - stream->field_0x1e8;
            stream->state = 0;
            stream->err = -9;
            if (stream->field_0x1e8 != 0) {
                while (*(char*)((int)&stream->send_buf + uVar6) != PCP_CHAR_BINACK) {
                    uVar6 = uVar6 + 1;
                    if (stream->field_0x1e8 <= uVar6) {
                        return stream->err;
                    }
                }
                stream->err = e_pcpp_ok;
            }
            eVar3 = stream->err;
        }
    }
    return eVar3;
}

e_pcpp_t pcppIsBusy(pcpp_t* stream, timeout_t timeout) {
    size_t* psVar1;
    unsigned char** send_buf;
    undefined4 uVar2;
    int iVar3;
    e_pcpt_t eVar4;
    int iVar5;
    uint uVar6;
    e_pcpp_t eVar7;
    unsigned char* pcVar8;
    unsigned char* pcVar9;
    uint uVar10;
    uint uVar11;
    bool bVar12;
    bool local_24;
    int local_20;
    int local_1c;
    amtime_t local_18;
    amtime_t local_10;
    amtime_t local_8;
    pcpt_t* sock;

    uVar11 = timeout;
    bVar12 = false;
    local_20 = 0;
    local_1c = 0;
    local_24 = 0;
    if (stream == NULL) {
        PCP_LOG("error PCPP stream isn\'t set\n");
        return e_pcpp_param_invalid;
    }
    amiTimerGet(&local_18);
    switch (stream->state) {
        case 0:
            stream->err = e_pcpp_no_client;
            break;
        case 1:
        case 8:
            if (stream->state == 1) {
                if (stream->open == 0) {
                    uVar2 = stream->field_0x210;
                } else {
                    uVar2 = stream->field_0xb8;
                }
                iVar3 = _amTimeMs(local_18);
            } else {
                iVar3 = _amTimeMs(local_18);
                uVar2 = stream->field_0x210;
            }
            iVar3 = pcppGetBlockingTime(uVar2, uVar11, stream, iVar3, &timeout);
            sock = &stream->sock;
            if (stream->state != 1) {
                sock = &stream->data_sock;
            }
            eVar4 = pcptIsBusy(sock, timeout);
            eVar7 = _errT2P(eVar4);
            stream->err = eVar7;
            if (eVar7 != e_pcpp_to) {
                if (eVar7 == e_pcpp_ok) {
                    amiTimerGet(&local_10);
                    stream->last_active = _amTimeMs(local_10);
                    if (stream->state != 8) {
                        if (stream->open != 0) {
                            stream->read_bytes_num = stream->read_bytes_num + stream->read_bytes_size;
                            stream->read_bytes_size = PCP_BUF_MAX - stream->read_bytes_num;
                            iVar3 = pcppCheckPrompt(stream);
                            if (iVar3 != 0) goto LAB_00457676;
                            stream->err = ~e_pcpp_no_client;
                            goto LAB_00457660;
                        }
                        stream->state = 3;
                        if (uVar11 != TIMEOUT_NONE) {
                            amiTimerGet(&local_8);
                            uVar6 = (local_8.microseconds - local_18.microseconds) / 1000 +
                                    (local_8.seconds - local_18.seconds) * 1000;
                            if (uVar6 < uVar11) {
                                uVar11 = uVar11 - uVar6;
                            } else {
                                uVar11 = 0;
                            }
                        }
                        stream->open = 1;
                        eVar7 = pcppRecvPrompt(stream, stream->field_0xb8, uVar11);
                        stream->err = eVar7;
                        if (eVar7 != e_pcpp_to) {
                            amiTimerGet(&local_10);
                            stream->state = 0;
                            stream->last_active = _amTimeMs(local_10);
                            return stream->err;
                        }
                    }
                    break;
                }
                goto LAB_004577b4;
            }
            if (iVar3 == 0) break;
            if (stream->state == 1) {
                stream->err = 0xfffffff5 - (uint)(stream->open != 0);
            } else {
                stream->err = -11;
            }
            goto LAB_00457392;
        case 2:
        case 4:
        case 6:
            uVar6 = uVar11;
            if ((stream->sock).field_0x54 != 0) {
                bVar12 = (stream->sock).open != PCPT_LISTENING;
                if (bVar12) {
                    uVar2 = stream->field_0xb8;
                } else {
                    uVar2 = stream->field_0x210;
                }
                bVar12 = !bVar12;
                local_20 = pcppGetBlockingTime(uVar2, uVar11, stream, _amTimeMs(local_18), &timeout);
                uVar6 = timeout;
            }
            eVar4 = pcptIsBusy(&stream->sock, uVar6);
            eVar7 = _errT2P(eVar4);
            stream->err = eVar7;
            if (eVar7 != e_pcpp_to) {
                if (eVar7 == e_pcpp_ok) {
                    amiTimerGet(&local_10);
                    stream->last_active = _amTimeMs(local_10);
                    if (stream->state != 2) goto LAB_004577b4;
                    stream->open = 0;
                    amiTimerGet(&local_8);
                    stream->state = 5;
                    iVar3 = pcppGetBlockingTime(stream->field_0xb8, uVar11, stream, _amTimeMs(local_18), &timeout);
                    eVar7 = pcppRecvRequestMain(stream, &local_24, timeout);
                    stream->err = eVar7;
                    if ((eVar7 == e_pcpp_to) && (iVar3 != 0)) {
                        stream->err = e_pcpp_timeout_closed;
                        pcptCloseDataSock(&stream->sock);
                        pcppResetRead(stream);
                    }
                    if (stream->err == e_pcpp_to) {
                        if (local_24 != 0) {
                            stream->state = 14;
                        }
                    } else {
                        stream->state = 0;
                    }
                    if (stream->err == e_pcpp_ok) goto LAB_004574b8;
                    break;
                }
                goto LAB_004577af;
            }
            if (local_20 == 0) break;
            stream->err = (-(uint)bVar12 & 0xfffffffb) - 6;
            goto LAB_00457392;
        case 3:
        case 5:
        case 7:
            if (stream->field_0x214 == 0) {
                uVar2 = stream->field_0xb8;
            } else {
                uVar2 = stream->field_0x204;
            }
            iVar3 = pcppGetBlockingTime(uVar2, uVar11, stream, _amTimeMs(local_18), &timeout);
            uVar6 = timeout;
            eVar4 = pcptIsBusy(&stream->sock, timeout);
            eVar7 = _errT2P(eVar4);
            stream->err = eVar7;
            if (eVar7 != e_pcpp_to) {
                if (eVar7 != e_pcpp_ok) {
                LAB_00457660:
                    stream->state = 0;
                    return stream->err;
                }
                stream->read_bytes_num = stream->read_bytes_num + stream->read_bytes_size;
                stream->field_0x214 = 0;
                stream->read_bytes_size = PCP_BUF_MAX - stream->read_bytes_num;
                if (stream->state == 3) {
                    uVar6 = uVar11;
                    if (uVar11 != TIMEOUT_NONE) {
                        amiTimerGet(&local_8);
                        uVar6 = -(uint)((uint)((local_8.microseconds - local_18.microseconds) / 1000 +
                                               (local_8.seconds - local_18.seconds) * 1000) < uVar11) &
                                uVar11;
                    }
                    eVar7 = pcppRecvPrompt(stream, stream->field_0xb8, uVar6);
                    stream->err = eVar7;
                    if (eVar7 != e_pcpp_to) {
                        if (eVar7 != e_pcpp_ok) goto LAB_00457660;
                        if (stream->open == 0) {
                            if (uVar11 != TIMEOUT_NONE) {
                                amiTimerGet(&local_8);
                                uVar11 = -(uint)((uint)((local_8.microseconds - local_18.microseconds) / 1000 +
                                                        (local_8.seconds - local_18.seconds) * 1000) < uVar11) &
                                         uVar11;
                            }
                            stream->state = 4;
                            eVar7 = pcppSendRequestMain(stream, stream->field_0xb8, uVar11);
                            stream->err = eVar7;
                            if ((eVar7 != e_pcpp_to) && (eVar7 != e_pcpp_ok)) {
                                stream->state = 0;
                                return eVar7;
                            }
                        }
                    }
                    break;
                }
                if (stream->read_bytes_num == 0) {
                LAB_0045848d:
                    amiTimerGet(&local_8);
                    stream->last_active = _amTimeMs(local_8);
                    uVar10 = _amTimeDelta(local_8, local_18);
                    stream->field_0x214 = 1;
                    if (uVar11 != TIMEOUT_NONE) {
                        if (uVar11 <= uVar10) {
                            stream->state = stream->state == 5 ? 14 : 15;
                            return stream->err = e_pcpp_to;
                        }
                        uVar6 = uVar11 - uVar10;
                    }
                    eVar7 = pcppRecvAllMsg(uVar6, stream, &local_24);
                    stream->err = eVar7;
                    if (eVar7 != e_pcpp_ok) {
                        if (eVar7 == e_pcpp_to) {
                            if (local_24 != 0) {
                                stream->state = stream->state == 5 ? 14 : 15;
                                return e_pcpp_to;
                            }
                            break;
                        }
                        goto LAB_0045739d;
                    }
                } else {
                    uVar10 = stream->read_bytes_num;
                    pcVar8 = stream->read_bytes_buf;
                    iVar3 = local_1c;
                    do {
                        if ((*pcVar8 == '\r') || (*pcVar8 == '\n')) {
                            iVar3 = 1;
                        }
                        pcVar8 = pcVar8 + 1;
                        uVar10 = uVar10 - 1;
                    } while (uVar10 != 0);
                    if (iVar3 == 0) goto LAB_0045848d;
                }
                pcVar8 = stream->read_bytes_buf;
                eVar7 = pcppCheckRecvMsg(pcVar8, stream->read_bytes_num, (uint)(stream->state == 5));
                stream->err = eVar7;
                if (eVar7 == e_pcpp_ok) {
                    pcppChangeRequest(stream, &timeout);
                    uVar11 = timeout;
                    pcVar9 = pcVar8;
                    if (timeout < stream->read_bytes_num) {
                        do {
                            *pcVar9 = pcVar8[uVar11];
                            uVar11 = uVar11 + 1;
                            pcVar9 = pcVar9 + 1;
                        } while (uVar11 < stream->read_bytes_num);
                    }
                LAB_00458226:
                    stream->read_bytes_num = stream->read_bytes_num - timeout;
                    stream->read_bytes_size = stream->read_bytes_size + timeout;
                    stream->state = 0;
                    amiTimerGet(&local_18);
                    stream->last_active = _amTimeMs(local_18);
                    return stream->err;
                }
                stream->state = 0;
                pcppResetRead(stream);
                PCP_LOG("error Response format error\n");
                return stream->err;
            }
            if (iVar3 == 0) break;
            if (stream->field_0x214 == 0) {
                stream->err = (-(uint)(stream->open != 0) & 0xfffffffa) - 6;
            } else {
                stream->err = -7;
            }
        LAB_00457392:
            pcptCloseDataSock(&stream->sock);
            pcppResetRead(stream);
        LAB_0045739d:
            stream->state = 0;
            return stream->err;
        case 9:
            bVar12 = (stream->data_sock).open != PCPT_LISTENING;
            if (bVar12) {
                uVar2 = stream->field_0x208;
            } else {
                uVar2 = stream->field_0x210;
            }
            iVar5 = pcppGetBlockingTime(uVar2, uVar11, stream, _amTimeMs(local_18), &timeout);
            eVar4 = pcptIsBusy(&stream->data_sock, timeout);
            eVar7 = _errT2P(eVar4);
            iVar3 = (stream->data_sock).field_0x54;
            stream->err = eVar7;
            if (iVar3 == 0) {
                if ((iVar5 != 0) && (eVar7 == e_pcpp_to)) {
                    stream->err = (uint)!bVar12 * 2 + -13;
                    pcptCloseDataSock(&stream->sock);
                    pcppResetRead(stream);
                }
            } else {
                amiTimerGet(&local_10);
                stream->last_active = _amTimeMs(local_10);
            }
            if (stream->err != e_pcpp_to) {
                stream->state = 0;
            }
            if (stream->err == e_pcpp_ok) {
                amiTimerGet(&local_10);
                stream->last_active = _amTimeMs(local_10);
                stream->state = 10;
                psVar1 = &stream->send_buf_len;
                *psVar1 = PCP_SEND_BUF_MAX;
                ZERO(stream->send_buf);
                stream->field_0x1e8 = 0;
                iVar3 = pcppGetBlockingTime(stream->field_0x20c, uVar11, stream, _amTimeMs(local_18), &timeout);
                eVar4 = pcptRecv(&stream->sock, stream->send_buf, psVar1, timeout);
                eVar7 = _errT2P(eVar4);
                stream->err = eVar7;
                if ((iVar3 != 0) && (eVar7 == e_pcpp_to)) {
                    stream->err = -14;
                    pcptCloseDataSock(&stream->sock);
                    pcppResetRead(stream);
                }
                if (stream->err != e_pcpp_to) {
                    stream->state = 0;
                }
                if (stream->err == e_pcpp_ok) {
                    stream->field_0x1e8 = stream->field_0x1e8 + *psVar1;
                    uVar11 = 0;
                    *psVar1 = PCP_SEND_BUF_MAX - stream->field_0x1e8;
                    stream->state = 0;
                    stream->err = -9;
                    if (stream->field_0x1e8 != 0) {
                        do {
                            if (*(char*)((int)&stream->send_buf + uVar11) == PCP_CHAR_BINACK) {
                                stream->err = e_pcpp_ok;
                                break;
                            }
                            uVar11 = uVar11 + 1;
                        } while (uVar11 < stream->field_0x1e8);
                    }
                    amiTimerGet(&local_10);
                    stream->last_active = _amTimeMs(local_10);
                    return stream->err;
                }
            }
            break;
        case 10:
            iVar3 = pcppGetBlockingTime(stream->field_0x20c, uVar11, stream, _amTimeMs(local_18), &timeout);
            eVar4 = pcptIsBusy(&stream->sock, timeout);
            eVar7 = _errT2P(eVar4);
            stream->err = eVar7;
            if ((iVar3 != 0) && (eVar7 == e_pcpp_to)) {
                stream->err = -14;
                pcptCloseDataSock(&stream->sock);
                pcppResetRead(stream);
            }
            if (stream->err != e_pcpp_to) {
                stream->state = 0;
            }
            if (stream->err == e_pcpp_ok) {
                amiTimerGet(&local_10);
                stream->field_0x1e8 = stream->field_0x1e8 + stream->send_buf_len;
                stream->last_active = _amTimeMs(local_10);
                ;
                uVar11 = 0;
                stream->send_buf_len = PCP_SEND_BUF_MAX - stream->field_0x1e8;
                stream->err = -9;
                if (stream->field_0x1e8 != 0) {
                    while (*(char*)((int)&stream->send_buf + uVar11) != PCP_CHAR_BINACK) {
                        uVar11 = uVar11 + 1;
                        if (stream->field_0x1e8 <= uVar11) {
                            return stream->err;
                        }
                    }
                LAB_00457676:
                    stream->err = e_pcpp_ok;
                    return e_pcpp_ok;
                }
            }
            break;
        case 11:
            if (stream->field_0x214 != 0) {
                bVar12 = (stream->data_sock).open != PCPT_LISTENING;
                if (bVar12) {
                    uVar2 = stream->field_0x208;
                } else {
                    uVar2 = stream->field_0x210;
                }
                iVar3 = pcppGetBlockingTime(uVar2, uVar11, stream, _amTimeMs(local_18), &timeout);
                eVar4 = pcptIsBusy(&stream->data_sock, timeout);
                eVar7 = _errT2P(eVar4);
                stream->err = eVar7;
                if ((eVar7 == e_pcpp_to) && (iVar3 != 0)) {
                    stream->err = (uint)!bVar12 * 2 + -13;
                    pcptCloseDataSock(&stream->sock);
                    pcppResetRead(stream);
                }
                eVar7 = stream->err;
                if (eVar7 == e_pcpp_to) {
                LAB_00457b2e:
                    if (eVar7 != e_pcpp_ok) {
                        return eVar7;
                    }
                } else {
                    if (eVar7 != e_pcpp_ok) {
                        stream->field_0x214 = 0;
                        stream->state = 0;
                        goto LAB_00457b2e;
                    }
                }
                stream->field_0x1fc = stream->field_0x1fc + stream->field_0x1f8;
                stream->field_0x214 = 0;
                stream->field_0x1f8 = stream->recv_binary_buf_len - stream->field_0x1fc;
                amiTimerGet(&local_10);
                stream->last_active = _amTimeMs(local_10);
            }
            if (stream->field_0x1fc <= stream->recv_binary_buf_len &&
                stream->recv_binary_buf_len != stream->field_0x1fc) {
                psVar1 = &stream->field_0x1f8;
                do {
                    iVar3 = pcppGetBlockingTime(stream->field_0x208, uVar11, stream, _amTimeMs(local_18), &timeout);
                    stream->field_0x214 = 1;
                    eVar4 =
                        pcptRecv(&stream->data_sock, stream->recv_binary_buf + stream->field_0x1fc, psVar1, timeout);
                    eVar7 = _errT2P(eVar4);
                    stream->err = eVar7;
                    if ((eVar7 == e_pcpp_to) && (iVar3 != 0)) {
                        stream->err = -13;
                        pcptCloseDataSock(&stream->sock);
                        pcppResetRead(stream);
                    }
                    eVar7 = stream->err;
                    if (eVar7 == e_pcpp_to) {
                    LAB_00457c47:
                        if (eVar7 != e_pcpp_ok) goto switchD_004572ff_caseD_10;
                    } else {
                        if (eVar7 != e_pcpp_ok) {
                            stream->state = 0;
                            goto LAB_00457c47;
                        }
                    }
                    stream->field_0x1fc = stream->field_0x1fc + *psVar1;
                    stream->field_0x214 = 0;
                    *psVar1 = stream->recv_binary_buf_len - stream->field_0x1fc;
                    amiTimerGet(&local_10);
                    stream->last_active = _amTimeMs(local_10);
                } while (stream->field_0x1fc <= stream->recv_binary_buf_len &&
                         stream->recv_binary_buf_len != stream->field_0x1fc);
            }
            send_buf = (unsigned char**)&stream->send_buf;
            psVar1 = &stream->send_buf_len;
            stream->state = 12;
            *send_buf[0] = PCP_CHAR_BINACK;
            *psVar1 = 1;
            local_20 = pcppGetBlockingTime(stream->field_0x20c, uVar11, stream, _amTimeMs(local_18), &timeout);
            eVar4 = pcptSend(&stream->sock, *send_buf, psVar1, stream->field_0x204, timeout);
            eVar7 = _errT2P(eVar4);
            stream->err = eVar7;
            if ((eVar7 == e_pcpp_to) && (local_20 != 0)) {
                stream->err = -14;
                pcptCloseDataSock(&stream->sock);
                pcppResetRead(stream);
            }
            if (stream->err != e_pcpp_to) {
                stream->state = 0;
            }
            if (stream->err == e_pcpp_ok) {
                amiTimerGet(&local_10);
                stream->last_active = _amTimeMs(local_10);
                stream->state = 13;
                iVar3 = pcppGetBlockingTime(stream->field_0x208, uVar11, stream, _amTimeMs(local_18), &timeout);
                *psVar1 = PCP_SEND_BUF_MAX;
                ZERO(stream->send_buf);
                eVar4 = pcptRecv(&stream->data_sock, *send_buf, psVar1, timeout);
                eVar7 = _errT2P(eVar4);
                stream->err = eVar7;
                if ((iVar3 != 0) && (eVar7 == e_pcpp_to)) {
                    stream->err = -13;
                }
                if (stream->err != e_pcpp_to) {
                    stream->state = 0;
                }
                if (stream->err == e_pcpp_closed) {
                    stream->err = e_pcpp_ok;
                LAB_004574b8:
                    amiTimerGet(&local_10);
                    stream->last_active = _amTimeMs(local_10);
                    return stream->err;
                }
            }
            break;
        case 12:
            iVar3 = pcppGetBlockingTime(stream->field_0x20c, uVar11, stream, _amTimeMs(local_18), &timeout);
            eVar4 = pcptIsBusy(&stream->sock, timeout);
            eVar7 = _errT2P(eVar4);
            stream->err = eVar7;
            if ((iVar3 != 0) && (eVar7 == e_pcpp_to)) {
                stream->err = -14;
                pcptCloseDataSock(&stream->sock);
                pcppResetRead(stream);
            }
            if (stream->err != e_pcpp_to) {
                stream->state = 0;
            }
            if (stream->err != e_pcpp_ok) break;
            stream->state = 13;
            amiTimerGet(&local_10);
            stream->last_active = _amTimeMs(local_10);
            iVar3 = pcppGetBlockingTime(stream->field_0x208, uVar11, stream, _amTimeMs(local_18), &timeout);
            stream->send_buf_len = PCP_SEND_BUF_MAX;
            ZERO(stream->send_buf);
            eVar4 = pcptRecv(&stream->data_sock, stream->send_buf, &stream->send_buf_len, timeout);
            eVar7 = _errT2P(eVar4);
            stream->err = eVar7;
            if ((iVar3 != 0) && (eVar7 == e_pcpp_to)) {
                stream->err = -13;
                pcptCloseDataSock(&stream->sock);
                pcppResetRead(stream);
            }
            if (stream->err != e_pcpp_to) stream->state = 0;

            if (stream->err != e_pcpp_closed) break;
            stream->err = e_pcpp_ok;
            goto LAB_004574b8;
        case 13:
            iVar3 = pcppGetBlockingTime(stream->field_0x208, uVar11, stream, _amTimeMs(local_18), &timeout);
            eVar4 = pcptIsBusy(&stream->data_sock, timeout);
            eVar7 = _errT2P(eVar4);
            stream->err = eVar7;
            if ((iVar3 == 0) || (eVar7 != e_pcpp_to)) {
                if (eVar7 == e_pcpp_closed) {
                    stream->err = e_pcpp_ok;
                    amiTimerGet(&local_10);
                    stream->last_active = _amTimeMs(local_10);
                    return stream->err;
                }
                break;
            }
            stream->err = -13;
            pcptCloseDataSock(&stream->sock);
        LAB_004577af:
            pcppResetRead(stream);
        LAB_004577b4:
            stream->state = 0;
            return stream->err;
        case 14:
        case 15:
            amiTimerGet(&local_8);
            uVar6 = _amTimeDelta(local_8, local_18);
            stream->field_0x214 = 1;
            if (uVar11 != TIMEOUT_NONE) {
                if (uVar6 < uVar11) {
                    uVar11 = uVar11 - uVar6;
                } else {
                    uVar11 = 0;
                }
            }
            eVar7 = pcppRecvAllMsg(uVar11, stream, &local_24);
            stream->err = eVar7;
            if (eVar7 == e_pcpp_ok) {
                pcVar8 = stream->read_bytes_buf;
                eVar7 = pcppCheckRecvMsg(pcVar8, stream->read_bytes_num, (uint)(stream->state == 14));
                stream->err = eVar7;
                if (eVar7 != e_pcpp_ok) {
                    stream->state = 0;
                    pcppResetRead(stream);
                    PCP_LOG("error Response format error\n");
                    return stream->err;
                }
                pcppChangeRequest(stream, &timeout);
                uVar11 = timeout;
                pcVar9 = pcVar8;
                if (timeout < stream->read_bytes_num) {
                    do {
                        *pcVar9 = pcVar8[uVar11];
                        uVar11 = uVar11 + 1;
                        pcVar9 = pcVar9 + 1;
                    } while (uVar11 < stream->read_bytes_num);
                }
                goto LAB_00458226;
            }
            if (eVar7 == e_pcpp_to) {
                if (local_24 == 0) {
                    stream->state = stream->state == 14 ? 5 : 7;
                    return stream->err;
                }
                break;
            }
            goto LAB_0045739d;
    }
switchD_004572ff_caseD_10:
    return stream->err;
}

int pcpp_something(amtime_t* time, timeout_t timeout, pcpp_t* stream, pcpt_t* sock, unsigned char* send_buf,
                   size_t* send_len, undefined4 param_7, undefined4 param_8, int fallback_err) {
    uint iVar1;
    uint blocking_time;
    amtime_t now;
    e_pcpp_t err;

    iVar1 = pcppGetBlockingTime(param_8, timeout, stream, _amTimeMs(*time), &blocking_time);
    err = _errT2P(pcptSend(sock, send_buf, send_len, param_7, blocking_time));
    if (err != e_pcpp_to) return err;

    if (sock->field_0x54 != 0) {
        if (timeout == TIMEOUT_NONE) return _errT2P(pcptIsBusy(sock, TIMEOUT_NONE));

        amiTimerGet(&now);
        iVar1 = FUN_00459d00(time, &now);
        if ((iVar1 / 1000) < timeout) {
            return _errT2P(pcptIsBusy(sock, timeout - iVar1 / 1000));
        }
        return _errT2P(pcptIsBusy(sock, 0));
    }

    if (iVar1 != 0) {
        return fallback_err;
    }
    return err;
}

pcp_send_data_t* pcppAddSendPacket(pcp_send_data_t* send_data, char* keyword, char* value) {
    if (send_data == NULL || keyword == NULL) {
        PCP_LOG("error Send Data isn\'t set\n");
        return NULL;
    }

    size_t existing_length = send_data->length;
    if (existing_length == 0) return pcppSetSendPacket(send_data, keyword, value);

    size_t kwLen = strlen(keyword);
    if (value == NULL) {
        if (existing_length + kwLen + 3 < PCP_BUF_MAX) {
            sprintf_s((char*)&send_data->data[existing_length], PCP_BUF_MAX - existing_length, "&%s\r\n", keyword);
            send_data->length += kwLen + 1;
            return send_data;
        }
    } else {
        size_t valLen = strlen(value);
        if (existing_length + kwLen + valLen + 4 < PCP_BUF_MAX) {
            sprintf_s((char*)&send_data->data[existing_length], PCP_BUF_MAX - existing_length, "&%s=%s\r\n", keyword, value);
            send_data->length += valLen + kwLen + 2;
            return send_data;
        }
    }
    return NULL;
}
