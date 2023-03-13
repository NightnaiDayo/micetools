#include "pcpt.h"

e_pcpt_t _pcptGetErrorFromWin(int err) {
    switch (err) {
        case WSAEBADF:
        case WSAEINVAL:
        case WSAENOTSOCK:
        case WSAEISCONN:
            return e_pcpt_wsa2_generic;
        case WSAEMFILE:
            return e_pcpt_cannot_open;
        case WSAEWOULDBLOCK:
        case WSAETIMEDOUT:
            return e_pcpt_to;
        case WSAEADDRNOTAVAIL:
            return e_pcpt_inval_addr;
        case WSAECONNABORTED:
        case WSAECONNRESET:
            return e_pcpt_closed;
        case WSAENOBUFS:
            return e_pcpt_nobufs;
        case WSANOTINITIALISED:
            return e_pcpt_wsa_noinit;
        default:
            return e_pcpt_unknown;
    }
}

e_pcpt_t pcptInitStream(pcpt_t *sock) {
    SOCKET s;

    if (sock == NULL) {
        PCP_LOG("error PCP stream isn\'t set\n");
        return e_pcpt_pointer_unset;
    }

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == SOCKET_INVAL) return _errW2T(WSAGetLastError());
    closesocket(s);

    sock->client_open = 0;
    sock->open = PCPT_CLOSED;
    sock->client_event = NULL;
    sock->server_event = NULL;
    sock->send_buf = NULL;
    sock->send_buf_count = NULL;
    sock->recv_buf_count = NULL;
    sock->recv_buf = NULL;
    sock->server_sock = SOCKET_INVAL;
    sock->client_sock = SOCKET_INVAL;
    sock->err = e_pcpt_unknown;
    memset(&sock->client_addr, 0, sizeof sock->client_addr);
    sock->field_0x58 = 0;
    sock->tcp_nodelay = 0;
    sock->config_0 = 12000;
    sock->config_1 = 5;
    sock->so_linger = 1;

    return e_pcpt_ok;
}

void pcptClose(pcpt_t *sock) {
    if (sock == NULL) {
        PCP_LOG("error PCP stream isn\'t set\n");
        return;
    }

    if (sock->server_sock != SOCKET_INVAL) {
        WSAEventSelect(sock->server_sock, 0, 0);
        WSACloseEvent(sock->server_event);
        sock->server_event = NULL;
        closesocket(sock->server_sock);
        sock->server_sock = SOCKET_INVAL;
    }
    if (sock->client_sock != SOCKET_INVAL) {
        WSAEventSelect(sock->client_sock, 0, 0);
        WSACloseEvent(sock->client_event);
        sock->client_event = NULL;
        closesocket(sock->client_sock);
        sock->client_sock = SOCKET_INVAL;
    }
    sock->client_open = 0;
    sock->open = PCPT_CLOSED;
    sock->err = e_pcpt_unknown;
    if (sock->client_event != NULL) {
        WSACloseEvent(sock->client_event);
        sock->client_event = NULL;
    }
    if (sock->server_event != NULL) {
        WSACloseEvent(sock->server_event);
        sock->server_event = NULL;
    }
    sock->send_buf = NULL;
    sock->send_buf_count = NULL;
    sock->recv_buf_count = NULL;
    sock->recv_buf = NULL;
    memset(&sock->client_addr, 0, sizeof sock->client_addr);
    sock->field_0x58 = 0;
}

void pcptCloseDataSock(pcpt_t *sock) {
    if (sock == NULL) {
        PCP_LOG("error PCP stream isn\'t set\n");
        return;
    }
    if (sock->field_0x58 != 0) {
        sock->open = PCPT_LISTENING;
    }
    sock->client_open = 0;
    if (sock->client_sock != SOCKET_INVAL) {
        WSAEventSelect(sock->client_sock, 0, 0);
        WSACloseEvent(sock->client_event);
        sock->client_event = NULL;
        closesocket(sock->client_sock);
        sock->client_sock = SOCKET_INVAL;
    }
}

