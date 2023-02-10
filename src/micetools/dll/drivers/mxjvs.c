#include <Windows.h>

#include "../common.h"
#include "jvs.h"
#include "mx.h"

BOOL JVS_SENSE = false;
BOOL coin_solenoid = false;
BOOL test_btn = false;

#define PLAYER_COUNT 2
#define COIN_COUNTERS 2
#define SWITCH_BYTES 2

// const char JVS_837_14572_ID[] = "SEGA ENTERPRISES,LTD.;I/O BD JVS;837-14572 ;Ver1.00;98/10";
const char JVS_837_14572_ID[] = "SEGA ENTERPRISES,LTD.;I/O BD JVS;837-13551 ;Ver1.00;98/10";

typedef struct _jvs_board {
    char test_keybind;
    char coin_keybinds[COIN_COUNTERS];

    char keybinds[PLAYER_COUNT][16];
    unsigned char flags[PLAYER_COUNT][16];

    unsigned short coin_counts[COIN_COUNTERS];

    unsigned char (*handler)(struct _jvs_board* board, unsigned char* inData, short inCount,
                             unsigned char* outData, unsigned char* outCount);
    const char* id;

    unsigned char last_sysbuttons;
    unsigned short last_buttons[PLAYER_COUNT];
} jvs_board_t;
#define JVS_FLAG_NONE 0
#define JVS_FLAG_NC 1
#define JVS_FLAG_INVERT 2

jvs_board_t (*jvs_config)[];
size_t jvs_num_boards;

short jvs_unpad(unsigned char* paddedData, short length, unsigned char* unpaddedData) {
    short index = 0;
    bool escape = false;
    for (short i = 0; i < length; i++) {
        if (escape) {
            unpaddedData[index++] = paddedData[i] + 1;
            escape = false;
        } else if (paddedData[i] == JVS_MARK)
            escape = true;
        else
            unpaddedData[index++] = paddedData[i];
    }
    return index;
}

short jvs_pad(unsigned char* unpaddedData, short length, unsigned char* paddedData,
              short paddedMax) {
    short index = 0;
    for (short i = 0; i < length; i++) {
        if (i > paddedMax) return -1;

        if (i != 0 && (unpaddedData[i] == JVS_MARK || unpaddedData[i] == JVS_SYNC)) {
            paddedData[index++] = JVS_MARK;
            paddedData[index++] = unpaddedData[i] - 1;
        } else {
            paddedData[index++] = unpaddedData[i];
        }
    }
    return index;
}

void update_jvs_buttons(jvs_board_t* board) {
    unsigned char sysButtons = 0x00;
    if (GetAsyncKeyState(board->test_keybind) < 0) sysButtons |= 0x80;
    board->last_sysbuttons = sysButtons;

    for (int player = 0; player < PLAYER_COUNT; player++) {
        unsigned short buttons = 0x0000;
        for (int i = 0; i < SWITCH_BYTES * 8; i++) {
            int scancode = board->keybinds[player][i];
            unsigned char flags = board->flags[player][i];

            if (flags & JVS_FLAG_NC) continue;
            if (flags & JVS_FLAG_INVERT)
                buttons |= (GetAsyncKeyState(scancode) >= 0) << i;
            else
                buttons |= (GetAsyncKeyState(scancode) < 0) << i;
        }
        board->last_buttons[player] = buttons;
    }
}

