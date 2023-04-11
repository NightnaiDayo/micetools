#include "pcpp.h"

int FUN_00459d00(amtime_t* t1, amtime_t* t2) {
    if (t1 == NULL || t2 == NULL) return -1;
    return ((t2->seconds - t1->seconds) * 1000000 - t1->microseconds) +
           t2->microseconds;  // Swapped
}

// TODO: Invert these conditions to be clearer
int pcppIsValidDataChar(char chr) {
    if (isalnum(chr) || chr == '.' || chr == '_' || chr == '-' || chr == ':' || chr == '@' ||
        chr == '%' || chr == '/' || chr == '\\' || chr == '#' || chr == '{' || chr == '}') {
        return 1;
    }
    return 0;
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

/**
 * @brief Locate a PCP prompt string. The last prompt located in the
 * buffer will be used, and all data following it moved to the
 * beginning of the buffer.
 *
 * @return true
 * @return false
 */
bool pcppCheckPrompt(pcpp_t* stream) {
    if (stream == NULL) {
        amiDebugLog("error PCPP stream isn't set");
        return false;
    }

    uint max = stream->read_bytes_num;
    bool found = false;

    for (uint cursor = 0; cursor < max; cursor++) {
        char* pos = stream->read_bytes_buf + cursor;
        // Scan for the prompt character
        if (stream->read_bytes_buf[cursor] != PCP_CHAR_PROMPT) continue;

        found = true;

        // Shift everything one character left (clobber the prompt)
        for (uint i = cursor; i < stream->read_bytes_num - 1; i++) {
            pos[i] = pos[i + 1];
        }

        // Adjust counts accordingly
        stream->read_bytes_num--;
        stream->read_bytes_size++;
    }

    return found;
}

e_pcpp_t pcppCheckRecvMsg(char* recv_data, size_t buf_len, int param_3) {
    bool bVar1 = true;

    if (recv_data == NULL) {
        amiDebugLog("pointer error");
        return e_pcpp_param_invalid;
    }

    int check_offset = 0;
    while (1) {
        check_offset = buf_len - check_offset;
        uint recv_err = pcppRecvCheck(recv_data, &check_offset);

        if (recv_err == 1) return e_pcpp_param_invalid;
        if (recv_err == 8) {
            if (!bVar1) return e_pcpp_param_invalid;
            if (param_3 != 0) return e_pcpp_param_invalid;
        } else {
            if (recv_err == 7 && !bVar1) return e_pcpp_param_invalid;
        }

        recv_data += check_offset;
        check_offset++;
        bVar1 = false;

        if (recv_err == 0 || recv_err == 6 || recv_err == 7 || recv_err == 8) {
            return e_pcpp_ok;
        }
    }
}

pcpp_char_type_t pcppCheckStr(char* recv_data, uint* found_at, int* is_qmark) {
    bool end = false;
    bool saw_data = false;
    uint idx = 0;

    if (recv_data == NULL || is_qmark == NULL || found_at == NULL) {
        amiDebugLog("pointer error");
        return pcpp_char_error;
    }

    *is_qmark = 0;
    if (*found_at == 0) {
        amiDebugLog("stream exhausted");
        return pcpp_char_error;
    }

    do {
        char chr = recv_data[idx];
        if (PCP_WHITESPACE(chr)) {
            if (saw_data) end = true;
        } else if (pcppIsValidDataChar(chr)) {
            if (end || *is_qmark != 0) {
                *found_at = idx + 1;
                return pcpp_char_error;
            }
            saw_data = true;
        } else if (chr == PCP_CHAR_QMARK) {
            if (saw_data || end) {
                *found_at = idx + 1;
                return pcpp_char_error;
            }

            *is_qmark = 1;
        } else {
            *found_at = idx + 1;
            switch (chr) {
                case PCP_CHAR_SEP:
                    return pcpp_char_sep;
                case PCP_CHAR_CR:
                    if (recv_data[idx + 1] != '\n') return pcpp_char_crlf;
                    *found_at = idx + 2;
                    return pcpp_char_cr;
                case PCP_CHAR_LF:
                    return pcpp_char_lf;
                case PCP_CHAR_EQU:
                    return pcpp_char_equ;
                default:
                    return pcpp_char_error;
            }
        }

        idx++;
        if (*found_at <= idx) return pcpp_char_error;
    } while (true);
}

void pcppClose(pcpp_t* stream) {
    if (stream == NULL) {
        amiDebugLog("error PCPP stream isn't set");
        return;
    }

    pcptClose(&stream->sock);
    pcptClose(&stream->data_sock);

    stream->field_0xb8 = 0;
    stream->last_active = 0;
    stream->state = pcpp_state_none;
    stream->err = e_pcpp_unknown;
    stream->recv_data_buffer = NULL;
    memset(stream->read_bytes_buf, 0, sizeof stream->read_bytes_buf);
    stream->read_bytes_size = sizeof stream->read_bytes_buf;
    stream->read_bytes_num = 0;
    stream->resp_buffer = NULL;
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
        amiDebugLog("error PCPP stream isn't set");
        return;
    }

    pcptCloseDataSock(&stream->data_sock);
    stream->read_bytes_size = sizeof stream->read_bytes_buf;
    stream->read_bytes_num = 0;
    memset(stream->read_bytes_buf, 0, sizeof stream->read_bytes_buf);
    stream->resp_buffer = NULL;

    amtime_t time;
    amiTimerGet(&time);
    stream->last_active = _amTimeMs(time);
    stream->state = pcpp_state_none;
}

e_pcpp_t pcppGetBlockingTime(uint param_1, timeout_t timeout, pcpp_t* stream, int param_4,
                             timeout_t* blocking_time) {
    byte bVar1;
    uint uVar2;
    uint uVar3;
    amtime_t local_8;

    if (stream == NULL || blocking_time == NULL) {
        amiDebugLog("pointer error");
        return 0;
    }

    amiTimerGet(&local_8);
    uVar2 =
        (local_8.seconds * 1000 - stream->last_active) + local_8.microseconds / 1000;  // swapped
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
        amiDebugLog("error Recv Data isn't set");
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
        amiDebugLog("error Recv Data isn't set");
        return NULL;
    }

    if ((keywordNum >= PCP_CMDS_MAX) && (keywordNum > recvData->cmd_count)) {
        amiDebugLog("error keywordNum over");
        return NULL;
    }
    return recvData->strings + recvData->keywords[keywordNum];
}

