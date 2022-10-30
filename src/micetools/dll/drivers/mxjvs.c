#include <Windows.h>

#include "../common.h"
#include "jvs.h"
#include "mx.h"

BOOL JVS_SENSE = false;
BOOL coin_solenoid = false;
BOOL test_btn = false;

// #define SCAN_COIN 0x70
#define SCAN_TEST VK_OEM_4  // [{
// 2 Players, 16 buttons each
int jvs_buttons[2][16] = {
    {
        'C',  // *1P3
        'P',  // NC
        'E',  // *1P1
        'D',  // *1P2
        'P',  // NC
        'P',  // NC
        '1',  // *1P Service
        'P',  // NC
        'P',  // --
        'P',  // --
        'P',  // --
        'W',  // *1P8
        'Q',  // *1P7
        'A',  // *1P6
        'Z',  // *1P5
        'X',  // *1P4
    },
    {
        VK_OEM_PERIOD,  // *2P3
        'P',            // NC
        'O',            // *2P1
        'L',            // *2P2
        'P',            // NC
        'P',            // NC
        '2',            // *2P Service
        'P',            // NC
        'P',            // --
        'P',            // --
        'P',            // --
        'I',            // *2P8
        'U',            // *2P7
        'J',            // *2P6
        'M',            // *2P5
        VK_OEM_COMMA,   // *2P4
    },
};

