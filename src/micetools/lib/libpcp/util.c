#include "pcpa.h"

void pcptPrint(pcpt_t* stream, char* name) {
    printf("  PCPT %s STATUS\n", name);
    printf("    Current errno: %d\n", stream->err);
    printf("    Server socket: %08X (%d)\n", stream->server_sock, stream->open);
    printf("    Client socket: %08X (%d)\n", stream->client_sock, stream->client_open);
    if (stream->send_buf)
        printf("    Send buffer: %.*s\n", *stream->send_buf_count, stream->send_buf);
    else
        puts("    Send buffer: -");
    if (stream->recv_buf)
        printf("    Recv buffer: %.*s\n", *stream->recv_buf_count, stream->recv_buf);
    else
        puts("    Recv buffer: -");
}

void pcppPrint(pcpp_t* stream) {
    puts("PCPP STATUS");
    printf("  Current errno: %d  state: %d\n", stream->err, stream->state);
    printf("  Open: %d at %d\n", stream->open, stream->last_active);
    printf("  Read buf: %.*s\n", stream->read_bytes_num, stream->read_bytes_buf);
    printf("  Send buf: %.*s\n", stream->send_buf_len, stream->send_buf);
    printf("  Send binary: %p (%d[?])\n", stream->send_binary_buf, stream->field_0x1f8);
    printf("  Recv binary: %p (%d)\n", stream->recv_binary_buf, stream->recv_binary_buf_len);

    pcptPrint(&stream->sock, "TEXT");
    pcptPrint(&stream->data_sock, "BINARY");
}

void pcpaPrint(pcpa_t* stream) {
    puts("PCPA STATUS");
    amtime_t now;
    amiTimerGet(&now);
    printf("Now: %d\n", _amTimeMs(now));

    printf("Current errno: %d\n", stream->err);
    printf("%d/%d callbacks registered at %p\n", stream->callback_count, stream->callback_max, stream->callback_table);
    printf("Binary mode: %d (%p/%p)\n", stream->binary_mode, stream->binary_mode_before_cb,
           stream->binary_mode_after_cb);

    if (stream->recv_buffer)
        printf("Recv buffer: %.*s\n", stream->recv_buffer_len, stream->recv_buffer);
    else
        puts("Recv buffer: -");
    if (stream->send_buffer)
        printf("Send buffer: %.*s\n", stream->send_buffer_len, stream->send_buffer);
    else
        puts("Send buffer: -");

    pcppPrint(&stream->pcpp);

    printf("Send data: %.*s\n", stream->send_data.length, stream->send_data.data);

    puts("Parse data:");
    printf(" %d commands:\n", stream->recv_data.cmd_count);
    for (size_t i = 0; i < stream->recv_data.cmd_count; i++) {
        byte kwd = stream->recv_data.keywords[i];
        if (kwd != 0) kwd++;
        printf("  & %02d '%s'\n", kwd, stream->recv_data.strings + kwd);
        byte value = stream->recv_data.values[i];
        if (value != 0) value++;
        printf("  = %02d '%s'\n", value, stream->recv_data.strings + value);
        if (stream->recv_data.strings[kwd] == 0) break;
    }
    puts(" Raw strings table:");
    for (size_t i = 0; i < PCP_BUF_MAX; i = i + 32) {
        printf("  ");
        for (size_t j = i; j < i + 32; j++) {
            printf("%02X ", stream->recv_data.strings[j]);
            if (j % 16 == 15)
                printf("  ");
            else if (j % 8 == 7)
                printf(" ");
        }
        printf("  ");
        for (size_t j = i; j < i + 32; j++) {
            char chr = stream->recv_data.strings[j];
            if (' ' <= chr && chr <= '~')
                printf("%c", chr);
            else
                printf(".");
            if (j % 16 == 15) printf(" ");
        }
        puts("");
    }
}
