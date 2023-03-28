#include "../hooks/gui.h"
#include "_devices.h"

#define SERVO_OK 0
#define SERVO_NG 8

static DWORD WINAPI ser_servo_15069_thread(com_device_t* dev) {
    log_game(plfServo15069, "%ls woke up", dev->com->wName);

    unsigned char data[4];
    unsigned char response;

    while (1) {
        comdev_read_blocking(dev, data, 4);

        // Validate checksums!
        if (!(data[0] & 0x80) || (data[3] & 0x80) ||
            (data[0] & 0x7f) != (data[1] ^ data[2] ^ data[3])) {
            log_error(plfServo15069, "checksums failed: %02x %02x %02x %02x", data[0], data[1],
                      data[2], data[3]);
            continue;
        }

        log_game(plfServo15069, "read:%02x %02x %02x %02x", data[0], data[1], data[2], data[3]);

        /**
         * Read 4 bytes:
         *
         * [0]=v1 | 0x80
         * [1]=v2
         * [2]=v3
         * [3]=(v1^v2^v3) & 0x7f
         *
         * v1&0x7f=[3]^v2^v3
         * v2=[1]
         * v3=[2]
         *
         * High byte v1 unrecoverable!
         */

        /**
         * Wake sequence in test mode:
         *
         * 0x7f: 0, 0
         * 0x01, 64, 16
         * 0x02, 4, 84
         *
         */

        switch (data[0] & 0x7f) {
            case 0x7f:
                response = SERVO_OK;
                comdev_write(dev, &response, 1);
                break;
            case 1:
            case 2:
                log_warning(plfServo15069, "Not sure what %d is!", data[0] & 0x7f);
                response = (data[0] & 7) | SERVO_OK;
                comdev_write(dev, &response, 1);
                break;

            default:
                log_error(plfServo15069, "Unknown opcode: %02x", data[0] & 0x7f);
                response = SERVO_NG;
                comdev_write(dev, &response, 1);
                break;
        }

        /**
         * Respond one byte, 8=high:
         * [0] \
         * [1] => data
         * [2] /
         * [3] = status
         * [4] \
         * [5] => data again
         * [6] /
         * [7] = X
         */
    }
}
void install_servo_15069() { register_device("servo_15069", ser_servo_15069_thread); }