unsigned char jvs_exchange(jvs_board_t* board, unsigned char* inData, short inCount,
                           unsigned char* response, unsigned char* outCount) {
    short jvsIndex = 0;
    unsigned char respIndex = 0;

#define jvs_read(x)                 \
    if (jvsIndex >= inCount) {      \
        return JVS_STATUS_OVERFLOW; \
    } else {                        \
        (x) = inData[jvsIndex++];   \
    }
#define jvs_write(x) response[respIndex++] = (x);

    static bool flipflop = false;

    while (jvsIndex < inCount) {
        unsigned char cmd;
        jvs_read(cmd);

        // log_info("mxjvs", "CMD: %02x", cmd);

        switch (cmd) {
            case JVS_CMD_RESET:
                unsigned char reset_assert;
                jvs_read(reset_assert);
                if (reset_assert != JVS_CMD_RESET_ASSERT) return JVS_STATUS_UKCOM;

                JVS_SENSE = true;
                // Special case
                *outCount = 0;
                return JVS_STATUS_OK;
            case JVS_CMD_CHANGE_COMMS:
                // Special case
                *outCount = 0;
                return JVS_STATUS_OK;

            case JVS_CMD_ASSIGN_ADDR:
                jvs_write(JVS_REPORT_OK);
                JVS_SENSE = false;
                // we don't bother remembering the address because at the moment we only simulate
                // a single board in the JVS chain.
                unsigned char _;
                jvs_read(_);
                break;

            case JVS_CMD_READ_ID:
                jvs_write(JVS_REPORT_OK);
                for (size_t i = 0; i < strlen(board->id); i++) jvs_write(board->id[i]);
                break;
            case JVS_CMD_GET_CMD_VERSION:
                jvs_write(JVS_REPORT_OK);
                jvs_write(JVS_VERSION_CMD);
                break;
            case JVS_CMD_GET_JVS_VERSION:
                jvs_write(JVS_REPORT_OK);
                jvs_write(JVS_VERSION_JVS);
                break;
            case JVS_CMD_GET_COMM_VERSION:
                jvs_write(JVS_REPORT_OK);
                jvs_write(JVS_VERSION_COMM);
                break;

            case JVS_CMD_GET_FEATURES:
                jvs_write(JVS_REPORT_OK);

                jvs_write(JVS_FEATURE_PLAYERS);
                jvs_write(PLAYER_COUNT);
                jvs_write(13);  // bits per player
                jvs_write(JVS_FEATURE_PAD);
                jvs_write(JVS_FEATURE_COINS);
                jvs_write(COIN_COUNTERS);
                jvs_write(JVS_FEATURE_PAD);
                jvs_write(JVS_FEATURE_PAD);
                jvs_write(JVS_FEATURE_ANALOG);
                jvs_write(8);                // 8 ADC channels
                jvs_write(JVS_FEATURE_PAD);  // ?? (was "10" prior)
                jvs_write(JVS_FEATURE_PAD);
                jvs_write(JVS_FEATURE_GPIO);
                jvs_write(6);  // 6 ports
                jvs_write(JVS_FEATURE_PAD);
                jvs_write(JVS_FEATURE_PAD);

                jvs_write(JVS_FEATURE_EOF);

                break;
            case JVS_CMD_RECEIVE_MAIN_ID:
                unsigned char tempRead = 0xff;
                while (tempRead != 0) jvs_read(tempRead);
                // TODO: Do we need to report here?
                break;

            case JVS_CMD_READ_SW:
                unsigned char players;
                unsigned char switch_bytes;
                jvs_read(players);
                jvs_read(switch_bytes);
                if (players > PLAYER_COUNT || switch_bytes != 2) {
                    jvs_write(JVS_REPORT_PARAM_INVALID);
                    break;
                }
                update_jvs_buttons(board);

                jvs_write(JVS_REPORT_OK);
                jvs_write(board->last_sysbuttons);
                for (int player = 0; player < players; player++) {
                    for (int i = switch_bytes - 1; i >= 0; i--) {
                        jvs_write((board->last_buttons[player] >> (i * 8)) & 0xff);
                    }
                }
                break;

            case JVS_CMD_READ_COIN:
                jvs_write(JVS_REPORT_OK);
                unsigned char coin_count;
                jvs_read(coin_count);

                if (board->coin_keybinds[0] && GetAsyncKeyState(board->coin_keybinds[0]) & 1)
                    board->coin_counts[0]++;
                if (board->coin_keybinds[1] && GetAsyncKeyState(board->coin_keybinds[1]) & 1)
                    board->coin_counts[1]++;

                for (unsigned char slot = 0; slot < coin_count; slot++) {
                    jvs_write(board->coin_counts[slot] >> 8);
                    jvs_write(board->coin_counts[slot] & 0xff);
                }
                break;

            case JVS_CMD_READ_ANALOGS:
                jvs_write(JVS_REPORT_OK);
                // TODO: Actually emulate these (super low priority)
                unsigned char analog_count;
                jvs_read(analog_count);
                for (int i = analog_count; i > 0; i--) {
                    jvs_write(0xde);
                    jvs_write(0xad);
                }
                break;

            case JVS_CMD_WRITE_GPIO1:
                jvs_write(JVS_REPORT_OK);
                unsigned char gpio_bytes;
                jvs_read(gpio_bytes);
                for (int i = 0; i < gpio_bytes; i++) {
                    unsigned char gpio_value;
                    jvs_read(gpio_value);

                    if (i == 0) {
                        if (!!(gpio_value & 0x80) != coin_solenoid) {
                            coin_solenoid = !!(gpio_value & 0x80);
                            log_info("mxjvs", "Coin solenoid: %s",
                                     coin_solenoid ? "Locked" : "Unlocked");
                        }
                    }

                    // log_warning("mxjvs", "Unhandled GPIO write: *(%d) = %02x", i, gpio_value);
                }
                break;

            case JVS_CMD_COIN_DECREASE:
                jvs_write(JVS_REPORT_OK);

                unsigned char slot;
                jvs_read(slot);
                unsigned char cAmount[2];
                jvs_read(cAmount[0]);
                jvs_read(cAmount[1]);
                unsigned short sAmount = cAmount[0] << 8 | cAmount[1];
                board->coin_counts[slot] -= sAmount;
                break;

            default:
                log_error("mxjvs", "Unknown command: 0x%02x", cmd);
                return JVS_STATUS_UKCOM;
        }
    }
#undef jvs_read
#undef jvs_write

    *outCount = respIndex;
    return JVS_STATUS_OK;
}