e_pcpt_t pcptAcceptServer(pcpt_t *stream, timeout_t timeout_ms) {
    if (stream == NULL) {
        PCP_LOG("error PCP stream isn\'t set\n");
        return e_pcpt_pointer_unset;
    }

    e_pcpt_t err = pcptCheckEvent(stream->server_event, timeout_ms, stream->server_sock, FD_ACCEPT);
    if (err != e_pcpt_ok) return err;

    int addrlen = 16;
    SOCKET client = accept(stream->server_sock, &stream->client_addr, &addrlen);
    stream->client_sock = client;
    if (client == SOCKET_INVAL) return _errW2T(GetLastError());

    WSAEVENT event = WSACreateEvent();
    stream->client_event = event;
    if (event == NULL) {
        closesocket(stream->client_sock);
        stream->client_sock = SOCKET_INVAL;
        return e_pcpt_unknown;
    }
    WSAEventSelect(stream->client_sock, event, FD_READ | FD_WRITE | FD_CLOSE);

    uint so_linger = (uint)(!!stream->so_linger);
    setsockopt(stream->client_sock, SOL_SOCKET, SO_LINGER, (void *)&so_linger, sizeof so_linger);

    uint nodelay = (uint)(!stream->tcp_nodelay);
    setsockopt(stream->client_sock, IPPROTO_TCP, TCP_NODELAY, (void *)&nodelay, sizeof nodelay);
    return e_pcpt_ok;
}

e_pcpt_t pcptCheckConnectAble(pcpt_t *stream, timeout_t timeout_ms) {
    DWORD DVar1;
    uint uVar3;
    HANDLE event;
    bool bVar5;
    e_pcpt_t local_44;
    u_long local_40;
    amtime_t now;
    amtime_t start;

    if (stream == NULL) {
        PCP_LOG("error PCP stream isn\'t set\n");
        return e_pcpt_pointer_unset;
    }
    amiTimerGet(&start);
    DVar1 = timeout_ms;

    if (timeout_ms == TIMEOUT_NONE) DVar1 = TIMEOUT_NONE;

    while (DVar1 = WaitForSingleObject(stream->client_event, DVar1), DVar1 == WAIT_TIMEOUT) {
    LAB_004589c8:
        amiTimerGet(&now);
        uVar3 = _amTimeDelta(now, start);
        if (timeout_ms <= uVar3) return e_pcpt_to;

        DVar1 = timeout_ms - uVar3;
    }

    if (DVar1 == 0) {
        WSANETWORKEVENTS net_events;

        if (FAILED(WSAEnumNetworkEvents(stream->client_sock, stream->client_event, &net_events))) {
            bVar5 = WSAGetLastError() == WSAEWOULDBLOCK;
        } else {
            if ((net_events.lNetworkEvents & 0x10) != 0 &&
                net_events.iErrorCode[FD_CONNECT_BIT] == 0) {
                local_44 = e_pcpt_ok;
                goto LAB_00458a1e;
            }
            if (WSAGetLastError() == WSAEWOULDBLOCK) goto LAB_004589c8;
            bVar5 = net_events.iErrorCode[FD_CONNECT_BIT] == WSAECONNREFUSED;
        }
        if (bVar5) goto LAB_004589c8;
    }
    local_44 = e_pcpt_wsa2_generic;

LAB_00458a1e:
    local_40 = 0;
    WSAEventSelect(stream->client_sock, 0, 0);
    WSACloseEvent(stream->client_event);
    stream->client_event = NULL;
    if (local_44 != e_pcpt_ok) {
        closesocket(stream->client_sock);
        stream->client_sock = SOCKET_INVAL;
        return local_44;
    }
    ioctlsocket(stream->client_sock, FIONBIO, &local_40);

    event = WSACreateEvent();
    stream->client_event = event;
    if (event != NULL) {
        WSAEventSelect(stream->client_sock, event, FD_READ | FD_WRITE | FD_CLOSE);
        return e_pcpt_ok;
    }
    closesocket(stream->client_sock);
    stream->client_sock = SOCKET_INVAL;
    return e_pcpt_unknown;
}