short jvs_unpad(unsigned char* paddedData, short length, unsigned char* unpaddedData) {
    short index = 0;
    bool escape = false;
    for (short i = 0; i < length; i++) {
        if (escape)
            unpaddedData[index++] = paddedData[i] + 1;
        else if (paddedData[i] == JVS_MARK)
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

const char JVS_ID[] = "SEGA ENTERPRISES,LTD.;I/O BD JVS;837-13551 ;Ver1.00;98/10";

void mxjvs_exchange(unsigned char* paddedIn, short inCount, unsigned char* outData, short maxOut,
                    int* outCount) {
    unsigned char* inData = malloc(inCount);
    inCount = jvs_unpad(paddedIn, inCount, inData);
    unsigned char* response = malloc(maxOut);

    unsigned char status = JVS_STATUS_OK;

    // JVS frame is 4 bytes in total
    if (inCount < 4) {
        log_error("mxjvs", "inCount impossibly small: %d", inCount);
        status = JVS_STATUS_UNKNOWN;
        goto jvs_exchange_error;
    }
    // This isn't a JVS packet
    if (inData[0] != JVS_SYNC) {
        log_error("mxjvs", "SYNC missing. Saw 0x%02x", inData[0]);
        status = JVS_STATUS_UNKNOWN;
        goto jvs_exchange_error;
    }

    // Validate the checksum before proceeding
    unsigned char sum = 0;
    for (int i = 1; i < inCount - 1; i++) sum += inData[i];
    if (sum != inData[inCount - 1]) {
        log_error("mxjvs", "Checksum failed. Computed 0x%02x, expected 0x%02x", sum,
                  inData[inCount - 1]);
        status = JVS_STATUS_SUM;
        goto jvs_exchange_error;
    }

    unsigned char destination = inData[1];
    unsigned char length = inData[2];
    // length 0 is nonsensical because there's a checksum!
    if (length == 0) {
        status = JVS_STATUS_SUM;
        goto jvs_exchange_error;
    }
    short jvsIndex = 3;  // D0

    response[0] = JVS_SYNC;
    response[1] = JVS_NODE_MASTER;
    short respIndex = 4;  // D0
#define jvs_read(x)                   \
    if (jvsIndex - 2 >= length) {     \
        status = JVS_STATUS_OVERFLOW; \
        goto jvs_exchange_error;      \
    } else {                          \
        (x) = inData[jvsIndex++];     \
    }
#define jvs_write(x) response[respIndex++] = (x);

    while (jvsIndex - 2 < length) {
        unsigned char cmd;
        jvs_read(cmd);
        // log_info("jvs", "jvs cmd: %02x", cmd);
        switch (cmd) {
            case JVS_CMD_RESET:
                unsigned char reset_assert;
                jvs_read(reset_assert);
                if (reset_assert != JVS_CMD_RESET_ASSERT) {
                    status = JVS_STATUS_UKCOM;
                    goto jvs_exchange_error;
                }
                JVS_SENSE = true;
                // Special case
                *outCount = 0;
                return;
            case JVS_CMD_CHANGE_COMMS:
                // Special case
                *outCount = 0;
                return;

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
                for (int i = 0; i < sizeof JVS_ID; i++) jvs_write(JVS_ID[i]);
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
                jvs_write(2);
                jvs_write(13);
                jvs_write(JVS_FEATURE_PAD);
                jvs_write(JVS_FEATURE_COINS);
                jvs_write(2);
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
                if (players > 2 || switch_bytes != 2) {
                    jvs_write(JVS_REPORT_PARAM_INVALID);
                    break;
                }

                jvs_write(JVS_REPORT_OK);
                unsigned char buttons = 0x00;
                if (GetAsyncKeyState(SCAN_TEST) < 0) buttons |= 0x80;
                jvs_write(buttons);

                for (int i = 0; i < players; i++) {
                    for (int j = 0; j < switch_bytes; j++) {
                        buttons = 0x00;
                        for (int bit = 0; bit < 8; bit++) {
                            int scancode = jvs_buttons[i][j * 8 + bit];

                            // Buttons on maimai use beam interrupt sensors, so logical high =
                            // unpressed.
                            bool invert = ((j == 0 && (bit == 0 || bit == 2 || bit == 3)) ||
                                           (j == 1 && bit >= 3));

                            if (invert)
                                buttons |= (GetAsyncKeyState(scancode) >= 0) << bit;
                            else
                                buttons |= (GetAsyncKeyState(scancode) < 0) << bit;

                            // JAMMA does **NOT** do any of this lol
                            // Service is pull-down for some reason
                            // bool pulse = (j == 0) && (bit == 6);
                            // // JAMMA uses the falling edge of a pull-up switch
                            // if (pulse) {
                            //     buttons |= (GetAsyncKeyState(scancode) >= 0) << bit;
                            //     buttons |= (!(GetAsyncKeyState(scancode) & 1)) << bit;
                            // } else {
                            //     buttons |= (GetAsyncKeyState(scancode) < 0) << bit;
                            //     buttons |= (GetAsyncKeyState(scancode) & 1) << bit;
                            // }
                        }
                        jvs_write(buttons);
                    }
                }

                break;

            case JVS_CMD_READ_COIN:
                jvs_write(JVS_REPORT_OK);
                unsigned char coin_count;
                jvs_read(coin_count);
                for (; coin_count; coin_count--) {
                    jvs_write(0x00);  // coin MSB
                    jvs_write(0x00);  // coin LSB
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

            default:
                log_error("mxjvs", "Unknown command: 0x%02x", cmd);

                status = JVS_STATUS_UKCOM;
                goto jvs_exchange_error;
        }
    }
#undef jvs_read
#undef jvs_write

    goto jvs_exchange_complete;
jvs_exchange_error:
    log_error("mxjvs", "JVS status: 0x%02x", status);
    respIndex = 4;  // As if we've just written status, but nothing else

jvs_exchange_complete:
    response[2] = (respIndex - 2) & 0xFF;  // respIndex doesn't include SUM yet
    response[3] = status;

    // Compute and set the checksum
    sum = 0;
    for (int i = 1; i < respIndex; i++) sum += response[i];
    response[respIndex++] = sum;

    short paddedLength = jvs_pad(response, respIndex, outData, maxOut);
    // We failed hard. It's not worth sending an overflow response
    if (paddedLength == -1)
        *outCount = 0;
    else
        *outCount = 0x00000000 | paddedLength;

    free(response);
    free(inData);
}

BOOL mxjvs_DeviceIoControl(void* file, DWORD dwIoControlCode, LPVOID lpInBuffer,
                           DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                           LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    switch (dwIoControlCode) {
        case IOCTL_MXJVS_EXCHANGE:
            log_trace("mxjvs",
                      "DeviceIoControl(<mxjvs>, <exchange>, 0x%p, 0x%x, -, "
                      "0x%x, -, -)",
                      lpInBuffer, nInBufferSize, nOutBufferSize);

            mxjvs_exchange(lpInBuffer, nInBufferSize & 0xffff, lpOutBuffer, nOutBufferSize & 0xFFFF,
                           (int*)lpBytesReturned);

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

void setup_mxjvs() {
    file_hook_t* mxjvs = new_file_hook(L"\\\\.\\mxjvs");
    mxjvs->DeviceIoControl = &mxjvs_DeviceIoControl;

    com_hook_t* jvscom = new_com_hook(0);
    wcscpy_s(jvscom->wName, sizeof jvscom->wName, L"\\\\.\\mxjvs");
    wcscpy_s(jvscom->wDosName, sizeof jvscom->wDosName, L"\\\\.\\mxjvs");
    jvscom->GetCommState = mxjvs_GetCommState;
    jvscom->SetCommState = mxjvs_SetCommState;
    jvscom->SetCommTimeouts = mxjvs_SetCommTimeouts;
    jvscom->SetupComm = mxjvs_SetupComm;
    jvscom->PurgeComm = mxjvs_PurgeComm;
    jvscom->GetCommModemStatus = mxjvs_GetCommModemStatus;

    hook_file(mxjvs);
    hook_com(jvscom);
}