void jvs_send_status(unsigned char status, unsigned char* outData, LPDWORD outCount) {
    short respIndex = 4;

    outData[0] = JVS_SYNC;
    outData[1] = JVS_NODE_MASTER;
    outData[2] = 2;
    if (status == JVS_MARK || status == JVS_SYNC) {
        outData[3] = JVS_MARK;
        outData[4] = status - 1;
        outData[5] = JVS_NODE_MASTER + 2 + status;
        *outCount = 6;
    } else {
        outData[3] = status;
        outData[4] = JVS_NODE_MASTER + 2 + status;
        *outCount = 5;
    }
}

void mxjvs_handle(unsigned char* paddedIn, short inCount, unsigned char* outData, short maxOut,
                  LPDWORD outCount) {
    *outCount = 0;

    unsigned char* inData = malloc(inCount);
    inCount = jvs_unpad(paddedIn, inCount, inData);

    // JVS frame is 4 bytes in total
    if (inCount < 4) {
        log_error("mxjvs", "inCount impossibly small: %d", inCount);
        jvs_send_status(JVS_STATUS_UNKNOWN, outData, outCount);
        free(inData);
        return;
    }
    // This isn't a JVS packet
    if (inData[0] != JVS_SYNC) {
        log_error("mxjvs", "SYNC missing. Saw 0x%02x", inData[0]);
        jvs_send_status(JVS_STATUS_UNKNOWN, outData, outCount);
        free(inData);
        return;
    }

    // Validate the checksum before proceeding
    unsigned char sum = 0;
    bool escape = false;
    for (int i = 1; i < inCount - 1; i++) sum += inData[i];
    if (sum != inData[inCount - 1]) {
        log_error("mxjvs", "Checksum failed. Computed 0x%02x, expected 0x%02x", sum,
                  inData[inCount - 1]);
        jvs_send_status(JVS_STATUS_SUM, outData, outCount);
        return;
    }

    unsigned char destination = inData[1];
    if (destination == 0 || destination > jvs_num_boards) return;

    unsigned char* response = malloc(maxOut);
    jvs_board_t* board = &(*jvs_config)[jvs_num_boards - destination];
    // jvs_board_t* board = &(*jvs_config)[0];
    unsigned char packetSize;
    unsigned char status =
        board->handler(board, inData + 3, inData[2] - 1, response + 4, &packetSize);
    free(inData);

    if (status == JVS_STATUS_OK) {
        if (packetSize == 0) return;

        response[0] = JVS_SYNC;
        response[1] = JVS_NODE_MASTER;
        response[2] = packetSize + 2;
        response[3] = status;

        sum = 0;
        for (int i = 1; i < packetSize + 4; i++) sum += response[i];
        response[packetSize + 4] = sum;

        short paddedLength = jvs_pad(response, packetSize + 5, outData, maxOut);
        *outCount = (paddedLength == -1) ? 0 : paddedLength;

        free(response);
    } else {
        log_error("mxjvs", "JVS board %d returned status %d", destination, status);
        free(response);
        jvs_send_status(status, outData, outCount);
    }
}

