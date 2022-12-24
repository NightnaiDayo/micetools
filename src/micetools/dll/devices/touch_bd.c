#include "_devices.h"

static BYTE read_one(com_device_t* dev) {
    while (!comdev_available(dev)) Sleep(50);
    BYTE data;
    comdev_read(dev, &data, 1);
    return data;
}

const BYTE TOUCH_ID_LUT[] = "ABCD\0EFGH\0IJKL\0MNOPQRSTU\0VWXY\0";
static BYTE get_touch_id(BYTE id) {
    for (BYTE i = 0; i < sizeof(TOUCH_ID_LUT); i++) {
        if (TOUCH_ID_LUT[i] == id) return i;
    }
    return 0xff;
}

BOOL touch_is_enabled = false;
BYTE thresh = 0x00;  // Lazy caching of single value
DWORD WINAPI touch_bd_thread(com_device_t* dev) {
    log_info("touch_bd", "%ls woke up", dev->com->wName);

    while (1) {
        if (touch_is_enabled && !comdev_available(dev)) {
            // Active mode!
            comdev_write(dev, (unsigned char*)"(@@@@@@@@@@@@)", 14);
            Sleep(100);
            continue;
        }

        while (read_one(dev) != '{') continue;
        while (comdev_available(dev) < 5) {
            log_info("touch", "<. .>");
            Sleep(50);
        }
        BYTE command[5];
        comdev_read(dev, command, 5);
        BYTE response[6];
        memcpy(response, "(    )", 6);

        if (memcmp(command, "HALT}", 5) == 0) {
            if (touch_is_enabled)
                log_info("touch", "Touchscreen left active mode");
            else
                log_misc("touch", "Touchscreen not in active mode");
            touch_is_enabled = false;
        } else if (memcmp(command, "STAT}", 5) == 0) {
            if (!touch_is_enabled)
                log_info("touch", "Touchscreen entered active mode");
            else
                log_misc("touch", "Touchscreen already in active mode");
            touch_is_enabled = true;
        } else if (command[2] == 'k' && command[4] == '}') {
            BYTE sensor = get_touch_id(command[1]);

            log_misc("touch", "k-command recieved: %d >=%d", sensor, command[3]);
            // Sensor == '@': failed
            // ( <L/R> <sensor> <> <> )
            response[1] = command[0];
            response[2] = command[1];
            thresh = command[3];

            comdev_write(dev, response, 6);
        } else if (command[2] == 't' && command[3] == 'h' && command[4] == '}') {
            BYTE sensor = get_touch_id(command[1]);

            // { <L/R> <sensor> t h }
            log_misc("touch", "th-command recieved: %d", sensor);

            // Sensor == '@': failed
            // ( <L/R> <sensor> <> <threshold> )
            response[1] = command[0];  // 'L' or 'R'
            response[2] = command[1];  // Sensor
            response[4] = thresh;

            comdev_write(dev, response, 6);
        } else {
            log_error("touch", "Unhandled: {%.*s", 5, command);
        }
    }
}

void install_touch_bd() {
    char* text = MiceConfig.devices.touch_bd;
    char* copy = (char*)malloc(strlen(text) + 1);
    memcpy_s(copy, strlen(text) + 1, text, strlen(text) + 1);

    char* next_token;
    char* token = strtok_s(copy, ",", &next_token);
    while (token != NULL) {
        BYTE com_port = atoi(token) & 0xFF;
        if (com_port) com_device_thread(new_com_device(com_port), touch_bd_thread);
        token = strtok_s(NULL, ",", &next_token);
    }

    free(copy);
}
