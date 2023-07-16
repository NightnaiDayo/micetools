#include "pcpa.h"

e_pcpa_t _pcpaGetErrorFromPcpp(e_pcpp_t err) {
    switch (err) {
        case e_pcpp_ok:
            return e_pcpa_ok;
        case e_pcpp_to:
            return e_pcpa_to;
        case e_pcpp_closed:
            return e_pcpa_closed;
        case e_pcpp_no_client:
            return e_pcpa_no_client;
        case e_pcpp_wsa_noinit:
            return e_pcpa_wsa_noinit;
        case e_pcpp_inval_addr:
            return e_pcpa_inval_addr;
        case -14:
            return (e_pcpa_t)-16;
        case -13:
            return (e_pcpa_t)-15;
        case e_pcpp_timeout_open:
            return e_pcpa_timeout_open;
        case e_pcpp_no_server:
            return e_pcpa_no_server;
        case e_pcpp_recv_unset:
            return e_pcpa_cb_table_full;
        case -9:
            return (e_pcpa_t)-7;
        case -8:
            return (e_pcpa_t)-6;
        case -7:
            return (e_pcpa_t)-3;
        case e_pcpp_timeout_closed:
            return e_pcpa_timeout_closed;
        case e_pcpp_param_invalid:
            return e_pcpa_param_invalid;
        case -4:
            return (e_pcpa_t)-8;
        case e_pcpp_already_open:
            return e_pcpa_already_open;
        case e_pcpp_cannot_open:
            return e_pcpa_cannot_open;
        case e_pcpp_not_open:
            return e_pcpa_not_open;
        default:
            return e_pcpa_closed;
    }
}

void pcpaCloseBinary(pcpa_t *stream) {
    if (stream == NULL) {
        amiDebugLog("error PCPA stream isn't set");
        return;
    }
    stream->binary_mode = binary_mode_none;

    pcppCloseBinary(&stream->pcpp);
}

void pcpaClose(pcpa_t *stream) {
    if (stream == NULL) {
        amiDebugLog("error PCPA stream isn't set");
        return;
    }
    pcppClose(&stream->pcpp);
    stream->err = e_pcpa_unknown;
    stream->state = pcpa_state_none;
    stream->callback_table = NULL;
    stream->callback_max = 0;
    stream->callback_count = 0;
    stream->recv_buffer = NULL;
    stream->recv_buffer_len = 0;
    stream->send_buffer = NULL;
    stream->send_buffer_len = 0;
    stream->binary_mode = binary_mode_none;
    stream->binary_mode_before_cb = NULL;
    stream->binary_mode_after_cb = NULL;
    stream->binary_mode_before_data = NULL;
    stream->binary_mode_after_data = NULL;
    memset(&stream->send_data, 0, sizeof stream->send_data);
    (stream->send_data).length = 0;
}

e_pcpa_t pcpaInitStream(pcpa_t *stream) {
    if (LIBPCP_VERSION == NULL) {
        amiDebugLog("error PCPA version isn't set");
        return e_pcpa_generic;
    }
    if (stream == NULL) {
        amiDebugLog("error PCPA stream isn't set");
        return e_pcpa_stream_unset;
    }

    e_pcpp_t err = pcppInitStream(&stream->pcpp);
    if (err != e_pcpp_ok) {
        return err == e_pcpp_wsa_noinit ? e_pcpa_wsa_noinit : e_pcpa_param_invalid;
    }

    stream->err = e_pcpa_unknown;
    stream->state = pcpa_state_none;
    stream->callback_table = NULL;
    stream->callback_max = 0;
    stream->callback_count = 0;
    stream->recv_buffer = NULL;
    stream->recv_buffer_len = 0;
    stream->send_buffer = NULL;
    stream->send_buffer_len = 0;
    stream->binary_mode = binary_mode_none;
    stream->binary_mode_before_cb = NULL;
    stream->binary_mode_after_cb = NULL;
    stream->binary_mode_before_data = NULL;
    stream->binary_mode_after_data = NULL;
    stream->before_cb = NULL;
    ZERO(stream->recv_data);
    ZERO(stream->send_data);
    return e_pcpa_ok;
}

