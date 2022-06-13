#include <Windows.h>

#include "../lib/mice/mice.h"
#include "mx.h"
#include "smbus.h"
#include "w83791d.h"

BYTE w83791d_bank = 0x00;
BYTE w83791d_config = W83791D_CONFIG_START;
BYTE w83791d_noncrit_t1 = 75;  // C
BYTE w83791d_crit_t1 = 80;     // C
BYTE w83791d_noncrit_t2 = 75;  // C
BYTE w83791d_crit_t2 = 80;     // C
BYTE w83791d_noncrit_t3 = 75;  // C
BYTE w83791d_crit_t3 = 80;     // C
BYTE w83791d_vbat_monitor_control = 0x00;
BOOL w83791d_4f_high = 0;

BOOL mxsuperio_DeviceIoControl(DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer,
                               DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    mxsuperio_lpc_packet* lpc_packet = (mxsuperio_lpc_packet*)lpInBuffer;
    mxsuperio_lpc_packet* lpc_out = (mxsuperio_lpc_packet*)lpOutBuffer;

    switch (dwIoControlCode) {
        case IOCTL_MXSUPERIO_PING:
            log_misc("mxsuperio",
                     "DeviceIoControl(<mxsuperio>, <ping>, 0x%p, 0x%x, -, "
                     "0x%x, -, -)",
                     lpInBuffer, nInBufferSize, nOutBufferSize);

            ((LPDWORD)lpOutBuffer)[0] = 0x01000001;
            if (lpBytesReturned) *lpBytesReturned = 4;
            break;
        case IOCTL_MXSUPERIO_READ:
            log_warning("mxsuperio",
                        "DeviceIoControl(<mxsuperio>, <read>, 0x%p, 0x%x, -, "
                        "0x%x, -, -)",
                        lpInBuffer, nInBufferSize, nOutBufferSize);

            /*
                    BYTE chipId = 0|1
                    BYTE ??     = 0x0b
                    BYTE ??     = 0x20|0x21
                    BYTE ret	= 0
            */

            switch (((LPBYTE)lpInBuffer)[2]) {
                case 0x20:
                    ((LPBYTE)lpInBuffer)[3] = 0xa0;
                    break;
                case 0x21:
                    ((LPBYTE)lpInBuffer)[3] = 0x20;
                    break;
                default:
                    ((LPBYTE)lpInBuffer)[3] = 0x00;
                    break;
            }

            if (lpBytesReturned) *lpBytesReturned = 4;
            break;
        case IOCTL_MXSUPERIO_HWMONITOR_LPC_READ: {
            BYTE index = lpc_packet->index;
            BYTE reg = lpc_packet->reg;

            switch (w83791d_bank) {
                case 0x00: {
                    switch (reg) {
                        case W83791D_REG_WCHIPID:
                            lpc_out->data = 0x71;
                            break;
                        case W83791D_REG_CHIPMAN:
                            if (w83791d_4f_high)
                                lpc_out->data = 0x5c;  // High byte
                            else
                                lpc_out->data = 0xa3;  // Low byte
                            break;
                        case W83791D_REG_I2C_ADDR:
                            lpc_out->data = 0x11;
                            break;
                        case W83791D_REG_BANK:
                            lpc_out->data = w83791d_bank;
                            break;

                        case W83791D_REG_CONFIG:
                            lpc_out->data = w83791d_config;
                            break;

                        case W83791D_REG_BEEP_CTRL_0:
                            lpc_out->data = 0xff;  // All the beeps! (see main.h)
                            break;

                        case W83791D_REG_TEMP1_0:
                            lpc_out->data = 0x00;  // TODO: Figure out the temp val lol
                            break;
                        case W83791D_RAM_VCOREA:
                        case W83791D_RAM_VNIR0:
                        case W83791D_RAM_12VIN:
                            lpc_out->data = 0x00;  // TODO: Figure out the batt val
                            break;

                        case W83791D_RAM_FAN1:
                        case W83791D_RAM_FAN2:
                        case W83791D_RAM_FAN3:
                            lpc_out->data = 0x00;  // Fan no spinny!
                            break;

                        case W83791D_VID_FAN_DIV:
                            // Boths fans divide by 2 (01), VDD 5 latched
                            // good.
                            lpc_out->data = 0b01011111;  // Let's not /0!
                            break;

                        case W83791D_VBAT_MONITOR_CONTROL:
                            lpc_out->data = w83791d_vbat_monitor_control;
                            break;

                        default:
                            log_error("mxsuperio", "Unknown HM b0 register: 0x%02x", reg);
                            exit(1);
                    }
                } break;
                case 0x01: {
                    switch (reg) {
                        case W83791D_REG_BANK:
                            lpc_out->data = w83791d_bank;
                            break;

                        case W83791D_NONCRIT_TEMP_1:
                            lpc_out->data = w83791d_noncrit_t1;
                            break;
                        case W83791D_CRIT_TEMP_1:
                            lpc_out->data = w83791d_crit_t1;
                            break;
                        case W83791D_NONCRIT_TEMP_2:
                            lpc_out->data = w83791d_noncrit_t2;
                            break;
                        case W83791D_CRIT_TEMP_2:
                            lpc_out->data = w83791d_crit_t2;
                            break;
                        case W83791D_NONCRIT_TEMP_3:
                            lpc_out->data = w83791d_noncrit_t3;
                            break;
                        case W83791D_CRIT_TEMP_3:
                            lpc_out->data = w83791d_crit_t3;
                            break;

                        case W83791D_VIN0:
                            lpc_out->data = W83791D_ENTITY_CPU;
                            break;
                        case W83791D_VIN1:
                            lpc_out->data = W83791D_ENTITY_SYSTEM;
                            break;

                        default:
                            log_error("mxsuperio", "Unknown HM b1 register: 0x%02x", reg);
                            exit(1);
                    }
                } break;
                case 0x05: {
                    switch (reg) {
                        case W83791D_REG_BANK:
                            lpc_out->data = w83791d_bank;
                            break;

                        // Used in amHmReadVoltageInternal
                        // What are these??
                        case 0x51:
                        case 0x40:
                            lpc_out->data = 0xff;
                            break;

                        default:
                            log_error("mxsuperio", "Unknown HM b5 register: 0x%02x", reg);
                            return FALSE;
                            // exit(1);
                    }
                } break;
                default:
                    log_error("mxsuperio", "Unknown HM bank: 0x%02x", w83791d_bank);
                    exit(1);
            }

            // reg 0x48 = LPC Chip ID
            //   index = 0,1
            //   data > 0x0f, data < 0x70

            // lpc_out->data = eeprom_read_one(reg, index);
            log_misc("mxsuperio", "amHmLpcReadByte Index=0x%02x Reg=0x%02x Data=0x%02x", index, reg, lpc_out->data);

            if (lpBytesReturned) *lpBytesReturned = sizeof(mxsuperio_lpc_packet);
        } break;
        case IOCTL_MXSUPERIO_HWMONITOR_LPC_WRITE: {
            BYTE index = lpc_packet->index;
            BYTE reg = lpc_packet->reg;
            log_misc("mxsuperio", "amHmLpcWriteByte Index=0x%02x Reg=0x%02x Data=0x%02b", index, reg, lpc_packet->data);

            switch (w83791d_bank) {
                case 0x00: {
                    switch (reg) {
                        case W83791D_REG_BEEP_CTRL_0:
                            // Ignore for now
                            break;

                        case W83791D_REG_BANK:
                            w83791d_4f_high = !!(lpc_packet->data & 0x80);
                            w83791d_bank = lpc_packet->data & 7;
                            break;
                        case W83791D_REG_CONFIG:
                            w83791d_config = lpc_packet->data;
                            break;
                        case W83791D_VBAT_MONITOR_CONTROL:
                            w83791d_vbat_monitor_control = lpc_packet->data;
                            break;
                        default:
                            log_error("mxsuperio", "Unknown HM b0 register: 0x%02x", reg);
                            exit(1);
                    }
                    break;
                }
                case 0x01: {
                    switch (reg) {
                        case W83791D_REG_BANK:
                            w83791d_4f_high = !!(lpc_packet->data & 0x80);
                            w83791d_bank = lpc_packet->data & 7;
                            break;

                        case W83791D_NONCRIT_TEMP_1:
                            w83791d_noncrit_t1 = lpc_packet->data;
                            break;
                        case W83791D_CRIT_TEMP_1:
                            w83791d_crit_t1 = lpc_packet->data;
                            break;
                        case W83791D_NONCRIT_TEMP_2:
                            w83791d_noncrit_t2 = lpc_packet->data;
                            break;
                        case W83791D_CRIT_TEMP_2:
                            w83791d_crit_t2 = lpc_packet->data;
                            break;
                        case W83791D_NONCRIT_TEMP_3:
                            w83791d_noncrit_t3 = lpc_packet->data;
                            break;
                        case W83791D_CRIT_TEMP_3:
                            w83791d_crit_t3 = lpc_packet->data;
                            break;

                        default:
                            log_error("mxsuperio", "Unknown HM b1 register: 0x%02x", reg);
                            exit(1);
                    }
                } break;
                case 0x05: {
                    switch (reg) {
                        case W83791D_REG_BANK:
                            w83791d_4f_high = !!(lpc_packet->data & 0x80);
                            w83791d_bank = lpc_packet->data & 7;
                            break;

                        // Used in amHmReadVoltageInternal
                        // What is this??
                        case 0x40:
                            break;

                        default:
                            log_error("mxsuperio", "Unknown HM b5 register: 0x%02x", reg);
                            return FALSE;
                            // exit(1);
                    }
                } break;
                default:
                    log_error("mxsuperio", "Unknown HM bank: 0x%02x", w83791d_bank);
                    exit(1);
            }

            if (lpBytesReturned) *lpBytesReturned = 0;
        } break;
        default:
            log_warning("mxsuperio", "unhandled 0x%08x", dwIoControlCode);
            return FALSE;
    }

    return TRUE;
}

void setup_mxsuperio() {
    file_hook_t* mxsuperio = new_file_hook(L"\\\\.\\mxsuperio");
    mxsuperio->DeviceIoControl = &mxsuperio_DeviceIoControl;

    hook_file(mxsuperio);
}