SOCKET pcppGetServerSocket(pcpp_t* stream, int which) {
    if (stream == NULL) {
        amiDebugLog("error PCPP stream isn't set");
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
        amiDebugLog("error don't set stream");
        return e_pcpp_param_invalid;
    }

    e_pcpt_t err = pcptInitStream(&stream->sock);
    if (err != e_pcpt_ok) return _errT2P(err);

    err = pcptInitStream(&stream->data_sock);
    if (err != e_pcpt_ok) return _errT2P(err);

    pcptSetConfig(&stream->data_sock, PCPT_TCP_NODELAY, 1);

    stream->field_0xb8 = 0;
    stream->last_active = 0;
    stream->state = pcpp_state_none;
    stream->err = e_pcpp_unknown;
    stream->recv_data_buffer = NULL;
    ZERO(stream->read_bytes_buf);
    // memset(stream->read_bytes_buf, 0, sizeof stream->read_bytes_buf);
    stream->read_bytes_size = sizeof stream->read_bytes_buf;
    stream->read_bytes_num = 0;
    stream->resp_buffer = NULL;
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

e_pcpp_t pcppOpenClient(pcpp_t* stream, char* ipAddr, ushort port, uint param_4,
                        timeout_t timeout) {
    e_pcpp_t iVar1;
    timeout_t uVar3;
    amtime_t start;
    amtime_t now;

    if (stream == NULL) {
        amiDebugLog("error PCPP stream isn't set");
        return e_pcpp_param_invalid;
    }
    if (ipAddr == NULL) {
        amiDebugLog("error IpAddr isn't set");
        return e_pcpp_param_invalid;
    }

    stream->state = pcpp_state_open;
    stream->field_0xb8 = param_4;
    stream->open = 0;
    amiTimerGet(&start);
    iVar1 = start.microseconds / 1000 + start.seconds * 1000;
    stream->last_active = iVar1;
    e_pcpp_t err = pcppGetBlockingTime(stream->field_0x210, timeout, stream, iVar1, &timeout);
    e_pcpp_t err2 = _pcppGetErrorFromPcpt(pcptOpenClient(&stream->sock, ipAddr, port, timeout));
    stream->err = err2;
    if (err2 == e_pcpp_to) {
        if (err == e_pcpp_ok) {
            return e_pcpp_to;
        }
        stream->err = e_pcpp_no_server;
        amiDebugLog("error Time out error");
    }
    if (stream->err == 0) {
        amiTimerGet(&now);
        stream->last_active = now.microseconds / 1000 + now.seconds * 1000;
        if (timeout != TIMEOUT_NONE) {
            uVar3 = (now.microseconds - start.microseconds) / 1000 +
                    (now.seconds - start.seconds) * 1000;
            if (uVar3 < timeout) {
                timeout -= uVar3;
            } else {
                timeout = 0;
            }
        }
        stream->open = 1;
        stream->state = pcpp_state_wait_prompt;
        err = pcppRecvPrompt(stream, stream->field_0xb8, timeout);
        stream->err = err;
        if ((err == e_pcpp_to) || (stream->state = pcpp_state_none, err == e_pcpp_ok))
            goto LAB_00a16a90;
        pcptCloseDataSock(&stream->sock);
        pcppResetRead(stream);
        err = stream->err;
        amiDebugLog("error pcppRecvPrompt error = %d", err);
    } else {
        pcptCloseDataSock(&stream->sock);
        pcppResetRead(stream);
        stream->state = pcpp_state_none;
        err = stream->err;
        amiDebugLog("error pcptOpenClient error = %d", err);
    }
LAB_00a16a90:
    return stream->err;
}

e_pcpp_t pcppOpenBinaryServer(pcpp_t* stream, int open_mode, ushort port) {
    if (stream == NULL) {
        amiDebugLog("error PCPP stream isn't set");
        return e_pcpp_param_invalid;
    }

    amtime_t time;
    amiTimerGet(&time);
    stream->last_active = _amTimeMs(time);
    stream->state = pcpp_state_none;
    if (open_mode != 0 && open_mode != 1) {
        amiDebugLog("error Open Mode isn't set");
        stream->state = pcpp_state_none;
        return stream->err = e_pcpp_param_invalid;
    }

    e_pcpp_t err = _errT2P(pcptOpenServer(&stream->data_sock, open_mode, port));
    stream->err = err;
    if (err != e_pcpp_ok) {
        amiDebugLog("error pcptOpenBinaryServer");
        return err;
    }

    amiTimerGet(&time);
    stream->last_active = _amTimeMs(time);
    return e_pcpp_ok;
}

e_pcpp_t pcppOpenBinaryClient(pcpp_t* stream, char* ipAddr, ushort port, timeout_t timeout) {
    if (stream == NULL) {
        amiDebugLog("error PCPP stream isn't set");
        return e_pcpp_param_invalid;
    }
    if (ipAddr == NULL) {
        amiDebugLog("error IpAddr isn't set");
        return e_pcpp_param_invalid;
    }
    stream->state = pcpp_state_open_binary;

    amtime_t time;
    amiTimerGet(&time);
    stream->last_active = _amTimeMs(time);

    bool bVar1 = false;
    if (timeout == TIMEOUT_NONE ||
        (stream->field_0x210 <= timeout && timeout != stream->field_0x210)) {
        bVar1 = true;
    }

    e_pcpp_t pcpp_err =
        _pcppGetErrorFromPcpt(pcptOpenClient(&stream->data_sock, ipAddr, port, timeout));
    stream->err = pcpp_err;
    if (pcpp_err == e_pcpp_to && bVar1) {
        stream->err = e_pcpp_no_server;
        pcptCloseDataSock(&stream->sock);
        pcppResetRead(stream);
    }

    if (stream->err != e_pcpp_to) stream->state = pcpp_state_none;
    if (stream->err == e_pcpp_ok) {
        amiTimerGet(&time);
        stream->last_active = _amTimeMs(time);
    }
    return stream->err;
}

e_pcpp_t pcppOpenServer(pcpp_t* stream, int open_mode, u_short port, undefined4 param_4) {
    if (stream == NULL) {
        amiDebugLog("error PCPP stream isn't set");
        return e_pcpp_param_invalid;
    }
    stream->field_0xb8 = param_4;
    if (open_mode != 0 && open_mode != 1) {
        amiDebugLog("error Open Mode isn't set");

        stream->err = e_pcpp_param_invalid;
        stream->state = pcpp_state_none;
        return e_pcpp_param_invalid;
    }

    e_pcpp_t err = _errT2P(pcptOpenServer(&stream->sock, open_mode, port));
    stream->err = err;
    if (err != e_pcpp_ok) {
        amiDebugLog("error pcppOpenServer error = %d", err);
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
        amiDebugLog("error Send Data isn't set");
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

uint pcppRecvCheck(char* buf, int* offset) {
    if (buf == NULL || offset == NULL) {
        amiDebugLog("pointer error");
        return 1;
    }

    uint total_length = *offset;
    *offset = 0;

    uint local_offset = 0;

    /**
     * The following are the patterns this function validates against:
     *
     * * ? [equ ] -> 1            NG      eg ?=
     * * . [sep ] -> 3            ->      eg keychip.version&
     * * ? [sep ] -> 5            ->      eq ?&
     * * . [crlf] -> 6            OK      eg keychip.version\r\n
     * * ? [crlf] -> 8            OK      eg ?\r\n
     * * . [equ ] -> continue     \/      eg keychip.version=
     *
     * * . [equ ] . [crlf] -> 0   OK      eg keychip.version=1\r\n
     * * . [equ ] * [equ ] -> 1   NG      eg keychip.version=1=
     * * . [equ ] . [sep ] -> 2   ->      eg keychip.version=1&
     * * . [equ ] ? [sep ] -> 4   ->      eg keychip.version=?&
     * * . [equ ] ? [crlf] -> 7   OK      eg keychip.version=?\r\n
     */

    for (int is_value = 0; is_value < 2; is_value++) {
        uint nbytes = total_length - local_offset;
        int is_qmark = 0;
        pcpp_char_type_t char_type = pcppCheckStr(&buf[local_offset], &nbytes, &is_qmark);
        *offset += nbytes;

        switch (char_type) {
            case pcpp_char_error:
                return 1;

            case pcpp_char_equ:
                if (is_value) return 1;
                local_offset = nbytes;
                if (is_qmark) return 1;
                break;

            case pcpp_char_lf:
            case pcpp_char_crlf:
            case pcpp_char_cr:
                if (is_value) return is_qmark ? 7 : 0;
                return is_qmark ? 8 : 6;

            case pcpp_char_sep:
                if (is_value) return is_qmark ? 4 : 2;
                return is_qmark ? 5 : 3;
        }
    }
    return 1;
}

e_pcpp_t pcppSendResponseTable(pcpp_t* stream, pcp_send_data_t* data, timeout_t timeout_ms) {
    if (stream == NULL) {
        amiDebugLog("error PCPP stream isn't set");
        return e_pcpp_param_invalid;
    }
    if (data == NULL) {
        amiDebugLog("error Response buffer isn't set");
        return e_pcpp_param_invalid;
    }

    return pcppSendResponse(stream, data, data->length + 2, timeout_ms);
}

e_pcpp_t pcppSendResponse(pcpp_t* stream, pcp_send_data_t* resp_buffer, size_t buf_len,
                          timeout_t timeout_ms)

{
    e_pcpp_t err;
    amtime_t time;

    if (stream == NULL) {
        amiDebugLog("error PCPP stream isn't set");
        return e_pcpp_param_invalid;
    }
    if (resp_buffer == NULL) {
        amiDebugLog("error Response buffer isn't set");
        return e_pcpp_param_invalid;
    }

    if (buf_len > PCP_BUF_MAX - 1) {
        amiDebugLog("error Response buffer over");
        return stream->err = e_pcpp_param_invalid;
    }

    amiTimerGet(&time);
    stream->err = err = pcppCheckRecvMsg((char*)resp_buffer->data, buf_len, 0);

    if (err != e_pcpp_ok) {
        stream->state = pcpp_state_none;
        pcppResetRead(stream);
        amiDebugLog("error Send message format error");
        printf("'%s'\n", resp_buffer->data);
        return stream->err;
    }

    resp_buffer->length = buf_len;
    stream->resp_buffer = resp_buffer;
    stream->state = pcpp_state_send_response;
    err = pcpp_something(&time, timeout_ms, stream, &stream->sock, resp_buffer->data,
                         &resp_buffer->length, stream->field_0x204, stream->field_0x210, -6);
    stream->err = err;
    if (err != e_pcpp_to) {
        stream->state = pcpp_state_none;
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
}

e_pcpp_t pcppRecvRequest(pcpp_t* stream, pcp_parse_data_t* recv_data, timeout_t timeout) {
    e_pcpp_t err;
    uint timeout_ms = timeout;
    undefined4 uVar1;
    amtime_t now;
    amtime_t start;
    amtime_t now2;

    if (stream == NULL) {
        amiDebugLog("error PCPP stream isn't set");
        return e_pcpp_param_invalid;
    }
    if (recv_data == NULL) {
        amiDebugLog("error Request buffer isn't set");
        return stream->err = e_pcpp_param_invalid;
    }

    if ((stream->sock).open == PCPT_LISTENING) {
        uVar1 = stream->field_0x210;
    } else {
        uVar1 = stream->field_0xb8;
    }
    stream->recv_data_buffer = recv_data;
    amiTimerGet(&start);
    stream->state = pcpp_state_send_prompt;
    err = pcppSendPrompt(stream, uVar1, timeout_ms);
    stream->err = err;
    if (err != e_pcpp_to) {
        stream->state = pcpp_state_none;
    }
    if (err == e_pcpp_ok) {
        stream->open = 0;
        amiTimerGet(&now);
        stream->last_active = _amTimeMs(now);
        stream->state = pcpp_state_recv_request;
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
            stream->state = pcpp_state_none;
            if (err == e_pcpp_ok) {
                amiTimerGet(&now2);
                stream->last_active = _amTimeMs(now2);
            }
        }
        err = stream->err;
    }
    return err;
}

e_pcpp_t pcppRecvRequestMain(pcpp_t* stream, bool* bReRecv, undefined4 timeout) {
    char* recv_data;

    if (stream == NULL) {
        amiDebugLog("error PCPP stream isn't set");
        return e_pcpp_param_invalid;
    }

    stream->field_0x214 = 0;

    e_pcpp_t err = pcppRecvAllMsg(stream, timeout, bReRecv);
    if (err != e_pcpp_ok) return err;

    recv_data = stream->read_bytes_buf;
    if (pcppCheckRecvMsg(recv_data, stream->read_bytes_num, 1) != e_pcpp_ok) {
        pcppResetRead(stream);
        return e_pcpp_data_format_error;
    }

    uint local_4 = 0;
    pcppChangeRequest(stream, &local_4);

    if (local_4 < stream->read_bytes_num) {
        for (uint i = 0; i < stream->read_bytes_num; i++) {
            recv_data[local_4 + i] = recv_data[i];
        }
    }

    stream->read_bytes_num -= local_4;
    stream->read_bytes_size += local_4;
    stream->read_bytes_buf[stream->read_bytes_num] = '\0';
    return e_pcpp_ok;
}

e_pcpp_t pcppSendPrompt(pcpp_t* stream, uint param_2, timeout_t timeout_ms) {
    amtime_t now;

    bool has_timeout = false;
    if (stream == NULL) {
        amiDebugLog("error PCPP stream isn't set");
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

    e_pcpp_t err = _errT2P(pcptSend(&stream->sock, (unsigned char*)stream->send_buf, send_len,
                                    stream->field_0x204, timeout_ms));

    if (has_timeout && err == e_pcpp_to) {
        err = stream->open ? -12 : -6;
        pcptCloseDataSock(&stream->sock);
        pcppResetRead(stream);
        if (stream->open == 0) amiDebugLog("Error : Time out error");
    }
    amiTimerGet(&now);
    stream->last_active = _amTimeMs(now);
    return err;
}

e_pcpp_t pcppRecvAllMsg(pcpp_t* stream, uint param_1, bool* bReRecv) {
    uint* recv_buf_len;
    int iVar1;
    e_pcpp_t eVar2;
    undefined4 uVar3;
    timeout_t local_14;
    amtime_t local_10;
    amtime_t local_8;

    local_14 = param_1;
    if (stream == NULL) {
        amiDebugLog("error PCPP stream isn't set");
        return e_pcpp_param_invalid;
    }

    if (bReRecv == NULL) {
        amiDebugLog("error bReRecv isn't set");
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
    eVar2 = _errT2P(pcptRecv(&stream->sock,
                             (unsigned char*)(stream->read_bytes_buf + stream->read_bytes_num),
                             recv_buf_len, local_14));
    if (eVar2 == e_pcpp_to) {
        if (iVar1 == 0) return e_pcpp_to;

        eVar2 = e_pcpp_timeout_closed - (uint)(stream->field_0x214 != 0);
        pcptCloseDataSock(&stream->sock);
        *recv_buf_len = PCP_BUF_MAX;
        stream->read_bytes_num = 0;
        ZERO_BUF(stream->read_bytes_buf);
        stream->resp_buffer = NULL;
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
    if (stream == NULL || lenout == NULL) {
        amiDebugLog("pointer error");
        return NULL;
    }

    if (stream->recv_data_buffer == NULL) {
        amiDebugLog("don't set recvData buffer");
        return NULL;
    }

    if (stream->read_bytes_num > PCP_BUF_MAX) {
        amiDebugLog("Buffer size error");
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
    byte string_idx = 0;
    size_t i;
    for (i = 0; i < stream->read_bytes_num; i++) {
        char chr = stream->read_bytes_buf[i];

        if (chr == PCP_CHAR_CR || chr == PCP_CHAR_LF) {
            stream->recv_data_buffer->strings[string_idx] = PCP_CHAR_EOF;
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
            case PCP_CHAR_QMARK:
                stream->recv_data_buffer->strings[string_idx++] = PCP_CHAR_QMARK;
                break;
            case PCP_CHAR_SEP:
                if (stream->recv_data_buffer->cmd_count > PCP_CMDS_MAX) {
                    amiDebugLog("Buffer size error");
                    return NULL;
                }
                stream->recv_data_buffer->strings[string_idx] = PCP_CHAR_EOF;
                stream->recv_data_buffer->keywords[stream->recv_data_buffer->cmd_count] =
                    ++string_idx;
                stream->recv_data_buffer->cmd_count++;
                break;
            case PCP_CHAR_EQU:
                if (stream->recv_data_buffer->cmd_count > PCP_CMDS_MAX) {
                    amiDebugLog("Buffer size error");
                    return NULL;
                }
                stream->recv_data_buffer->strings[string_idx] = PCP_CHAR_EOF;
                stream->recv_data_buffer
                    ->keywords[stream->recv_data_buffer->cmd_count + PCP_CMDS_MAX - 1] =
                    ++string_idx;
                break;
            case PCP_CHAR_HASH:
                comment = true;
                break;
            default:
                if (pcppIsValidDataChar(chr))
                    stream->recv_data_buffer->strings[string_idx++] = stream->read_bytes_buf[i];
        }
    }
    *lenout = i + 1;
    return stream->recv_data_buffer;
}

int pcppRecvPrompt(pcpp_t* stream, undefined4 param_2, timeout_t param_3) {
    uint* recv_buf_len;
    int iVar1;

    timeout_t local_c = param_3;
    if (stream == NULL) {
        amiDebugLog("error PCPP stream isn't set");
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
        err = _errT2P(pcptRecv(&stream->sock,
                               (unsigned char*)(stream->read_bytes_buf + stream->read_bytes_num),
                               recv_buf_len, local_c));
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
        amiDebugLog("error PCPP stream isn't set");
        return e_pcpp_param_invalid;
    }

    if (stream->resp_buffer == NULL) {
        amiDebugLog("error Request buffer isn't set");
        return e_pcpp_param_invalid;
    }

    amtime_t now;
    amiTimerGet(&now);

    e_pcpp_t eVar1 =
        pcpp_something(&now, timeout_ms, stream, &stream->sock, stream->resp_buffer->data,
                       &stream->resp_buffer->length, stream->field_0x204, param_2,
                       stream->open ? e_pcpp_timeout_open : e_pcpp_timeout_closed);

    if (eVar1 != e_pcpp_to && eVar1 != e_pcpp_ok) {
        pcptCloseDataSock(&stream->sock);
        pcppResetRead(stream);
    }
    amiTimerGet(&now);
    stream->last_active = _amTimeMs(now);
    return eVar1;
}

// TODO: MINIMAL CLEANUP DONE HERE

e_pcpp_t pcppRecvBinary(pcpp_t* stream, unsigned char* recv_buf, size_t buf_len,
                        timeout_t timeout) {
    size_t* psVar1;
    int iVar2;
    e_pcpt_t eVar3;
    e_pcpp_t eVar4;
    undefined4 uVar5;
    bool bVar6;
    timeout_t timeout_local;
    amtime_t now;
    amtime_t local_10;
    amtime_t local_8;

    timeout_local = timeout;
    if (stream == NULL) {
        amiDebugLog("error PCPP stream isn't set");
        return e_pcpp_param_invalid;
    }
    if (recv_buf == NULL) {
        amiDebugLog("error Recv Bninary Buffer isn't set");
        return e_pcpp_param_invalid;
    }
    stream->field_0x214 = 0;
    amiTimerGet(&now);
    psVar1 = &stream->field_0x1f8;
    stream->recv_binary_buf = recv_buf;
    *psVar1 = buf_len;
    stream->recv_binary_buf_len = buf_len;
    stream->field_0x1fc = 0;
    stream->state = pcpp_state_recv_binary;
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
        iVar2 = pcppGetBlockingTime(uVar5, timeout, stream, _amTimeMs(now), &timeout_local);
        stream->field_0x214 = 1;
        eVar3 = pcptRecv(&stream->data_sock, stream->recv_binary_buf + stream->field_0x1fc, psVar1,
                         timeout_local);
        eVar4 = _errT2P(eVar3);
        stream->err = eVar4;
        if ((eVar4 == e_pcpp_to) && (iVar2 != 0)) {
            stream->err = bVar6 ? -13 : e_pcpp_no_server;
            pcptCloseDataSock(&stream->sock);
            stream->read_bytes_size = PCP_BUF_MAX;
            stream->read_bytes_num = 0;
            memset(stream->read_bytes_buf, 0, PCP_BUF_MAX);
            stream->resp_buffer = NULL;
        }
        eVar4 = stream->err;
        if (eVar4 == e_pcpp_to) {
        LAB_00455e91:
            if (eVar4 != e_pcpp_ok) {
                return eVar4;
            }
        } else {
            if (eVar4 != e_pcpp_ok) {
                stream->state = pcpp_state_none;
                goto LAB_00455e91;
            }
        }
        stream->field_0x1fc = stream->field_0x1fc + *psVar1;
        *psVar1 = stream->recv_binary_buf_len - stream->field_0x1fc;
        stream->field_0x214 = 0;
        amiTimerGet(&local_10);
        stream->last_active = _amTimeMs(local_10);
        if (stream->recv_binary_buf_len < stream->field_0x1fc ||
            stream->recv_binary_buf_len == stream->field_0x1fc) {
            unsigned char* send_buf = (unsigned char*)stream->send_buf;
            psVar1 = &stream->send_buf_len;
            stream->state = pcpp_state_send_binary_ack;
            send_buf[0] = PCP_CHAR_BINACK;
            *psVar1 = 1;
            iVar2 = pcppGetBlockingTime(stream->field_0x20c, timeout, stream, _amTimeMs(now),
                                        &timeout_local);
            eVar3 = pcptSend(&stream->sock, send_buf, psVar1, stream->field_0x208, timeout_local);
            eVar4 = _errT2P(eVar3);
            stream->err = eVar4;
            if ((eVar4 == e_pcpp_to) && (iVar2 != 0)) {
                stream->err = -14;
                pcptCloseDataSock(&stream->sock);
                pcppResetRead(stream);
            }
            eVar4 = stream->err;
            if (eVar4 != e_pcpp_to) {
                stream->state = pcpp_state_none;
            }
            if (eVar4 == e_pcpp_ok) {
                amiTimerGet(&local_10);
                stream->last_active = _amTimeMs(local_10);
                stream->state = pcpp_state_send_binary_ack_wait;
                iVar2 = pcppGetBlockingTime(stream->field_0x208, timeout, stream, _amTimeMs(now),
                                            &timeout_local);
                *psVar1 = PCP_SEND_BUF_MAX;
                ZERO(stream->send_buf);
                eVar3 = pcptRecv(&stream->data_sock, send_buf, psVar1, timeout_local);
                eVar4 = _errT2P(eVar3);
                stream->err = eVar4;
                if ((iVar2 != 0) && (eVar4 == e_pcpp_to)) {
                    stream->err = -13;
                }
                if (stream->err != e_pcpp_to) {
                    stream->state = pcpp_state_none;
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

e_pcpp_t pcppSendBinary(pcpp_t* stream, const unsigned char* send_binary_buffer, size_t param_3,
                        uint param_4)

{
    size_t* psVar1;
    uint pVar2;
    e_pcpp_t eVar3;
    int iVar4;
    e_pcpt_t eVar5;
    uint uVar6;
    timeout_t local_14;
    amtime_t local_10;
    amtime_t local_8;

    local_14 = param_4;
    if (stream == NULL) {
        amiDebugLog("error PCPP stream isn't set");
        return e_pcpp_param_invalid;
    }
    if (send_binary_buffer == NULL) {
        amiDebugLog("error Send Bninary Buffer isn't set");
        return stream->err = e_pcpp_param_invalid;
    }
    amiTimerGet(&local_8);
    pVar2 = (stream->data_sock).open;
    psVar1 = &stream->field_0x1f0;
    stream->state = pcpp_state_send_binary;
    *psVar1 = param_3;
    stream->send_binary_buf = send_binary_buffer;
    if (pVar2 == PCPT_LISTENING) {
        amiTimerGet(&local_10);
        stream->last_active = _amTimeMs(local_10);
        eVar3 =
            pcpp_something(&local_8, param_4, stream, &stream->data_sock, stream->send_binary_buf,
                           psVar1, stream->field_0x208, stream->field_0x210, -11);
        stream->err = eVar3;
    } else {
        eVar5 =
            pcptSend(&stream->data_sock, send_binary_buffer, psVar1, stream->field_0x208, param_4);
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
        stream->state = pcpp_state_none;
    }
    if (eVar3 == e_pcpp_ok) {
        amiTimerGet(&local_10);
        stream->last_active = _amTimeMs(local_10);
        stream->state = pcpp_state_send_binary_wait;
        psVar1 = &stream->send_buf_len;
        *psVar1 = 8;
        ZERO(stream->send_buf);
        stream->field_0x1e8 = 0;
        iVar4 = pcppGetBlockingTime(stream->field_0x20c, param_4, stream, _amTimeMs(local_8),
                                    &local_14);
        eVar5 = pcptRecv(&stream->sock, (unsigned char*)stream->send_buf, psVar1, local_14);
        eVar3 = _errT2P(eVar5);
        stream->err = eVar3;
        if ((iVar4 != 0) && (eVar3 == e_pcpp_to)) {
            stream->err = -14;
            pcptCloseDataSock(&stream->sock);
            pcppResetRead(stream);
        }
        eVar3 = stream->err;
        if (eVar3 != e_pcpp_to) {
            stream->state = pcpp_state_none;
        }
        if (eVar3 == e_pcpp_ok) {
            stream->field_0x1e8 = stream->field_0x1e8 + *psVar1;
            uVar6 = 0;
            *psVar1 = 8 - stream->field_0x1e8;
            stream->state = pcpp_state_none;

            for (uint i = 0; i < stream->field_0x1e8; i++) {
                if (stream->send_buf[i] == PCP_CHAR_BINACK) {
                    return (stream->err = e_pcpp_ok);
                }
            }
            return (stream->err = e_pcpp_data_format_error);
        }
    }
    return eVar3;
}

e_pcpp_t pcppIsBusy(pcpp_t* stream, timeout_t timeout) {
    size_t* psVar1;
    char** send_buf;
    undefined4 uVar2;
    int iVar3;
    e_pcpt_t eVar4;
    int iVar5;
    timeout_t uVar6;
    e_pcpp_t eVar7;
    char* pcVar8;
    timeout_t uVar10;
    timeout_t uVar11;
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
        amiDebugLog("error PCPP stream isn't set");
        return e_pcpp_param_invalid;
    }
    amiTimerGet(&local_18);

    switch (stream->state) {
        case pcpp_state_none:
            stream->err = e_pcpp_no_client;
            break;
        case pcpp_state_open:
        case pcpp_state_open_binary:
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
                            stream->read_bytes_num =
                                stream->read_bytes_num + stream->read_bytes_size;
                            stream->read_bytes_size = PCP_BUF_MAX - stream->read_bytes_num;
                            iVar3 = pcppCheckPrompt(stream);
                            if (iVar3 != 0) {
                                stream->err = e_pcpp_ok;
                                return e_pcpp_ok;
                            }
                            stream->err = ~e_pcpp_no_client;
                            goto LAB_00457660;
                        }
                        stream->state = pcpp_state_wait_prompt;
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
                            stream->state = pcpp_state_none;
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
                stream->err = e_pcpp_no_server;
            }
            goto LAB_00457392;
        case pcpp_state_send_prompt:
        case pcpp_state_send_request:
        case pcpp_state_send_response:
            uVar6 = uVar11;
            if ((stream->sock).field_0x54 != 0) {
                bVar12 = (stream->sock).open != PCPT_LISTENING;
                if (bVar12) {
                    uVar2 = stream->field_0xb8;
                } else {
                    uVar2 = stream->field_0x210;
                }
                bVar12 = !bVar12;
                local_20 =
                    pcppGetBlockingTime(uVar2, uVar11, stream, _amTimeMs(local_18), &timeout);
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
                    stream->state = pcpp_state_recv_request;
                    iVar3 = pcppGetBlockingTime(stream->field_0xb8, uVar11, stream,
                                                _amTimeMs(local_18), &timeout);
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
                        stream->state = pcpp_state_none;
                    }
                    if (stream->err == e_pcpp_ok) goto LAB_004574b8;
                    break;
                }
                goto LAB_004577af;
            }
            if (local_20 == 0) break;
            stream->err = bVar12 ? e_pcpp_no_server : e_pcpp_timeout_closed;
            goto LAB_00457392;
        case pcpp_state_wait_prompt:
        case pcpp_state_recv_request:
        case pcpp_state_recv_response:
            if (stream->field_0x214 == 0) {
                uVar2 = stream->field_0xb8;
            } else {
                uVar2 = stream->field_0x204;
            }
            iVar3 = pcppGetBlockingTime(uVar2, uVar11, stream, _amTimeMs(local_18), &timeout);
            uVar6 = timeout;
            eVar7 = _errT2P(pcptIsBusy(&stream->sock, timeout));
            stream->err = eVar7;
            if (eVar7 != e_pcpp_to) {
                if (eVar7 != e_pcpp_ok) {
                LAB_00457660:
                    stream->state = pcpp_state_none;
                    return stream->err;
                }
                stream->read_bytes_num = stream->read_bytes_num + stream->read_bytes_size;
                stream->field_0x214 = 0;
                stream->read_bytes_size = PCP_BUF_MAX - stream->read_bytes_num;
                if (stream->state == pcpp_state_wait_prompt) {
                    uVar6 = uVar11;
                    if (uVar11 != TIMEOUT_NONE) {
                        amiTimerGet(&local_8);
                        uVar6 = amiTimerDiffMsec(&local_18, &local_8) < uVar11 ? uVar11 : 0;
                    }
                    eVar7 = pcppRecvPrompt(stream, stream->field_0xb8, uVar6);
                    stream->err = eVar7;
                    if (eVar7 != e_pcpp_to) {
                        if (eVar7 != e_pcpp_ok) goto LAB_00457660;
                        if (stream->open == 0) {
                            if (uVar11 != TIMEOUT_NONE) {
                                amiTimerGet(&local_8);
                                uVar11 =
                                    amiTimerDiffMsec(&local_18, &local_8) < uVar11 ? uVar11 : 0;
                            }
                            stream->state = pcpp_state_send_request;
                            eVar7 = pcppSendRequestMain(stream, stream->field_0xb8, uVar11);
                            stream->err = eVar7;
                            if ((eVar7 != e_pcpp_to) && (eVar7 != e_pcpp_ok)) {
                                stream->state = pcpp_state_none;
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
                    eVar7 = pcppRecvAllMsg(stream, uVar6, &local_24);
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
                eVar7 =
                    pcppCheckRecvMsg(pcVar8, stream->read_bytes_num, (uint)(stream->state == 5));
                stream->err = eVar7;
                if (eVar7 == e_pcpp_ok) {
                    uint lenout;
                    pcppChangeRequest(stream, &lenout);
                    if (lenout < stream->read_bytes_num) {
                        for (uint i = 0; i < stream->read_bytes_num - lenout; i++) {
                            pcVar8[i] = pcVar8[i + lenout];
                        }
                    }
                    stream->read_bytes_num = stream->read_bytes_num - lenout;
                    stream->read_bytes_size = stream->read_bytes_size + lenout;
                    stream->state = pcpp_state_none;
                    amiTimerGet(&local_18);
                    stream->last_active = _amTimeMs(local_18);
                    return stream->err;
                }
                stream->state = pcpp_state_none;
                pcppResetRead(stream);
                amiDebugLog("error Response format error");
                return stream->err;
            }
            if (iVar3 == 0) break;
            if (stream->field_0x214 == 0) {
                stream->err = (stream->open != 0) ? e_pcpp_timeout_open : e_pcpp_timeout_closed;
            } else {
                stream->err = -7;
            }
        LAB_00457392:
            pcptCloseDataSock(&stream->sock);
            pcppResetRead(stream);
        LAB_0045739d:
            stream->state = pcpp_state_none;
            return stream->err;
        case pcpp_state_send_binary:
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
                stream->state = pcpp_state_none;
            }
            if (stream->err == e_pcpp_ok) {
                amiTimerGet(&local_10);
                stream->last_active = _amTimeMs(local_10);
                stream->state = pcpp_state_send_binary_wait;
                psVar1 = &stream->send_buf_len;
                *psVar1 = PCP_SEND_BUF_MAX;
                ZERO(stream->send_buf);
                stream->field_0x1e8 = 0;
                iVar3 = pcppGetBlockingTime(stream->field_0x20c, uVar11, stream,
                                            _amTimeMs(local_18), &timeout);
                eVar4 = pcptRecv(&stream->sock, (unsigned char*)stream->send_buf, psVar1, timeout);
                eVar7 = _errT2P(eVar4);
                stream->err = eVar7;
                if ((iVar3 != 0) && (eVar7 == e_pcpp_to)) {
                    stream->err = -14;
                    pcptCloseDataSock(&stream->sock);
                    pcppResetRead(stream);
                }
                if (stream->err != e_pcpp_to) {
                    stream->state = pcpp_state_none;
                }
                if (stream->err == e_pcpp_ok) {
                    stream->field_0x1e8 = stream->field_0x1e8 + *psVar1;
                    *psVar1 = PCP_SEND_BUF_MAX - stream->field_0x1e8;
                    stream->state = pcpp_state_none;
                    stream->err = e_pcpp_data_format_error;
                    if (stream->field_0x1e8 != 0) {
                        for (uint i = 0; i < stream->field_0x1e8; i++) {
                            if (stream->send_buf[i] == PCP_CHAR_BINACK) {
                                stream->err = e_pcpp_ok;
                                break;
                            }
                        }
                    }
                    amiTimerGet(&local_10);
                    stream->last_active = _amTimeMs(local_10);
                    return stream->err;
                }
            }
            break;
        case pcpp_state_send_binary_wait:
            iVar3 = pcppGetBlockingTime(stream->field_0x20c, uVar11, stream, _amTimeMs(local_18),
                                        &timeout);
            eVar4 = pcptIsBusy(&stream->sock, timeout);
            eVar7 = _errT2P(eVar4);
            stream->err = eVar7;
            if ((iVar3 != 0) && (eVar7 == e_pcpp_to)) {
                stream->err = -14;
                pcptCloseDataSock(&stream->sock);
                pcppResetRead(stream);
            }
            if (stream->err != e_pcpp_to) {
                stream->state = pcpp_state_none;
            }
            if (stream->err == e_pcpp_ok) {
                amiTimerGet(&local_10);
                stream->field_0x1e8 = stream->field_0x1e8 + stream->send_buf_len;
                stream->last_active = _amTimeMs(local_10);
                ;
                uVar11 = 0;
                stream->send_buf_len = PCP_SEND_BUF_MAX - stream->field_0x1e8;
                for (uint i = 0; i < stream->field_0x1e8; i++) {
                    if (stream->send_buf[i] == PCP_CHAR_BINACK) {
                        return (stream->err = e_pcpp_ok);
                    }
                }
                return (stream->err = e_pcpp_data_format_error);
            }
            return stream->err;
        case pcpp_state_recv_binary:
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
                        stream->state = pcpp_state_none;
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
                    iVar3 = pcppGetBlockingTime(stream->field_0x208, uVar11, stream,
                                                _amTimeMs(local_18), &timeout);
                    stream->field_0x214 = 1;
                    eVar4 =
                        pcptRecv(&stream->data_sock, stream->recv_binary_buf + stream->field_0x1fc,
                                 psVar1, timeout);
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
                            stream->state = pcpp_state_none;
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
            send_buf = (char**)&stream->send_buf;
            psVar1 = &stream->send_buf_len;
            stream->state = pcpp_state_send_binary_ack;
            *send_buf[0] = PCP_CHAR_BINACK;
            *psVar1 = 1;
            local_20 = pcppGetBlockingTime(stream->field_0x20c, uVar11, stream, _amTimeMs(local_18),
                                           &timeout);
            eVar4 = pcptSend(&stream->sock, (unsigned char*)*send_buf, psVar1, stream->field_0x204,
                             timeout);
            eVar7 = _errT2P(eVar4);
            stream->err = eVar7;
            if ((eVar7 == e_pcpp_to) && (local_20 != 0)) {
                stream->err = -14;
                pcptCloseDataSock(&stream->sock);
                pcppResetRead(stream);
            }
            if (stream->err != e_pcpp_to) {
                stream->state = pcpp_state_none;
            }
            if (stream->err == e_pcpp_ok) {
                amiTimerGet(&local_10);
                stream->last_active = _amTimeMs(local_10);
                stream->state = pcpp_state_send_binary_ack_wait;
                iVar3 = pcppGetBlockingTime(stream->field_0x208, uVar11, stream,
                                            _amTimeMs(local_18), &timeout);
                *psVar1 = PCP_SEND_BUF_MAX;
                ZERO(stream->send_buf);
                eVar4 = pcptRecv(&stream->data_sock, (unsigned char*)*send_buf, psVar1, timeout);
                eVar7 = _errT2P(eVar4);
                stream->err = eVar7;
                if ((iVar3 != 0) && (eVar7 == e_pcpp_to)) {
                    stream->err = -13;
                }
                if (stream->err != e_pcpp_to) {
                    stream->state = pcpp_state_none;
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
        case pcpp_state_send_binary_ack:
            iVar3 = pcppGetBlockingTime(stream->field_0x20c, uVar11, stream, _amTimeMs(local_18),
                                        &timeout);
            eVar4 = pcptIsBusy(&stream->sock, timeout);
            eVar7 = _errT2P(eVar4);
            stream->err = eVar7;
            if ((iVar3 != 0) && (eVar7 == e_pcpp_to)) {
                stream->err = -14;
                pcptCloseDataSock(&stream->sock);
                pcppResetRead(stream);
            }
            if (stream->err != e_pcpp_to) {
                stream->state = pcpp_state_none;
            }
            if (stream->err != e_pcpp_ok) break;
            stream->state = pcpp_state_send_binary_ack_wait;
            amiTimerGet(&local_10);
            stream->last_active = _amTimeMs(local_10);
            iVar3 = pcppGetBlockingTime(stream->field_0x208, uVar11, stream, _amTimeMs(local_18),
                                        &timeout);
            stream->send_buf_len = PCP_SEND_BUF_MAX;
            ZERO(stream->send_buf);
            eVar4 = pcptRecv(&stream->data_sock, (unsigned char*)stream->send_buf,
                             &stream->send_buf_len, timeout);
            eVar7 = _errT2P(eVar4);
            stream->err = eVar7;
            if ((iVar3 != 0) && (eVar7 == e_pcpp_to)) {
                stream->err = -13;
                pcptCloseDataSock(&stream->sock);
                pcppResetRead(stream);
            }
            if (stream->err != e_pcpp_to) stream->state = pcpp_state_none;

            if (stream->err != e_pcpp_closed) break;
            stream->err = e_pcpp_ok;
            goto LAB_004574b8;
        case pcpp_state_send_binary_ack_wait:
            iVar3 = pcppGetBlockingTime(stream->field_0x208, uVar11, stream, _amTimeMs(local_18),
                                        &timeout);
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
            stream->state = pcpp_state_none;
            return stream->err;
        case 14:
        case pcpp_state_wait_response:
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
            eVar7 = pcppRecvAllMsg(stream, uVar11, &local_24);
            stream->err = eVar7;
            if (eVar7 == e_pcpp_ok) {
                pcVar8 = stream->read_bytes_buf;
                eVar7 =
                    pcppCheckRecvMsg(pcVar8, stream->read_bytes_num, (uint)(stream->state == 14));
                stream->err = eVar7;
                if (eVar7 != e_pcpp_ok) {
                    stream->state = pcpp_state_none;
                    pcppResetRead(stream);
                    amiDebugLog("error Response format error");
                    return stream->err;
                }
                uint lenout;
                pcppChangeRequest(stream, &lenout);
                if (lenout < stream->read_bytes_num) {
                    for (uint i = 0; i < stream->read_bytes_num - lenout; i++) {
                        pcVar8[i] = pcVar8[i + lenout];
                    }
                }

                stream->read_bytes_num = stream->read_bytes_num - lenout;
                stream->read_bytes_size = stream->read_bytes_size + lenout;
                stream->state = pcpp_state_none;
                amiTimerGet(&local_18);
                stream->last_active = _amTimeMs(local_18);
                return stream->err;
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

int pcpp_something(amtime_t* time, timeout_t timeout, pcpp_t* stream, pcpt_t* sock,
                   const unsigned char* send_buf, size_t* send_len, undefined4 param_7,
                   undefined4 param_8, int fallback_err) {
    timeout_t iVar1;
    timeout_t blocking_time;
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
        amiDebugLog("error Send Data isn't set");
        return NULL;
    }

    size_t existing_length = send_data->length;
    if (existing_length == 0) return pcppSetSendPacket(send_data, keyword, value);

    size_t kwLen = strlen(keyword);
    if (value == NULL) {
        if (existing_length + kwLen + 3 < PCP_BUF_MAX) {
            sprintf_s((char*)&send_data->data[existing_length], PCP_BUF_MAX - existing_length,
                      "&%s\r\n", keyword);
            send_data->length += kwLen + 1;
            return send_data;
        }
    } else {
        size_t valLen = strlen(value);
        if (existing_length + kwLen + valLen + 4 < PCP_BUF_MAX) {
            sprintf_s((char*)&send_data->data[existing_length], PCP_BUF_MAX - existing_length,
                      "&%s=%s\r\n", keyword, value);
            send_data->length += valLen + kwLen + 2;
            return send_data;
        }
    }
    return NULL;
}

e_pcpp_t pcppSendRequestTable(pcpp_t* stream, pcp_send_data_t* request_buffer, timeout_t timeout) {
    if (stream == NULL) {
        amiDebugLog("error PCPP stream isn't set");
        return e_pcpp_param_invalid;
    }

    if (request_buffer == NULL) {
        amiDebugLog("error Request buffer isn't set");
        return e_pcpp_param_invalid;
    }

    return pcppSendRequest(stream, request_buffer, request_buffer->length + 2, timeout);
}

e_pcpp_t pcppSendRequest(pcpp_t* stream, pcp_send_data_t* request_buffer, uint nbytes,
                         timeout_t timeout) {
    amtime_t local_18;
    amtime_t local_10;
    amtime_t local_8;

    if (stream == NULL) {
        amiDebugLog("error PCPP stream isn't set");
        return e_pcpp_param_invalid;
    }
    if (request_buffer == NULL) {
        amiDebugLog("error Request buffer isn't set");
        return e_pcpp_param_invalid;
    }

    if (0x100 < nbytes) {
        amiDebugLog("error Request buffer over");
        return (stream->err = e_pcpp_param_invalid);
    }

    amiTimerGet(&local_10);
    e_pcpp_t pcpp_err = pcppCheckRecvMsg((char*)&request_buffer->data, nbytes, 1);
    stream->err = pcpp_err;
    if (pcpp_err != e_pcpp_ok) {
        stream->state = pcpp_state_none;
        pcppResetRead(stream);
        amiDebugLog("error Message format error");
        return stream->err;
    }

    request_buffer->length = nbytes;
    stream->resp_buffer = request_buffer;

    *(uint*)stream->send_buf = nbytes;
    if (stream->open == 0) {
        stream->state = pcpp_state_wait_prompt;
        pcpp_err = pcppRecvPrompt(stream, stream->field_0xb8, timeout);
        stream->err = pcpp_err;
        if (pcpp_err != e_pcpp_to) {
            stream->state = pcpp_state_none;
        }
        if (pcpp_err != e_pcpp_ok) {
            return pcpp_err;
        }
    }

    amiTimerGet(&local_18);
    stream->state = pcpp_state_send_request;

    pcpp_err = pcppSendRequestMain(stream, stream->field_0xb8, timeout);
    stream->err = pcpp_err;
    if (pcpp_err != e_pcpp_to) {
        stream->state = pcpp_state_none;
        amiTimerGet(&local_8);
        stream->last_active = local_8.microseconds / 1000 + local_8.seconds * 1000;
        stream->open = 0;
    }
    return stream->err;
}

e_pcpp_t pcppRecvResponse(pcpp_t* stream, pcp_parse_data_t* buffer, timeout_t timeout) {
    int iVar1;
    amtime_t local_8;

    if (stream == NULL) {
        amiDebugLog("error PCPP stream isn't set");
        return -5;
    }

    if (buffer == NULL) {
        amiDebugLog("error Response buffer isn't set");
        return -5;
    }

    stream->recv_data_buffer = buffer;
    stream->state = pcpp_state_recv_response;
    stream->field_0x214 = 0;
    bool bReRecv;
    iVar1 = pcppRecvAllMsg(stream, timeout, &bReRecv);
    stream->err = iVar1;
    if (iVar1 != e_pcpp_ok) {
        if (iVar1 != e_pcpp_to) {
            stream->state = pcpp_state_none;
            return iVar1;
        }
        if (stream != NULL) stream->state = pcpp_state_wait_response;
        return e_pcpp_to;
    }

    char* pcVar5 = stream->read_bytes_buf;
    iVar1 = pcppCheckRecvMsg(pcVar5, stream->read_bytes_num, 0);
    stream->err = iVar1;
    if (iVar1 == 0) {
        uint lenout;
        pcppChangeRequest(stream, &lenout);
        if (lenout < stream->read_bytes_num) {
            for (uint i = 0; i < stream->read_bytes_num - lenout; i++) {
                pcVar5[i] = pcVar5[i + lenout];
            }
        }

        stream->read_bytes_num = stream->read_bytes_num - (int)lenout;
        stream->read_bytes_size = (stream->read_bytes_size - 0x46) + lenout;
        stream->state = pcpp_state_none;
        stream->err = 0;
        amiTimerGet(&local_8);
        stream->last_active = local_8.microseconds / 1000 + local_8.seconds * 1000;
    } else {
        stream->state = pcpp_state_none;
        pcppResetRead(stream);
        amiDebugLog("error Response format error");
        return stream->err;
    }

    return stream->err;
}

SOCKADDR_IN* pcppGetSockAddr(pcpp_t* stream, bool is_data) {
    if (stream == NULL) {
        amiDebugLog("error PCPP stream isn't set");
        return NULL;
    }

    if (is_data) {
        return &(stream->data_sock).client_addr;
    }
    return &(stream->sock).client_addr;
}