e_pcpa_t pcpaOpenClient(pcpa_t *stream, char *ipAddr, ushort port, uint param_4,
                        timeout_t timeout) {
    if (stream == NULL) {
        amiDebugLog("error PCPA stream isn't set");
        return e_pcpa_stream_unset;
    }
    if (ipAddr == 0) {
        amiDebugLog("error PCPA IpAddr isn't set");
        return e_pcpa_param_invalid;
    }

    stream->state = pcpa_state_wait_prompt;
    e_pcpa_t err =
        _pcpaGetErrorFromPcpp(pcppOpenClient(&stream->pcpp, ipAddr, port, param_4, timeout));
    stream->err = err;
    if (err != e_pcpa_to) stream->state = pcpa_state_none;

    return err;
}

e_pcpa_t pcpaOpenServerWithBinary(pcpa_t *stream, int open_mode, u_short port, u_short binary_port,
                                  timeout_t param_5) {
    e_pcpa_t err;

    if (stream == NULL) {
        amiDebugLog("error PCPA stream isn't set");
        return e_pcpa_stream_unset;
    }

    switch (open_mode) {
        case 0:
            stream->err = err = _errP2A(pcppOpenServer(&stream->pcpp, 0, port, param_5));
            if (err != e_pcpa_ok) {
                amiDebugLog("error pcppOpenServer");
                return err;
            }
            stream->err = err = _errP2A(pcppOpenBinaryServer(&stream->pcpp, 0, binary_port));
            if (err != e_pcpa_ok) {
                amiDebugLog("error pcppOpenBinaryServer");
                return err;
            }
            break;
        case 1:
            stream->err = err = _errP2A(pcppOpenServer(&stream->pcpp, 1, port, param_5));
            if (err != e_pcpa_ok) {
                amiDebugLog("error pcppOpenServer");
                return err;
            }
            stream->err = err = _errP2A(pcppOpenBinaryServer(&stream->pcpp, 1, binary_port));
            if (err != e_pcpa_ok) {
                amiDebugLog("error pcppOpenBinaryServer");
                return err;
            }
            break;
        default:
            amiDebugLog("error Open Mode isn't set");
            return stream->err = e_pcpa_generic;
    }

    stream->state = pcpa_state_none;
    return e_pcpa_ok;
}

e_pcpa_t pcpaSetCallbackFunc(pcpa_t *stream, char *keyword, pcpa_callback *callback, void *data) {
    if (stream == NULL) {
        amiDebugLog("error PCPA stream isn't set");
        return e_pcpa_stream_unset;
    }

    if (keyword == NULL) {
        amiDebugLog("error keyword isn't set");
        return e_pcpa_generic;
    }

    if (callback == NULL) {
        amiDebugLog("error Callback func isn't set");
        return e_pcpa_generic;
    }

    if (stream->callback_table == NULL) {
        amiDebugLog("error Callback_table buffer isn't set");
        return e_pcpa_cb_table_unset;
    }

    if (strnlen(keyword, PCP_KEYWORD_MAX + 1) > PCP_KEYWORD_MAX) {
        amiDebugLog("error a keyword is too long");
        return e_pcpa_generic;
    }

    if (stream->callback_count >= stream->callback_max) return e_pcpa_cb_table_full;

    sprintf_s(stream->callback_table[stream->callback_count].keyword, PCP_KEYWORD_MAX, "%s",
              keyword);
    stream->callback_table[stream->callback_count].callback = callback;
    stream->callback_table[stream->callback_count].data = data;
    stream->callback_count++;

    return stream->err = e_pcpa_ok;
}

e_pcpa_t pcpaSetCallbackFuncBuffer(pcpa_t *stream, pcpa_cb_table_t *callback_table,
                                   uint callbacks_max) {
    if (stream == NULL) {
        amiDebugLog("error PCPA stream isn't set");
        return e_pcpa_stream_unset;
    }

    if (callback_table == NULL || callbacks_max == 0) return e_pcpa_no_table_space;

    stream->callback_max = callbacks_max;
    stream->callback_table = callback_table;
    stream->callback_count = 0;
    stream->err = e_pcpa_ok;
    return e_pcpa_ok;
}

pcp_send_data_t *pcpaSetSendPacket(pcpa_t *stream, char *keyword, char *value) {
    if (stream == NULL) {
        amiDebugLog("error PCPA stream isn't set");
        return NULL;
    }
    return pcppSetSendPacket(&stream->send_data, keyword, value);
}

char *pcpaGetCommand(pcpa_t *pcpa, char *command) {
    if (pcpa == NULL) {
        amiDebugLog("error PCPA stream isn't set");
        return NULL;
    }

    return pcppGetCommand(&pcpa->recv_data, command);
}

