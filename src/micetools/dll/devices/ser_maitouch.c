#include "../hooks/gui.h"
#include "_devices.h"

static BYTE read_one(com_device_t* dev) {}

const BYTE TOUCH_ID_LUT[] = "ABCD\0EFGH\0IJKL\0MNOPQRSTU\0VWXY\0";
static BYTE get_touch_id(BYTE id) {
    for (BYTE i = 0; i < sizeof(TOUCH_ID_LUT); i++) {
        if (TOUCH_ID_LUT[i] == id) return i;
    }
    return 0xff;
}

// High byte: index, Low byte: bitflag
#define BUTTON_A1 0x0101
#define BUTTON_B1 0x0102
#define BUTTON_A2 0x0104
#define BUTTON_B2 0x0108
#define BUTTON_A3 0x0201
#define BUTTON_B3 0x0202
#define BUTTON_A4 0x0204
#define BUTTON_B4 0x0208
#define BUTTON_A5 0x0301
#define BUTTON_B5 0x0302
#define BUTTON_A6 0x0304
#define BUTTON_B6 0x0308
#define BUTTON_A7 0x0401
#define BUTTON_B7 0x0402
#define BUTTON_A8 0x0404
#define BUTTON_B8 0x0408
#define BUTTON_C 0x0410

WORD getSensorInRegion(float mX, float mY, float rX, float rY, float rW, float rH) {
    // Out of region
    if (mX < rX || mY < rY || mX > rX + rW || mY > rY + rH) return 0;

    // i is now a position in a region from (-1,-1) to (1,1)
    float iX = ((mX - rX) / (rW / 2)) - 1;
    float iY = ((mY - rY) / (rH / 2)) - 1;

    float r2 = iX * iX + iY * iY;

    // Bound to a unit circle
    if (r2 > 1) return 0;

    // Centre button
    if (r2 < (121.0f / 540.0f) * (121.0f / 540.0f)) return BUTTON_C;

    WORD button = 0;

    // Four quadrants
    if (iX > 0 && iY < 0) button = -iY > iX ? BUTTON_A1 : BUTTON_A2;
    if (iX > 0 && iY > 0) button = iX > iY ? BUTTON_A3 : BUTTON_A4;
    if (iX < 0 && iY > 0) button = iY > -iX ? BUTTON_A5 : BUTTON_A6;
    if (iX < 0 && iY < 0) button = -iX > -iY ? BUTTON_A7 : BUTTON_A8;

    // Inner ring of buttons
    if (r2 < (298.0f / 540.0f) * (298.0f / 540.0f))
        button = (button & 0xff00) | ((button & 0xff) << 1);

    return button;
}

static BYTE g_ActiveResponse[14];
static inline void handleOneButton(int iW, int iH, int iX, int iY) {
    float x = (float)iX / (float)(iW);
    float y = (float)iY / (float)(iH);

    // If the window is square, assume `1P_ONLY` is set
    WORD button;
    if (iW == iH) {
        button = getSensorInRegion(x, y, 0.0f, 0.0f, 1.0f, 1.0f);
        if (button) g_ActiveResponse[button >> 8] |= button & 0xff;
    } else {
        // 1P
        button = getSensorInRegion(x, y, 0.0f, 0.4375f, 0.5f, 0.5625f);
        if (button) g_ActiveResponse[button >> 8] |= button & 0xff;
        // 2P
        button = getSensorInRegion(x, y, 0.5f, 0.4375f, 0.5f, 0.5625f);
        if (button) g_ActiveResponse[(button >> 8) + 6] |= button & 0xff;
    }
}
static void populateActiveResponse() {
    // Placeholders
    g_ActiveResponse[1] = '@';
    g_ActiveResponse[2] = '@';
    g_ActiveResponse[3] = '@';
    g_ActiveResponse[4] = '@';
    //
    g_ActiveResponse[7] = '@';
    g_ActiveResponse[8] = '@';
    g_ActiveResponse[9] = '@';
    g_ActiveResponse[10] = '@';

    if (!mainWindow) return;
    RECT winRect;
    if (!GetWindowRect(mainWindow, &winRect)) return;
    DWORD dwStyle = GetWindowLongW(mainWindow, GWL_STYLE);
    UnadjustWindowRect(&winRect, dwStyle, FALSE);
    int w = winRect.right - winRect.left;
    int h = winRect.bottom - winRect.top;

    if (GetAsyncKeyState(VK_LBUTTON)) {
        POINT pCursor;
        GetCursorPos(&pCursor);
        if (ScreenToClient(mainWindow, &pCursor)) {
            handleOneButton(w, h, pCursor.x, pCursor.y);
        }
    }

    for (DWORD i = 0; i < nActiveTouches; i++) {
        handleOneButton(w, h, activeTouches[i].m_X, activeTouches[i].m_Y);
    }
}

BOOL touch_is_enabled = TRUE;  // Default on is important!
BYTE thresh = 0x00;            // Lazy caching of single value
DWORD WINAPI touch_bd_thread(com_device_t* dev) {
    log_info(plfMaiTouch, "%ls woke up", dev->com->wName);

    // Constant values
    g_ActiveResponse[0] = '(';
    g_ActiveResponse[5] = '@';
    g_ActiveResponse[6] = '@';
    g_ActiveResponse[11] = '@';
    g_ActiveResponse[12] = '@';
    g_ActiveResponse[13] = ')';

    while (1) {
        if (touch_is_enabled && !comdev_available(dev)) {
            populateActiveResponse();
            comdev_write(dev, g_ActiveResponse, 14);
            Sleep(2);
            continue;
        }

        BYTE startWait;
        do {
            comdev_read_blocking(dev, &startWait, 1);
        } while (startWait != '{');

        BYTE command[5];
        comdev_read_blocking(dev, command, 5);
        BYTE response[6];
        memcpy(response, "(    )", 6);

        if (memcmp(command, "HALT}", 5) == 0) {
            if (touch_is_enabled)
                log_info(plfMaiTouch, "Touchscreen left active mode");
            else
                log_misc(plfMaiTouch, "Touchscreen not in active mode");
            touch_is_enabled = false;
        } else if (memcmp(command, "STAT}", 5) == 0) {
            if (!touch_is_enabled)
                log_info(plfMaiTouch, "Touchscreen entered active mode");
            else
                log_misc(plfMaiTouch, "Touchscreen already in active mode");
            touch_is_enabled = true;
        } else if (command[2] == 'k' && command[4] == '}') {
            BYTE sensor = get_touch_id(command[1]);

            log_misc(plfMaiTouch, "k-command recieved: %d >=%d", sensor, command[3]);
            // Sensor == '@': failed
            // ( <L/R> <sensor> <> <> )
            response[1] = command[0];
            response[2] = command[1];
            thresh = command[3];

            comdev_write(dev, response, 6);
        } else if (command[2] == 't' && command[3] == 'h' && command[4] == '}') {
            BYTE sensor = get_touch_id(command[1]);

            // { <L/R> <sensor> t h }
            log_misc(plfMaiTouch, "th-command recieved: %d", sensor);

            // Sensor == '@': failed
            // ( <L/R> <sensor> <> <threshold> )
            response[1] = command[0];  // 'L' or 'R'
            response[2] = command[1];  // Sensor
            response[4] = thresh;

            comdev_write(dev, response, 6);
        } else {
            log_error(plfMaiTouch, "Unhandled: {%.*s", 5, command);
        }
    }
}

void install_touch_bd() { register_device("maitouch", touch_bd_thread); }