e_pcpt_t pcptCheckEvent(HANDLE event, timeout_t timeout_ms, SOCKET socket, uint event_mask) {
    amtime_t now;
    amtime_t start;
    WSANETWORKEVENTS networkEvents;

    amiTimerGet(&start);
    now.seconds = start.seconds;
    now.microseconds = start.microseconds;
    DWORD wait_timeout = timeout_ms;
    if (timeout_ms == TIMEOUT_NONE) wait_timeout = TIMEOUT_NONE;

    if (event == NULL) {
        PCP_LOG("Error : EVENT HANDLE error\n");
        return e_pcpt_pointer_unset;
    }

    event_mask &= 0xffffffdf;

    while (1) {
        DWORD err = WaitForSingleObject(event, wait_timeout);
        if ((err != WAIT_TIMEOUT && err != 0) || FAILED(err)) {
            return e_pcpt_wsa2_generic;
        }

        if (err != WAIT_TIMEOUT) {
            if (FAILED(WSAEnumNetworkEvents(socket, event, &networkEvents)))
                return e_pcpt_wsa2_generic;

            if ((networkEvents.lNetworkEvents & 0x20) != 0) return e_pcpt_closed;

            if ((event_mask & networkEvents.lNetworkEvents) != 0) return e_pcpt_ok;
        }

        if (timeout_ms != TIMEOUT_NONE) {
            amiTimerGet(&now);
            uint elapsed = _amTimeDelta(now, start);
            if ((uint)timeout_ms <= elapsed) return e_pcpt_to;

            wait_timeout = timeout_ms - elapsed;
        }
    }
}

e_pcpt_t pcptOpenDataSockServer(pcpt_t *sock, timeout_t timeout_ms) {
    if (sock == NULL) {
        PCP_LOG("error PCP stream isn\'t set\n");
        return e_pcpt_pointer_unset;
    }
    if (sock->open == PCPT_CONNECTED) return e_pcpt_already_open;
    if (sock->open == PCPT_CLOSED) return e_pcpt_not_open;

    if ((sock->client_sock == SOCKET_INVAL)) {
        e_pcpt_t err = pcptAcceptServer(sock, timeout_ms);
        if (err != e_pcpt_ok) return err;
    }

    sock->open = PCPT_CONNECTED;
    return e_pcpt_ok;
}

e_pcpt_t pcptOpenServer(pcpt_t *stream, int open_mode, ushort port) {
    e_pcpt_t err;

    if (stream == NULL) {
        PCP_LOG("error PCP stream isn\'t set\n");
        return e_pcpt_pointer_unset;
    }

    if (stream->server_sock != SOCKET_INVAL || stream->client_sock != SOCKET_INVAL)
        return stream->err = e_pcpt_already_open;

    amtime_t start;
    amiTimerGet(&start);
    stream->field_0x58 = 1;
    stream->open = PCPT_CLOSED;
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    stream->server_sock = s;

    if (s == SOCKET_INVAL) return stream->err = _errW2T(GetLastError());

    uint reuseaddr = 1;
    if (FAILED(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (void *)&reuseaddr, 4))) {
        err = _errW2T(GetLastError());
        closesocket(stream->server_sock);
        stream->server_sock = SOCKET_INVAL;
        return stream->err = err;
    }

    SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(port);
    addr.sin_addr.s_addr = ntohl(0);
    if (open_mode != OPEN_MODE_GLOBAL) {
        INT len = sizeof &addr.sin_addr.s_addr;

        WSAStringToAddressW(L"127.0.0.1", AF_INET, NULL, (LPSOCKADDR)&addr.sin_addr.s_addr, &len);
    }

    if (FAILED(bind(stream->server_sock, (SOCKADDR *)&addr, sizeof addr))) {
        err = _errW2T(GetLastError());
        closesocket(stream->server_sock);
        stream->server_sock = SOCKET_INVAL;
        return stream->err = err;
    }
    if (FAILED(listen(stream->server_sock, 128))) {
        err = _errW2T(GetLastError());
        closesocket(stream->server_sock);
        stream->server_sock = SOCKET_INVAL;
        return stream->err = err;
    }

    WSAEVENT event = WSACreateEvent();
    stream->server_event = event;
    if (event != NULL) {
        WSAEventSelect(stream->server_sock, event, FD_CLOSE | FD_ACCEPT);
        stream->open = PCPT_LISTENING;
        stream->client_open = 0;
        return stream->err = e_pcpt_ok;
    }
    closesocket(stream->server_sock);
    stream->server_sock = SOCKET_INVAL;

    return stream->err = e_pcpt_unknown;
}