BOOL WINAPI mxjvs_DeviceIoControl(file_context_t* ctx, DWORD dwIoControlCode, LPVOID lpInBuffer,
                                  DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                                  LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    switch (dwIoControlCode) {
        case IOCTL_MXJVS_EXCHANGE:
            log_trace("mxjvs",
                      "DeviceIoControl(<mxjvs>, <exchange>, 0x%p, 0x%x, -, "
                      "0x%x, -, -)",
                      lpInBuffer, nInBufferSize, nOutBufferSize);

            mxjvs_handle(lpInBuffer, nInBufferSize & 0xffff, lpOutBuffer, nOutBufferSize & 0xFFFF,
                         lpBytesReturned);

            break;
        default:
            log_warning("mxjvs", "unhandled 0x%08x", dwIoControlCode);
            return FALSE;
    }

    return TRUE;
}

BOOL mxjvs_SetupComm(void* com, DWORD dwInQueue, DWORD dwOutQueue) { return TRUE; }
BOOL mxjvs_PurgeComm(void* com, DWORD dwFlags) { return TRUE; }
BOOL mxjvs_GetCommState(void* com, LPDCB lpDCB) { return TRUE; }
BOOL mxjvs_SetCommTimeouts(void* com, LPCOMMTIMEOUTS lpDCB) { return TRUE; }

BOOL mxjvs_GetCommModemStatus(void* com, LPDWORD lpModelStat) {
    *lpModelStat = !JVS_SENSE ? MS_DSR_ON : 0;
    return TRUE;
}

BOOL mxjvs_SetCommState(void* com, LPDCB lpDCB) {
    char PARITY[] = { 'N', 'O', 'E', 'M', 'S' };
    char* STOP[] = { "1", "1.5", "2" };
    log_info("mxjvs", "Switching to %d baud (%d%c%s)", lpDCB->BaudRate, lpDCB->ByteSize,
             PARITY[lpDCB->Parity], STOP[lpDCB->StopBits]);
    return TRUE;
}