e_pcpa_t pcpaServer(pcpa_t *stream, timeout_t timeout_ms) {
    byte bVar1;
    pcpa_callback *callback;
    char *pbVar3;
    int iVar3;
    e_pcpa_t eVar4;
    char *pbVar4;
    uint ms;
    e_pcpp_t iVar5;
    bool bVar6;
    uint local_14;
    amtime_t time;
    amtime_t time_start;
    int timeout;

    local_14 = timeout_ms;
    if (stream == NULL) {
        amiDebugLog("error PCPA stream isn't set");
        return e_pcpa_stream_unset;
    }
    amiTimerGet(&time_start);
    timeout = timeout_ms;
    do {
        ms = 0;
        switch (stream->state) {
            case pcpa_state_none:
                amiTimerGet(&time);
                ms = _amTimeDelta(time, time_start);
                if (timeout_ms != TIMEOUT_NONE) {
                    if (ms < (uint)timeout_ms) {
                        timeout = timeout_ms - ms;
                        local_14 = timeout;
                    } else {
                        timeout = 0;
                        local_14 = timeout;
                    }
                }
                stream->state = pcpa_state_wait_request;
                iVar5 = pcppRecvRequest(&stream->pcpp, &stream->recv_data, timeout);
                eVar4 = _errP2A(iVar5);
                stream->err = eVar4;
                timeout = local_14;
                if (eVar4 != e_pcpa_to) {
                    if (eVar4 == e_pcpa_ok) {
                        stream->state = 10;
                        amiTimerGet(&time);
                        if (timeout_ms != TIMEOUT_NONE &&
                            _amTimeDelta(time, time_start) >= timeout_ms) {
                            stream->err = e_pcpa_to;
                            return stream->err;
                        }
                    } else {
                        stream->state = pcpa_state_none;
                    }
                }
                break;
            default:
                stream->err = e_pcpa_unknown;
                goto LAB_00454d84;
            case pcpa_state_wait_request:
            case pcpa_state_send_response:
            case 9:
                amiTimerGet(&time);
                ms = _amTimeDelta(time, time_start);
                if (timeout_ms != TIMEOUT_NONE) {
                    if (ms < (uint)timeout_ms) {
                        timeout = timeout_ms - ms;
                        local_14 = timeout;
                    } else {
                        timeout = 0;
                        local_14 = timeout;
                    }
                }
                iVar5 = pcppIsBusy(&stream->pcpp, timeout);
                eVar4 = _errP2A(iVar5);
                stream->err = eVar4;
                timeout = local_14;
                if (eVar4 != e_pcpa_to) {
                    if (stream->state == pcpa_state_send_response) {
                        memset(&stream->send_data, 0, PCP_BUF_MAX);
                        (stream->send_data).length = 0;
                    }
                    if (stream->err == e_pcpa_ok) {
                        if (stream->binary_mode == binary_mode_none) {
                            stream->state = (stream->state != pcpa_state_wait_request) - 1 & 10;
                            amiTimerGet(&time);
                            if (timeout_ms != TIMEOUT_NONE &&
                                _amTimeDelta(time, time_start) >= timeout_ms) {
                                stream->err = e_pcpa_to;
                                return stream->err;
                            }
                        } else {
                            pcpaCloseBinary(stream);
                            stream->state = pcpa_state_none;
                            stream->binary_mode = binary_mode_none;
                            if (stream->binary_mode_after_cb != NULL) {
                                (*stream->binary_mode_after_cb)(stream,
                                                                stream->binary_mode_after_data);
                                stream->binary_mode_after_cb = NULL;
                            }
                            amiTimerGet(&time);
                            if (timeout_ms != TIMEOUT_NONE &&
                                _amTimeDelta(time, time_start) >= timeout_ms) {
                                stream->err = e_pcpa_to;
                                return stream->err;
                            }
                        }
                    } else {
                        if ((stream->state == 9) && (stream->binary_mode != binary_mode_none)) {
                            pcpaCloseBinary(stream);
                            if (stream->binary_mode_after_cb != NULL) {
                                (*stream->binary_mode_after_cb)(stream,
                                                                stream->binary_mode_after_data);
                                stream->binary_mode_after_cb = NULL;
                            }
                            stream->binary_mode = binary_mode_none;
                        }
                        stream->state = pcpa_state_none;
                    }
                }
                break;
            case pcpa_state_send_request_binary:
                callback = stream->binary_mode_before_cb;
                (stream->pcpp).last_active = _amTimeMs(time_start);
                if (callback != NULL) {
                    (*callback)(stream, stream->binary_mode_before_data);
                    stream->binary_mode_before_cb = NULL;
                }
                amiTimerGet(&time);
                if (timeout_ms != TIMEOUT_NONE) {
                    if (_amTimeDelta(time, time_start) > timeout_ms) {
                        timeout = timeout_ms - ms;
                        local_14 = timeout;
                    } else {
                        timeout = 0;
                        local_14 = timeout;
                    }
                }
                eVar4 = pcpaSendBinary(stream, timeout);
                stream->err = eVar4;
                if (eVar4 == e_pcpa_to) {
                    stream->state = pcpa_state_wait_binary;
                } else {
                    pcpaCloseBinary(stream);
                    if (stream->binary_mode_after_cb != NULL) {
                        (*stream->binary_mode_after_cb)(stream, stream->binary_mode_after_data);
                        stream->binary_mode_after_cb = NULL;
                    }
                    stream->binary_mode = binary_mode_none;
                    stream->state = pcpa_state_none;
                    if (stream->err == e_pcpa_ok) {
                    LAB_00454a1a:
                        amiTimerGet(&time);
                        if (timeout_ms != TIMEOUT_NONE &&
                            _amTimeDelta(time, time_start) >= timeout_ms) {
                            stream->err = e_pcpa_to;
                            return stream->err;
                        }
                    } else {
                        amiDebugLog("error pcpaSendBinary");
                    }
                }
                break;
            case pcpa_state_wait_response_binary:
                callback = stream->binary_mode_before_cb;
                (stream->pcpp).last_active = _amTimeMs(time_start);
                if (callback != NULL) {
                    (*callback)(stream, stream->binary_mode_before_data);
                    stream->binary_mode_before_cb = NULL;
                }
                amiTimerGet(&time);
                ms = _amTimeDelta(time, time_start);
                if (timeout_ms != TIMEOUT_NONE) {
                    if (ms < (uint)timeout_ms) {
                        timeout = timeout_ms - ms;
                        local_14 = timeout;
                    } else {
                        timeout = 0;
                        local_14 = timeout;
                    }
                }
                eVar4 = pcpaRecvBinary(stream, timeout);
                stream->err = eVar4;
                if (eVar4 == e_pcpa_to) {
                    stream->state = pcpa_state_wait_binary;
                } else {
                    pcpaCloseBinary(stream);
                    if (stream->binary_mode_after_cb != NULL) {
                        (*stream->binary_mode_after_cb)(stream, stream->binary_mode_after_data);
                        stream->binary_mode_after_cb = NULL;
                    }
                    stream->binary_mode = binary_mode_none;
                    stream->state = pcpa_state_none;
                    if (stream->err == e_pcpa_ok) goto LAB_00454a1a;
                    amiDebugLog("error pcpaRecvBinary");
                }
                break;
            case 10:
                // TODO: Figure out why recv_buf is empty at this point in the FSM
                if (stream->before_cb != NULL)
                    stream->before_cb(stream, (char *)stream->pcpp.sock.recv_buf,
                                      *stream->pcpp.sock.recv_buf_count);

                if (stream->callback_table == NULL) {
                    amiDebugLog("error Callback_table buffer isn't set");

                    stream->err = e_pcpa_cb_table_unset;
                    return stream->err;
                }
                if (stream->callback_count != 0) {
                    iVar5 = 0;
                    do {
                        pbVar3 = pcppGetKeyword(&stream->recv_data, 0);
                        pbVar4 = stream->callback_table->keyword + iVar5;
                        do {
                            bVar1 = *pbVar3;
                            bVar6 = bVar1 < (byte)*pbVar4;
                            if (bVar1 != *pbVar4) {
                            LAB_004547c8:
                                iVar3 = (1 - (uint)bVar6) - (uint)(bVar6 != 0);
                                goto LAB_004547cd;
                            }
                            if (bVar1 == 0) break;
                            bVar1 = ((byte *)pbVar3)[1];
                            bVar6 = bVar1 < ((byte *)pbVar4)[1];
                            if (bVar1 != ((byte *)pbVar4)[1]) goto LAB_004547c8;
                            pbVar3 = (char *)((byte *)pbVar3 + 2);
                            pbVar4 = (char *)((byte *)pbVar4 + 2);
                        } while (bVar1 != 0);
                        iVar3 = 0;
                    LAB_004547cd:
                        if (iVar3 == 0) {
                            (stream->callback_table[ms].callback)(stream,
                                                                  stream->callback_table[ms].data);
                            goto LAB_004547f1;
                        }
                        ms = ms + 1;
                        iVar5 = iVar5 + 40;
                    } while (ms < stream->callback_count);
                }
                pcppSetSendPacket(&stream->send_data, "?", 0);
            LAB_004547f1:
                amiTimerGet(&time);
                ms = _amTimeDelta(time, time_start);
                if (timeout_ms != TIMEOUT_NONE) {
                    if (ms < (uint)timeout_ms) {
                        local_14 = timeout_ms - ms;
                    } else {
                        local_14 = 0;
                    }
                }
                iVar5 = pcppSendResponseTable(&stream->pcpp, &stream->send_data, local_14);
                eVar4 = _errP2A(iVar5);
                stream->err = eVar4;
                if (eVar4 == e_pcpa_to) {
                    stream->state = pcpa_state_send_response;
                } else {
                    memset(&stream->send_data, 0, PCP_BUF_MAX);
                    (stream->send_data).length = 0;
                    stream->state = pcpa_state_none;
                }
                if (stream->binary_mode != binary_mode_none) {
                    stream->state = (stream->binary_mode == binary_mode_send) ? 7 : 8;
                }
                amiTimerGet(&time);
                timeout = local_14;
                if (timeout_ms != TIMEOUT_NONE && _amTimeDelta(time, time_start) >= timeout_ms) {
                    stream->err = e_pcpa_to;
                    return stream->err;
                }
        }
    } while (stream->err == e_pcpa_ok);
LAB_00454d84:
    return stream->err;
}