e_pcpt_t pcptOpenClient(pcpt_t *stream, char *ipAddr, ushort port, timeout_t timeout) {
    pcpt_t *pcpt;
    uint uVar6;
    amtime_t start;
    amtime_t now;

    pcpt = stream;
    if ((stream == NULL) || (ipAddr == NULL)) {
        PCP_LOG("error PCP stream isn\'t set\n");
        return e_pcpt_pointer_unset;
    }
    if (stream->client_sock != 0xffffffff) {
        return stream->err = e_pcpt_already_open;
    }

    stream->field_0x58 = 0;
    stream->open = PCPT_CLOSED;
    stream->client_open = 1;

    ZeroMemory(&stream->client_addr, sizeof stream->client_addr);
    SOCKADDR_IN *pAddr = &stream->client_addr;
    pAddr->sin_port = ntohs(port);
    pAddr->sin_family = AF_INET;
    pAddr->sin_addr.s_addr = inet_addr(ipAddr);

    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    pcpt->client_sock = s;
    if (s == SOCKET_INVAL) {
        return pcpt->err = _errW2T(GetLastError());
    }

    uint so_linger = (uint)(!!stream->so_linger);
    setsockopt(stream->client_sock, SOL_SOCKET, SO_LINGER, (void *)&so_linger, sizeof so_linger);

    uint nodelay = (uint)(!stream->tcp_nodelay);
    setsockopt(stream->client_sock, IPPROTO_TCP, TCP_NODELAY, (void *)&nodelay, sizeof nodelay);

    amiTimerGet(&start);
    do {
        port = 0;
        HANDLE client_event = WSACreateEvent();
        pcpt->client_event = client_event;
        if (client_event == 0) {
            closesocket(pcpt->client_sock);
            pcpt->err = -0x11;
            pcpt->client_sock = 0xffffffff;
            return pcpt->err;
        }
        WSAEventSelect(pcpt->client_sock, client_event, 0x30);

        if (connect(pcpt->client_sock, &stream->client_addr, 0x10) != -1) {
            WSAEventSelect(pcpt->client_sock, 0, 0);
            WSACloseEvent(pcpt->client_event);
            pcpt->client_event = 0;

            u_long local_40 = 0;
            ioctlsocket(pcpt->client_sock, FIONBIO, &local_40);

            client_event = WSACreateEvent();
            pcpt->client_event = client_event;
            if (client_event == 0) {
                closesocket(pcpt->client_sock);
                pcpt->err = -0x11;
                pcpt->client_sock = 0xffffffff;
                return pcpt->err;
            }
            WSAEventSelect(pcpt->client_sock, client_event, 0x23);
            pcpt->err = 0;
            pcpt->client_open = 0;
            pcpt->open = 2;
            return 0;
        }
        if ((pcpt->err = _errW2T(GetLastError())) != e_pcpt_to) {
            u_long local_40 = 0;
            ioctlsocket(pcpt->client_sock, FIONBIO, &local_40);
            return pcpt->err;
        }
        amiTimerGet(&now);
        if (timeout == TIMEOUT_NONE) {
        LAB_00a19435:
            uVar6 = pcpt->config_0;
        } else {
            uVar6 = (now.microseconds - start.microseconds) / 1000 +
                    (now.seconds - start.seconds) * 1000;
            if (uVar6 < timeout) {
                uVar6 = timeout - uVar6;
            } else {
                port = 1;
                uVar6 = 0;
            }
            if (pcpt->config_0 <= uVar6 && uVar6 != pcpt->config_0) goto LAB_00a19435;
        }
        if ((pcpt->err = pcptCheckConnectAble(pcpt, uVar6)) != e_pcpt_to) {
            pcpt->client_open = 0;
            if (pcpt->err == e_pcpt_ok) {
                pcpt->open = 2;
                return pcpt->err;
            }
            return pcpt->err;
        }
        if (port != 0) {
            pcpt->err = 1;
            amiTimerGet(&start);
            pcpt->field_0x40 = start.microseconds / 1000 + start.seconds * 1000;
            return pcpt->err;
        }
        WSAEventSelect(pcpt->client_sock, 0, 0);
        WSACloseEvent(pcpt->client_event);
        pcpt->client_event = 0;
        closesocket(pcpt->client_sock);

        pcpt->client_sock = SOCKET_INVAL;
        s = socket(AF_INET, SOCK_STREAM, 0);
        pcpt->client_sock = s;
        if (s == SOCKET_INVAL) {
            return pcpt->err = _errW2T(GetLastError());
        }
        if (pcpt->err != e_pcpt_to) {
            return pcpt->err;
        }
    } while (true);
}