jvs_board_t maimai_jvs_config[1] = { {
    .test_keybind = VK_OEM_4,  // [{
        .coin_keybinds = {'1', '2'},
    .keybinds = {
        {
            0,
            0,
            0,
            'W',  // *1P8
            'Q',  // *1P7
            'A',  // *1P6
            'Z',  // *1P5
            'X',  // *1P4
            'C',  // *1P3
            0,
            'E',  // *1P1
            'D',  // *1P2
            0,
            0,
            '1',  // *1P Service
            0,

        },
        {
            0,
            0,
            0,
            'I',            // *2P8
            'U',            // *2P7
            'J',            // *2P6
            'M',            // *2P5
            VK_OEM_COMMA,   // *2P4
            VK_OEM_PERIOD,  // *2P3
            0,
            'O',            // *2P1
            'L',            // *2P2
            0,
            0,
            '2',            // *2P Service
            0,

        },
    },
    .flags = {
        {
            JVS_FLAG_NC,
            JVS_FLAG_NC,
            JVS_FLAG_NC,
            JVS_FLAG_INVERT,
            JVS_FLAG_INVERT,
            JVS_FLAG_INVERT,
            JVS_FLAG_INVERT,
            JVS_FLAG_INVERT,
            JVS_FLAG_INVERT,
            JVS_FLAG_NC,
            JVS_FLAG_INVERT,
            JVS_FLAG_INVERT,
            JVS_FLAG_NC,
            JVS_FLAG_NC,
            JVS_FLAG_NONE,
            JVS_FLAG_NC,
        },
        {

            JVS_FLAG_NC,
            JVS_FLAG_NC,
            JVS_FLAG_NC,
            JVS_FLAG_INVERT,
            JVS_FLAG_INVERT,
            JVS_FLAG_INVERT,
            JVS_FLAG_INVERT,
            JVS_FLAG_INVERT,
            JVS_FLAG_INVERT,
            JVS_FLAG_NC,
            JVS_FLAG_INVERT,
            JVS_FLAG_INVERT,
            JVS_FLAG_NC,
            JVS_FLAG_NC,
            JVS_FLAG_NONE,
            JVS_FLAG_NC,
        },
    },
    .coin_counts = {0, 0},
    .handler = &jvs_exchange,
    .id = JVS_837_14572_ID,
} };

jvs_board_t under_night_jvs_config[1] = {
    {
        .test_keybind = VK_OEM_4,  // [{
        .coin_keybinds = {'1', '2'},
        .keybinds = {
            {
                0, 0, 0, 0, 0, 0,
                'A',       // D (EXCs)
                'R',       // C (Heavy)
                'E',       // B (Middle)
                'W',       // A (Light)
                VK_RIGHT,  // Right
                VK_LEFT,   // Left
                VK_DOWN,   // Down
                VK_UP,     // Up
                VK_BACK,   // Service
                VK_RETURN, // Start
            },
            {
            0, 0, 0, 0, 0, 0,
                'J',         // D (EXCs)
                'P',         // C (Heavy)
                'O',         // B (Middle)
                'I',         // A (Light)
                VK_NUMPAD6,  // Right
                VK_NUMPAD4,  // Left
                VK_NUMPAD2,  // Down
                VK_NUMPAD8,  // Up
                VK_BACK,     // Service
                VK_RETURN,   // Start
            },
        },
        .flags = {
            {
                JVS_FLAG_NC,
                JVS_FLAG_NC,
                JVS_FLAG_NC,
                JVS_FLAG_NC,
                JVS_FLAG_NC,
                JVS_FLAG_NC,
                JVS_FLAG_NONE,
                JVS_FLAG_NONE,
                JVS_FLAG_NONE,
                JVS_FLAG_NONE,
                JVS_FLAG_NONE,
                JVS_FLAG_NONE,
                JVS_FLAG_NONE,
                JVS_FLAG_NONE,
                JVS_FLAG_NONE,
                JVS_FLAG_NONE,
            },
            {
                // JVS_FLAG_NC,
                // JVS_FLAG_NC,
                // JVS_FLAG_NC,
                // JVS_FLAG_NC,
                // JVS_FLAG_NC,
                // JVS_FLAG_NC,
                // JVS_FLAG_NC,
                // JVS_FLAG_NC,
                // JVS_FLAG_NC,
                // JVS_FLAG_NC,
                // JVS_FLAG_NC,
                // JVS_FLAG_NC,
                // JVS_FLAG_NC,
                // JVS_FLAG_NC,
                // JVS_FLAG_NC,
                // JVS_FLAG_NC,
                JVS_FLAG_NC,
                JVS_FLAG_NC,
                JVS_FLAG_NC,
                JVS_FLAG_NC,
                JVS_FLAG_NC,
                JVS_FLAG_NC,
                JVS_FLAG_NONE,
                JVS_FLAG_NONE,
                JVS_FLAG_NONE,
                JVS_FLAG_NONE,
                JVS_FLAG_NONE,
                JVS_FLAG_NONE,
                JVS_FLAG_NONE,
                JVS_FLAG_NONE,
                JVS_FLAG_NONE,
                JVS_FLAG_NONE,
            },
        },
        .coin_counts = {0, 0},
        .handler = &jvs_exchange,
        .id = JVS_837_14572_ID,
    },
    // {
    //     .test_keybind = VK_OEM_4,  // [{
    //     .coin_keybinds = {0, 0},
    //     .keybinds = {
    //         {
    //             0, 0, 0, 0, 0, 0,
    //             'J',         // D (EXCs)
    //             'P',         // C (Heavy)
    //             'O',         // B (Middle)
    //             'I',         // A (Light)
    //             VK_NUMPAD6,  // Right
    //             VK_NUMPAD4,  // Left
    //             VK_NUMPAD2,  // Down
    //             VK_NUMPAD8,  // Up
    //             VK_BACK,     // Service
    //             VK_RETURN,   // Start
    //         },
    //         {0},
    //     },
    //     .flags = {
    //         {
    //             JVS_FLAG_NC,
    //             JVS_FLAG_NC,
    //             JVS_FLAG_NC,
    //             JVS_FLAG_NC,
    //             JVS_FLAG_NC,
    //             JVS_FLAG_NC,
    //             JVS_FLAG_NONE,
    //             JVS_FLAG_NONE,
    //             JVS_FLAG_NONE,
    //             JVS_FLAG_NONE,
    //             JVS_FLAG_NONE,
    //             JVS_FLAG_NONE,
    //             JVS_FLAG_NONE,
    //             JVS_FLAG_NONE,
    //             JVS_FLAG_NONE,
    //             JVS_FLAG_NONE,
    //         },
    //         {
    //             JVS_FLAG_NC,
    //             JVS_FLAG_NC,
    //             JVS_FLAG_NC,
    //             JVS_FLAG_NC,
    //             JVS_FLAG_NC,
    //             JVS_FLAG_NC,
    //             JVS_FLAG_NC,
    //             JVS_FLAG_NC,
    //             JVS_FLAG_NC,
    //             JVS_FLAG_NC,
    //             JVS_FLAG_NC,
    //             JVS_FLAG_NC,
    //             JVS_FLAG_NC,
    //             JVS_FLAG_NC,
    //             JVS_FLAG_NC,
    //             JVS_FLAG_NC,
    //         },
    //     },
    //     .coin_counts = {0, 0},
    //     .handler = &jvs_exchange,
    //     .id = JVS_837_14572_ID,
    // },
};