e_pcpa_t pcpaRecvBinary(pcpa_t *stream, timeout_t something) {
    if (stream == NULL) {
        amiDebugLog("error PCPA stream isn't set");
        return e_pcpa_stream_unset;
    }
    if (stream->recv_buffer == NULL) {
        amiDebugLog("error Recv buffer isn't set");
        pcpaCloseBinary(stream);
        pcpaClose(stream);
        return stream->err = e_pcpa_cb_table_full;
    }

    stream->state = pcpa_state_wait_response_binary;
    e_pcpa_t err;
    stream->err = err = _errP2A(
        pcppRecvBinary(&stream->pcpp, stream->recv_buffer, stream->recv_buffer_len, something));
    if (err != e_pcpa_to) stream->state = pcpa_state_none;

    return err;
}

e_pcpa_t pcpaSendBinary(pcpa_t *stream, timeout_t param_2) {
    if (stream == NULL) {
        amiDebugLog("error PCPA stream isn't set");
        return e_pcpa_stream_unset;
    }
    if (stream->send_buffer == NULL) {
        amiDebugLog("error Send buffer isn't set");
        stream->err = e_pcpa_cb_table_full;
        pcpaCloseBinary(stream);
        pcpaClose(stream);
        return stream->err;
    }

    stream->state = pcpa_state_send_request_binary;
    e_pcpa_t err;
    stream->err = err = _errP2A(
        pcppSendBinary(&stream->pcpp, stream->send_buffer, stream->send_buffer_len, param_2));
    if (err != e_pcpa_to) stream->state = pcpa_state_none;

    return err;
}