e_pcpt_t pcptRecv(pcpt_t *sock, unsigned char *recv_buf, size_t *recv_buf_len,
                  timeout_t timeout_ms) {
    e_pcpt_t err;
    amtime_t now;
    amtime_t start;

    if (sock == NULL || recv_buf == NULL || recv_buf_len == NULL) {
        PCP_LOG("error PCP stream isn\'t set\n");
        return e_pcpt_pointer_unset;
    }

    if (sock->open == PCPT_CLOSED) {
        PCP_LOG("error PCP not open\n");
        return sock->err = e_pcpt_not_open;
    }
    // TODO: URGENT: Something in the FSM causes this condition to error out!
    if (sock->client_open != 0) {
        PCP_LOG("error PCP already connected\n");
        return sock->err = e_pcpt_already_connected;
    }

    sock->recv_buf = recv_buf;
    sock->recv_buf_count = recv_buf_len;
    sock->client_open = 3;

    if (sock->open == PCPT_LISTENING && sock->field_0x58 != 0) {
        amiTimerGet(&start);
        sock->err = err = pcptOpenDataSockServer(sock, timeout_ms);
        if (err == e_pcpt_to) return e_pcpt_to;

        if (err != e_pcpt_ok) {
            pcptCloseDataSock(sock);
            return sock->err;
        }

        if (timeout_ms != TIMEOUT_NONE) {
            amiTimerGet(&now);
            uint elapsed = _amTimeDelta(now, start);

            if (elapsed < timeout_ms)
                timeout_ms = timeout_ms - elapsed;
            else
                timeout_ms = 0;
        }
    }
    sock->err = err = pcptCheckEvent(sock->client_event, timeout_ms, sock->client_sock, FD_READ);
    if (err == e_pcpt_to) return e_pcpt_to;

    sock->client_open = 0;
    if (err == e_pcpt_ok) {
        size_t recv_count =
            recv(sock->client_sock, (char *)sock->recv_buf, *sock->recv_buf_count, 0);
        if (0 < (int)recv_count) {
            *recv_buf_len = recv_count;
            sock->err = e_pcpt_ok;
            return e_pcpt_ok;
        }

        if (recv_count == 0) {
            pcptCloseDataSock(sock);
            return sock->err = e_pcpt_closed;
        }
        sock->err = _errW2T(GetLastError());
    }

    pcptCloseDataSock(sock);
    return sock->err;
}