// jvs_board_t (*jvs_config)[] = &maimai_jvs_config;
// size_t jvs_num_boards = sizeof maimai_jvs_config / sizeof maimai_jvs_config[0];
jvs_board_t (*jvs_config)[] = &under_night_jvs_config;
size_t jvs_num_boards = sizeof under_night_jvs_config / sizeof under_night_jvs_config[0];

void setup_mxjvs() {
    file_hook_t* mxjvs = new_file_hook(L"\\\\.\\mxjvs");
    mxjvs->DeviceIoControl = &mxjvs_DeviceIoControl;
    hook_file(mxjvs);

    com_hook_t* jvscom = new_com_hook(4);
    wcscpy_s(jvscom->wName, sizeof jvscom->wName, L"\\\\.\\mxjvs");
    wcscpy_s(jvscom->wDosName, sizeof jvscom->wDosName, L"\\\\.\\mxjvs");
    jvscom->GetCommState = mxjvs_GetCommState;
    jvscom->SetCommState = mxjvs_SetCommState;
    jvscom->SetCommTimeouts = mxjvs_SetCommTimeouts;
    jvscom->SetupComm = mxjvs_SetupComm;
    jvscom->PurgeComm = mxjvs_PurgeComm;
    jvscom->GetCommModemStatus = mxjvs_GetCommModemStatus;

    mxjvs->com_hook = jvscom;

    // TODO: Looks like some things might use JVS on COM4. We should setup a comdevice for this!
    // file_hook_t* jvscom_file = new_file_hook(jvscom->wName);
    // hook_file(jvscom_file);
}