e_pcpa_t pcpaSetSendBinaryBuffer(pcpa_t *stream, const unsigned char *send_buffer, size_t len) {
    if (stream == NULL) {
        amiDebugLog("error PCPA stream isn't set");
        return e_pcpa_stream_unset;
    }
    if (send_buffer == NULL) {
        amiDebugLog("error Send Buffer isn't set");
        return e_pcpa_timeout_closed;
    }
    stream->send_buffer = send_buffer;
    stream->send_buffer_len = len;
    stream->err = e_pcpa_ok;
    return e_pcpa_ok;
}

e_pcpa_t pcpaSetBeforeBinaryModeCallBackFunc(pcpa_t *stream, pcpa_callback *callback, void *data) {
    if (stream == NULL) {
        amiDebugLog("error don't set stream");
        return e_pcpa_stream_unset;
    }
    if (callback == NULL) {
        amiDebugLog("error Binary mode callback func isn't set");
        return e_pcpa_generic;
    }
    stream->binary_mode_before_cb = callback;
    stream->binary_mode_before_data = data;
    stream->err = e_pcpa_ok;
    return e_pcpa_ok;
}

e_pcpa_t pcpaSetAfterBinaryModeCallBackFunc(pcpa_t *stream, pcpa_callback *callback, void *data) {
    if (stream == NULL) {
        amiDebugLog("error don't set stream");
        return e_pcpa_stream_unset;
    }
    if (callback == NULL) {
        amiDebugLog("error Binary mode callback func isn't set");
        return e_pcpa_generic;
    }
    stream->binary_mode_after_cb = callback;
    stream->binary_mode_after_data = data;
    stream->err = e_pcpa_ok;
    return e_pcpa_ok;
}