e_pcpt_t pcptSend(pcpt_t *sock, unsigned char *send_buf, size_t *send_len, uint param_4,
                  timeout_t timeout_ms) {
    e_pcpt_t err;
    amtime_t now;
    amtime_t start_time;

    if (sock == NULL || send_buf == NULL || send_len == NULL) {
        PCP_LOG("error PCP stream isn\'t set\n");
        return e_pcpt_pointer_unset;
    }

    if (sock->open == PCPT_CLOSED) {
        PCP_LOG("error PCP not open\n");
        return sock->err = e_pcpt_not_open;
    }
    if (sock->client_open != 0) {
        PCP_LOG("error PCP already connected\n");
        return sock->err = e_pcpt_already_connected;
    }

    sock->send_buf_count = send_len;
    sock->send_buf = send_buf;
    sock->client_open = 2;
    sock->field_0x14 = param_4;
    sock->field_0x54 = 0;

    if (sock->open == PCPT_LISTENING && sock->field_0x58 != 0) {
        amiTimerGet(&start_time);
        err = pcptOpenDataSockServer(sock, timeout_ms);
        sock->err = err;
        if (err == e_pcpt_to) return e_pcpt_to;

        if (err == e_pcpt_ok) {
            amiTimerGet(&now);
            uint elapsed = _amTimeDelta(now, start_time);
            if (timeout_ms != TIMEOUT_NONE) {
                if (elapsed < timeout_ms)
                    timeout_ms = timeout_ms - elapsed;
                else
                    timeout_ms = 0;
            }
            goto LAB_0045930e;
        }
    } else {
    LAB_0045930e:
        err = pcptSendAllMsg(sock, timeout_ms);
        sock->err = err;
        if (err == e_pcpt_to) {
            return e_pcpt_to;
        }
        sock->client_open = 0;
        if (err == e_pcpt_ok) return err;
    }
    pcptCloseDataSock(sock);

    return sock->err;
}

e_pcpt_t pcptSendAllMsg(pcpt_t *sock, timeout_t timeout_ms) {
    size_t sVar1;
    bool bVar2;
    e_pcpt_t err;
    bool bVar3;
    amtime_t now;
    amtime_t start;

    if (sock == NULL) {
        PCP_LOG("error PCP stream isn\'t set\n");
        return e_pcpt_pointer_unset;
    }

    if (sock->send_buf == NULL || sock->send_buf_count == NULL) {
        PCP_LOG("error send buffer isn\'t set\n");
        return e_pcpt_pointer_unset;
    }

    bVar3 = sock->field_0x54 == 0;
    amiTimerGet(&start);
    sVar1 = *sock->send_buf_count;
    while (1) {
        if (sVar1 == 0) {
            sock->field_0x54 = 0;
            return e_pcpt_ok;
        }

        bVar2 = false;
        amiTimerGet(&now);
        uint elapsed = _amTimeDelta(now, start);
        if (timeout_ms != TIMEOUT_NONE && timeout_ms < elapsed) return e_pcpt_to;

        uint wait_time;
        if (bVar3) {
            wait_time = timeout_ms;
            if (timeout_ms != TIMEOUT_NONE) wait_time = timeout_ms - elapsed;
        } else {
            if ((timeout_ms == TIMEOUT_NONE) ||
                (wait_time = timeout_ms - elapsed,
                 sock->field_0x14 <= wait_time && wait_time != sock->field_0x14)) {
                wait_time = sock->field_0x14;
                bVar2 = true;
            }
        }
        int sent = send(sock->client_sock, (char *)sock->send_buf, *sock->send_buf_count, 0);
        if (sent == -1) {
            err = _errW2T(GetLastError());
            if (err != e_pcpt_to) return err;

            err = pcptCheckEvent(sock->client_event, wait_time, sock->client_sock, FD_WRITE);
            if (err == e_pcpt_to) {
                if (!bVar2) return e_pcpt_to;

                sock->field_0x54 = 0;
                return e_pcpt_NO_IDEA_WHAT_THIS_IS;
            }

            if (err != e_pcpt_ok) return err;
        } else {
            sock->send_buf = sock->send_buf + sent;
            sock->field_0x54 = 1;
            *sock->send_buf_count = *sock->send_buf_count - sent;
        }

        bVar3 = false;
        sVar1 = *sock->send_buf_count;
    }
}

void pcptSetConfig(pcpt_t *stream, uint config, uint value) {
    if (stream == NULL) {
        PCP_LOG("error don\'t set stream\n");
        return;
    }

    switch (config) {
        case PCPT_CONFIG_0:
            stream->config_0 = value;
            break;
        case PCPT_CONFIG_1:
            stream->config_1 = value;
            break;
        case PCPT_SO_LINGER:
            stream->so_linger = value;
            break;
        case PCPT_TCP_NODELAY:
            stream->tcp_nodelay = value;
            break;
    }
}

