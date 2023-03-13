#pragma once

#define PCP_LOG(fmt, ...) printf("%s:%d:" fmt, __func__, __LINE__, ##__VA_ARGS__)
#define ZERO(x) memset(&(x), 0, sizeof(x))
#define ZERO_BUF(x) memset((x), 0, sizeof(*x))

#include <Winsock2.h>
#include <ws2ipdef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "../ami/ami.h"

#define PCP_CHAR_PROMPT '>'
#define PCP_CHAR_BINACK '$'
#define PCP_CHAR_HASH '#'
#define PCP_CHAR_QMARK '?'
#define PCP_CHAR_SEP '&'
#define PCP_CHAR_EQU '='
#define PCP_CHAR_CR '\r'
#define PCP_CHAR_LF '\n'
#define PCP_CHAR_EOF '\0'
#define PCP_WHITESPACE(x) ((x) == ' ' || (x) == '\t')

extern char* LIBPCP_VERSION;

typedef unsigned int undefined4;
typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned int uint;

typedef unsigned int timeout_t;

#define PCP_BUF_MAX 256
#define PCP_CMDS_MAX 64
#define PCP_KEYWORD_MAX 32
#define PCP_SEND_BUF_MAX 8

typedef struct pcp_parse_data {
    char strings[PCP_BUF_MAX];
    byte keywords[PCP_CMDS_MAX];
    byte values[PCP_CMDS_MAX];
    uint cmd_count;
} pcp_parse_data_t;
typedef struct pcp_send_data {
    unsigned char data[PCP_BUF_MAX];
    size_t length;
} pcp_send_data_t;

// Errors
#define OPEN_MODE_GLOBAL 0
#define OPEN_MODE_LOCAL 1
// #define OPEN_MODE_2 2

typedef enum binary_mode {
    binary_mode_none = 0,
    binary_mode_send = 1,
    binary_mode_recv = 2,
} binary_mode_t;

#define PCPT_CONFIG_0 0
#define PCPT_CONFIG_1 1
#define PCPT_SO_LINGER 2
#define PCPT_TCP_NODELAY 3

#define SOCKET_INVAL ((SOCKET)-1)
#define HANDLE_INVAL -1
#define TIMEOUT_NONE ((timeout_t)-1)

#define PCPT_CLOSED 0
#define PCPT_LISTENING 1
#define PCPT_CONNECTED 2