e_pcpa_t pcpaSetBinaryMode(pcpa_t *stream, binary_mode_t binary_mode) {
    if (stream == NULL) {
        amiDebugLog("error PCPA stream isn't set");
        return e_pcpa_stream_unset;
    }
    if (pcppGetServerSocket(&stream->pcpp, 1) == HANDLE_INVAL) {
        return stream->err = -19;
    }
    stream->binary_mode = binary_mode;
    stream->err = e_pcpa_ok;
    return e_pcpa_ok;
}

e_pcpa_t pcpaSetRecvBinaryBuffer(pcpa_t *stream, unsigned char *recv_buffer, size_t len) {
    if (stream == NULL) {
        amiDebugLog("error PCPA stream isn't set");
        return e_pcpa_stream_unset;
    }
    if (recv_buffer == NULL) {
        amiDebugLog("error Recv Buffer isn't set");
        return e_pcpa_timeout_closed;
    }
    stream->recv_buffer = recv_buffer;
    stream->recv_buffer_len = len;
    stream->err = e_pcpa_ok;
    return e_pcpa_ok;
}

pcp_send_data_t *pcpaAddSendPacket(pcpa_t *stream, char *keyword, char *value) {
    if (stream == NULL) {
        amiDebugLog("error PCPA stream isn't set");
        return NULL;
    }
    return pcppAddSendPacket(&stream->send_data, keyword, value);
}

char *pcpaGetKeyword(pcpa_t *stream, uint keyword_num) {
    if (stream == NULL) {
        amiDebugLog("error PCPA stream isn't set");
        return NULL;
    }
    return pcppGetKeyword(&stream->recv_data, keyword_num);
}

e_pcpa_t pcpaIsBusy(pcpa_t *pcpa, timeout_t timeout) {
    amtime_t now;
    amtime_t start;

    if (pcpa == NULL) {
        amiDebugLog("error PCPA stream isn't set");
        return e_pcpa_stream_unset;
    }

    amiTimerGet(&start);
    e_pcpa_t err = _pcpaGetErrorFromPcpp(pcppIsBusy(&pcpa->pcpp, timeout));
    pcpa->err = err;
    if (err == e_pcpa_to) return e_pcpa_to;

    if (pcpa->state == pcpa_state_send_request)
        memset(pcpa->send_data.data, 0, sizeof pcpa->send_data.data);
    if (pcpa->err != e_pcpa_to) pcpa->state = pcpa_state_none;

    if (pcpa->state == 11) {
        pcpa->state = 12;
        if (timeout != TIMEOUT_NONE) {
            amiTimerGet(&now);
            timeout_t dt = _amTimeDelta(now, start);
            if (dt < timeout) {
                timeout -= dt;
            } else {
                timeout = 0;
            }
        }
        err = _pcpaGetErrorFromPcpp(pcppRecvResponse(&pcpa->pcpp, &pcpa->recv_data, timeout));
        pcpa->err = err;
        if (err != e_pcpa_to) pcpa->state = pcpa_state_none;
    } else if (pcpa->state == 13) {
        pcpa->state = 14;
        if (timeout != TIMEOUT_NONE) {
            amiTimerGet(&now);
            timeout_t dt = _amTimeDelta(now, start);
            if (dt < timeout) {
                timeout -= dt;
            } else {
                timeout = 0;
            }
        }

        e_pcpp_t pcpp_err;
        if (pcpa->binary_mode == binary_mode_send) {
            pcpp_err =
                pcppSendBinary(&pcpa->pcpp, pcpa->send_buffer, pcpa->send_buffer_len, timeout);
        } else {
            pcpp_err =
                pcppRecvBinary(&pcpa->pcpp, pcpa->recv_buffer, pcpa->recv_buffer_len, timeout);
        }
        err = _pcpaGetErrorFromPcpp(pcpp_err);
        pcpa->err = err;
        if (err != e_pcpa_to) {
            pcppCloseBinary(&pcpa->pcpp);
            return pcpa->err;
        }
    } else if (pcpa->state == 14 && pcpa->err != e_pcpa_to) {
        pcppCloseBinary(&pcpa->pcpp);
        return pcpa->err;
    }
    return pcpa->err;
}