// TODO: TIDY

e_pcpt_t pcptIsBusy(pcpt_t *sock, timeout_t timeout_ms) {
    bool bVar1;
    bool bVar2;
    uint uVar3;
    uint uVar4;
    SOCKET SVar5;
    WSAEVENT event;
    int iVar7;
    e_pcpt_t eVar8;
    u_long timeout_ms_00;
    uint uVar10;
    amtime_t local_18;
    amtime_t local_10;
    amtime_t local_8;

    timeout_ms_00 = timeout_ms;
    if (sock == NULL) {
        PCP_LOG("error PCP stream isn\'t set\n");
        return e_pcpt_pointer_unset;
    }
    switch (sock->client_open) {
        case 0:
            sock->err = e_pcpt_no_client;
            return sock->err;
        case 1:
            if (sock->open != PCPT_CLOSED) {
                sock->err = e_pcpt_cannot_open;
                sock->client_open = 0;
                return sock->err;
            }
            amiTimerGet(&local_10);
            uVar10 = timeout_ms_00;
            do {
                bVar1 = false;
                bVar2 = false;
                amiTimerGet(&local_18);
                if (uVar10 != TIMEOUT_NONE) {
                    uVar3 = _amTimeDelta(local_18, local_10);
                    if (uVar3 < uVar10) {
                        timeout_ms_00 = uVar10 - uVar3;
                    } else {
                        bVar2 = true;
                        timeout_ms_00 = 0;
                    }
                }
                amiTimerGet(&local_18);
                uVar4 = _amTimeMs(local_18) - sock->field_0x40;
                uVar3 = sock->config_0;
                if (uVar3 < uVar4) {
                    timeout_ms_00 = 0;
                LAB_00459626:
                    bVar1 = true;
                } else {
                    if ((uVar10 == TIMEOUT_NONE) || (uVar3 < timeout_ms_00)) {
                        timeout_ms_00 = uVar3 - uVar4;
                        goto LAB_00459626;
                    }
                }
                eVar8 = pcptCheckConnectAble(sock, timeout_ms_00);
                sock->err = eVar8;
                if (eVar8 != e_pcpt_to) {
                    sock->client_open = 0;
                    if (eVar8 == e_pcpt_ok) {
                        sock->open = PCPT_CONNECTED;
                        return sock->err;
                    }
                    break;
                }
                if (bVar1) {
                    WSAEventSelect(sock->client_sock, 0, 0);
                    WSACloseEvent(sock->client_event);
                    sock->client_event = NULL;
                    closesocket(sock->client_sock);
                    sock->client_sock = SOCKET_INVAL;
                    SVar5 = socket(2, 1, 0);
                    sock->client_sock = SVar5;
                    if (SVar5 == SOCKET_INVAL) goto LAB_004599cf;
                    event = WSACreateEvent();
                    sock->client_event = event;
                    if (event == NULL) {
                        closesocket(sock->client_sock);
                        sock->err = e_pcpt_unknown;
                        sock->client_sock = SOCKET_INVAL;
                        return sock->err;
                    }
                    WSAEventSelect(sock->client_sock, event, FD_CONNECT | FD_CLOSE);
                    iVar7 = connect(sock->client_sock, &sock->client_addr, 16);
                    if (iVar7 != -1) {
                        WSAEventSelect(sock->client_sock, 0, 0);
                        WSACloseEvent(sock->client_event);
                        sock->client_event = NULL;
                        timeout_ms = 0;
                        ioctlsocket(sock->client_sock, FIONBIO, (u_long *)&timeout_ms);
                        event = WSACreateEvent();
                        sock->client_event = event;
                        if (event != NULL) {
                            WSAEventSelect(sock->client_sock, event, FD_READ | FD_WRITE | FD_CLOSE);
                            sock->err = e_pcpt_ok;
                            sock->client_open = 0;
                            sock->open = PCPT_CONNECTED;
                            return e_pcpt_ok;
                        }
                        closesocket(sock->client_sock);
                        sock->err = e_pcpt_unknown;
                        sock->client_sock = SOCKET_INVAL;
                        return sock->err;
                    }
                    amiTimerGet(&local_8);
                    sock->field_0x40 = _amTimeMs(local_8);
                } else {
                    if (bVar2) {
                        sock->err = e_pcpt_to;
                        return e_pcpt_to;
                    }
                }
                uVar10 = timeout_ms;
            } while (sock->err == e_pcpt_to);
            break;
        case 2:
            if (sock->open == PCPT_CLOSED) {
                sock->err = e_pcpt_not_open;
                return e_pcpt_not_open;
            }
            if ((sock->open == PCPT_LISTENING) && (sock->field_0x58 != 0)) {
                amiTimerGet(&local_10);
                eVar8 = pcptOpenDataSockServer(sock, timeout_ms_00);
                sock->err = eVar8;
                if (eVar8 == e_pcpt_to) {
                    return e_pcpt_to;
                }
                if (eVar8 != e_pcpt_ok) {
                LAB_0045997d:
                    pcptCloseDataSock(sock);
                    return sock->err;
                }
                amiTimerGet(&local_18);
                if (timeout_ms_00 != TIMEOUT_NONE) {
                    iVar7 = (local_18.microseconds - local_10.microseconds) / 1000;
                    if (timeout_ms_00 <
                        (uint)((local_18.seconds - local_10.seconds) * 1000 + iVar7)) {
                        timeout_ms_00 =
                            ((local_10.seconds - local_18.seconds) * 1000 - iVar7) + timeout_ms_00;
                    } else {
                        timeout_ms_00 = 0;
                    }
                }
            }
            eVar8 = pcptSendAllMsg(sock, timeout_ms_00);
            sock->err = eVar8;
            if (eVar8 != e_pcpt_to) {
                if (eVar8 != e_pcpt_ok) {
                    pcptCloseDataSock(sock);
                }
                sock->client_open = 0;
                return sock->err;
            }
            break;
        case 3:
            if (sock->open == PCPT_CLOSED) {
                sock->err = e_pcpt_not_open;
                return sock->err;
            }
            if ((sock->open == PCPT_LISTENING) && (sock->field_0x58 != 0)) {
                amiTimerGet(&local_10);
                eVar8 = pcptOpenDataSockServer(sock, timeout_ms_00);
                sock->err = eVar8;
                if (eVar8 == e_pcpt_to) {
                    return e_pcpt_to;
                }
                if (eVar8 != e_pcpt_ok) goto LAB_0045997d;
                amiTimerGet(&local_18);
                if (timeout_ms_00 != TIMEOUT_NONE) {
                    iVar7 = (local_18.microseconds - local_10.microseconds) / 1000;
                    if (timeout_ms_00 <
                        (uint)((local_18.seconds - local_10.seconds) * 1000 + iVar7)) {
                        timeout_ms_00 =
                            ((local_10.seconds - local_18.seconds) * 1000 - iVar7) + timeout_ms_00;
                    } else {
                        timeout_ms_00 = 0;
                    }
                }
            }
            eVar8 = pcptCheckEvent(sock->client_event, timeout_ms_00, sock->client_sock, 1);
            sock->err = eVar8;
            if (eVar8 == e_pcpt_to) break;
            sock->client_open = 0;
            if (eVar8 != e_pcpt_ok) goto LAB_0045997d;
            if (sock->recv_buf == NULL || sock->recv_buf_count == NULL) {
                PCP_LOG("error Recv buffer isn\'t set\n");
                return sock->err = e_pcpt_recv_unset;
            }
            size_t received =
                recv(sock->client_sock, (char *)sock->recv_buf, *sock->recv_buf_count, 0);
            if (received < 1) {
                pcptCloseDataSock(sock);
                if (received == 0) {
                    sock->err = e_pcpt_closed;
                    return e_pcpt_closed;
                }
            LAB_004599cf:
                return sock->err = _errW2T(GetLastError());
            }
            *sock->recv_buf_count = received;
        default:
            sock->err = e_pcpt_ok;
    }
    return sock->err;
}