e_pcpa_t pcpaSendRequest(pcpa_t *stream, timeout_t timeout) {
    if (stream == NULL) {
        amiDebugLog("error PCPA stream isn't set");
        return e_pcpa_stream_unset;
    }

    stream->state = pcpa_state_send_request;
    e_pcpa_t err =
        _pcpaGetErrorFromPcpp(pcppSendRequestTable(&stream->pcpp, &stream->send_data, timeout));
    stream->err = err;
    if (err != e_pcpa_to) {
        if (err != e_pcpa_ok) amiDebugLog("Error : pcppSendRequestTable(%d)", err);
        memset(&stream->send_data, 0, sizeof stream->send_data);
        stream->state = pcpa_state_none;
    }
    return stream->err;
}

e_pcpa_t pcpaRecvResponse(pcpa_t *stream, timeout_t timeout) {
    if (stream == NULL) {
        amiDebugLog("error PCPA stream isn't set");
        return e_pcpa_stream_unset;
    }

    stream->state = pcpa_state_wait_response;
    e_pcpa_t err =
        _pcpaGetErrorFromPcpp(pcppRecvResponse(&stream->pcpp, &stream->recv_data, timeout));
    stream->err = err;
    if (err != e_pcpa_to) {
        if (err != e_pcpa_ok) amiDebugLog("Error : pcppRecvResponse(%d)", err);
        stream->state = pcpa_state_none;
    }
    return err;
}

e_pcpa_t pcpaOpenBinaryClient(pcpa_t *stream, ushort port, timeout_t timeout) {
    if (stream == NULL) {
        amiDebugLog("error PCPA stream isn't set");
        return e_pcpa_stream_unset;
    }

    SOCKADDR_IN *addr = (SOCKADDR_IN *)pcppGetSockAddr(&stream->pcpp, false);
    stream->state = pcpa_state_open_binary;
    // We can't use inet_ntop because we target XP
#pragma warning(suppress : 4996)
    char *ipAddr = inet_ntoa(addr->sin_addr);

    e_pcpa_t pcpa_err =
        _pcpaGetErrorFromPcpp(pcppOpenBinaryClient(&stream->pcpp, ipAddr, port, timeout));
    stream->err = pcpa_err;
    if (pcpa_err == e_pcpa_to) {
        stream->state = pcpa_state_none;
    }
    return pcpa_err;
}

e_pcpa_t pcpaAccessClient(pcpa_t *stream, timeout_t timeout) {
    if (stream == NULL) {
        amiDebugLog("error PCPA stream isn't set");
        return e_pcpa_stream_unset;
    }

    amtime_t start, now;
    e_pcpp_t pcpa_err;

    amiTimerGet(&start);
    stream->state = pcpa_state_send_request_table;

    pcpa_err =
        _pcpaGetErrorFromPcpp(pcppSendRequestTable(&stream->pcpp, &stream->send_data, timeout));
    stream->err = pcpa_err;
    if (pcpa_err != e_pcpa_to) stream->state = pcpa_state_none;

    if (pcpa_err != e_pcpa_ok) return pcpa_err;

    if (pcpa_err == 0) {
        stream->state = pcpa_state_12;

        if (timeout != TIMEOUT_NONE) {
            amiTimerGet(&now);
            timeout_t delta = amiTimerDiffMsec(&start, &now);
            if (delta < timeout) {
                timeout -= delta;
            } else {
                timeout = 0;
            }
        }

        pcpa_err =
            _pcpaGetErrorFromPcpp(pcppRecvResponse(&stream->pcpp, &stream->recv_data, timeout));
        stream->err = pcpa_err;
        if (pcpa_err != e_pcpa_to) stream->state = pcpa_state_none;
    }
    return pcpa_err;
}
