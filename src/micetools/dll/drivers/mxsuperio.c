#include "mxsuperio.h"

#include "mx.h"
#include "w83627uhg.h"
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

BYTE w83627uhg_fanconf = 0x01;
BYTE w83627uhg_fanconf_2 = 0x00;
BYTE w83627uhg_cpufanout_src_slct = 0x2d;
BYTE w83627uhg_sysfanout_src_slct = 0x00;
BYTE w83627uhg_systin_target = 0x00;
BYTE w83627uhg_cputin_target = 0x00;
BYTE w83627uhg_target_tolerance = 0x00;

// Configuration 0: Logical address 2E
// Configuration 1: Logical address 4E
// Neither of these values are needed because we aren't talking to a real chip!
#define LPC_CONFIGURATION 1

BYTE hwmon_read(superio_packet* packet) {
    BYTE index = packet->index;
    BYTE reg = packet->reg;

    switch (w83791d_bank) {
        case 0x00: {
            switch (reg) {
                case W83627UHG_REG_FAN_CONF:
                    return w83627uhg_fanconf;
                case W83627UHG_REG_FAN_CONF_2:
                    return w83627uhg_fanconf_2;
                case W83627UHG_REG_CPUFANOUT_TEMP_SRC_SLCT:
                    return w83627uhg_cpufanout_src_slct;
                case W83627UHG_REG_SYSFANOUT_TEMP_SRC_SLCT:
                    return w83627uhg_sysfanout_src_slct;
                case W83627UHG_REG_SYSTIN_TARGET_TEMP:
                    return w83627uhg_systin_target;
                case W83627UHG_REG_CPUTIN_TARGET_TEMP:
                    return w83627uhg_cputin_target;
                case W83627UHG_REG_TARGET_TEMP_TOLERANCE:
                    return w83627uhg_target_tolerance;

                case W83791D_REG_WCHIPID:
                    return 0x71;  // Observed: c1
                case W83791D_REG_CHIPMAN:
                    if (w83791d_4f_high)
                        return 0x5c;  // High byte
                    else
                        return 0xa3;  // Low byte
                case W83791D_REG_I2C_ADDR:
                    return 0x11;  // Observed: 2d
                case W83791D_REG_BANK:
                    return w83791d_bank;

                case W83791D_REG_CONFIG:
                    return w83791d_config;

                case W83791D_REG_BEEP_CTRL_0:
                    // No beeping :(
                    return 0x00;

                // TODO: No idea how to read any of these. Pulled from real system
                case W83791D_REG_TEMP1_0:
                    return 26;  // Chasis temp
                case W83791D_RAM_VCOREA:
                    return 0x76;
                case W83791D_RAM_VNIR0:
                    return 0x86;
                case W83791D_RAM_VDD5:
                    return 0x94;
                case W83791D_RAM_12VIN:
                    return 0xbe;
                case W83791D_RAM_N12VIN:
                    return 0xd1;

                case W83791D_RAM_FAN1:
                case W83791D_RAM_FAN2:
                case W83791D_RAM_FAN3:
                    return 0x00;  // Fan no spinny!

                case W83791D_VID_FAN_DIV:
                    // Boths fans divide by 2 (01)
                    return 0b00000101;

                case W83791D_VBAT_MONITOR_CONTROL:
                    return w83791d_vbat_monitor_control;

                default:
                    log_error(plfMxSuperio, "Unknown HM b0 register: read 0x%02x", reg);
                    return 0xff;
            }
        } break;
        case 0x01: {
            switch (reg) {
                case W83791D_REG_BANK:
                    return w83791d_bank;

                case W83791D_NONCRIT_TEMP_1:
                    return w83791d_noncrit_t1;
                case W83791D_CRIT_TEMP_1:
                    return w83791d_crit_t1;
                case W83791D_NONCRIT_TEMP_2:
                    return w83791d_noncrit_t2;
                case W83791D_CRIT_TEMP_2:
                    return w83791d_crit_t2;
                case W83791D_NONCRIT_TEMP_3:
                    return w83791d_noncrit_t3;
                case W83791D_CRIT_TEMP_3:
                    return w83791d_crit_t3;

                // TODO: Figure out what's going on with this discrepency!
                case 0x50:
                    return 35;  // CPU temp
                // case W83791D_VIN0:
                //     return W83791D_ENTITY_CPU;
                case W83791D_VIN1:
                    return W83791D_ENTITY_SYSTEM;

                default:
                    log_error(plfMxSuperio, "Unknown HM b1 register: 0x%02x", reg);
                    exit(1);
            }
        } break;
        case 0x05: {
            switch (reg) {
                case W83791D_REG_BANK:
                    return w83791d_bank;

                // Used in amHmReadVoltageInternal
                // What are these??
                case 0x51:
                case 0x40:
                    return 0xff;

                default:
                    log_error(plfMxSuperio, "Unknown HM b5 register: 0x%02x", reg);
                    return 0xff;
            }
        } break;
        default:
            log_error(plfMxSuperio, "Unknown HM bank: 0x%02x", w83791d_bank);
            exit(1);
    }
}

void hwmon_write(superio_packet* packet) {
    switch (w83791d_bank) {
        case 0x00: {
            switch (packet->reg) {
                case W83627UHG_REG_FAN_CONF:
                    w83627uhg_fanconf = packet->data;
                    break;
                case W83627UHG_REG_FAN_CONF_2:
                    w83627uhg_fanconf_2 = packet->data;
                    break;
                case W83627UHG_REG_CPUFANOUT_VALUE_SELECT:
                    // Just no-op this
                    break;
                case W83627UHG_REG_CPUFANOUT_TEMP_SRC_SLCT:
                    w83627uhg_cpufanout_src_slct = packet->data;
                    break;
                case W83627UHG_REG_SYSFANOUT_TEMP_SRC_SLCT:
                    w83627uhg_sysfanout_src_slct = packet->data;
                    break;
                case W83627UHG_REG_SYSTIN_TARGET_TEMP:
                    w83627uhg_systin_target = packet->data;
                    break;
                case W83627UHG_REG_CPUTIN_TARGET_TEMP:
                    w83627uhg_cputin_target = packet->data;
                    break;
                case W83627UHG_REG_TARGET_TEMP_TOLERANCE:
                    w83627uhg_target_tolerance = packet->data;
                    break;
                case W83627UHG_REG_SYSFANOUT_STOP_VALUE:
                case W83627UHG_REG_SYSFANOUT_START_VALUE:
                    break;

                case W83791D_REG_BEEP_CTRL_0:
                    // Ignore for now
                    break;

                case W83791D_REG_BANK:
                    w83791d_4f_high = !!(packet->data & 0x80);
                    w83791d_bank = packet->data & 7;
                    break;
                case W83791D_REG_CONFIG:
                    w83791d_config = packet->data;
                    break;
                case W83791D_VBAT_MONITOR_CONTROL:
                    w83791d_vbat_monitor_control = packet->data;
                    break;
                default:
                    log_error(plfMxSuperio, "Unknown HM b0 register: write 0x%02x", packet->reg);
                    // exit(1);
            }
            break;
        }
        case 0x01: {
            switch (packet->reg) {
                case W83791D_REG_BANK:
                    w83791d_4f_high = !!(packet->data & 0x80);
                    w83791d_bank = packet->data & 7;
                    break;

                case W83791D_NONCRIT_TEMP_1:
                    w83791d_noncrit_t1 = packet->data;
                    break;
                case W83791D_CRIT_TEMP_1:
                    w83791d_crit_t1 = packet->data;
                    break;
                case W83791D_NONCRIT_TEMP_2:
                    w83791d_noncrit_t2 = packet->data;
                    break;
                case W83791D_CRIT_TEMP_2:
                    w83791d_crit_t2 = packet->data;
                    break;
                case W83791D_NONCRIT_TEMP_3:
                    w83791d_noncrit_t3 = packet->data;
                    break;
                case W83791D_CRIT_TEMP_3:
                    w83791d_crit_t3 = packet->data;
                    break;

                default:
                    log_error(plfMxSuperio, "Unknown HM b1 register: 0x%02x", packet->reg);
                    exit(1);
            }
        } break;
        case 0x05: {
            switch (packet->reg) {
                case W83791D_REG_BANK:
                    w83791d_4f_high = !!(packet->data & 0x80);
                    w83791d_bank = packet->data & 7;
                    break;

                // Used in amHmReadVoltageInternal
                // What is this??
                case 0x40:
                    break;

                default:
                    log_error(plfMxSuperio, "Unknown HM b5 register: 0x%02x", packet->reg);
                    return;
            }
        } break;
        default:
            log_error(plfMxSuperio, "Unknown HM bank: 0x%02x", w83791d_bank);
            exit(1);
    }
}

BOOL WINAPI mxsuperio_DeviceIoControl(file_context_t* ctx, DWORD dwIoControlCode, LPVOID lpInBuffer,
                               DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                               LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    superio_packet* lpc_packet = (superio_packet*)lpInBuffer;
    superio_packet* lpc_out = (superio_packet*)lpOutBuffer;

    switch (dwIoControlCode) {
        case IOCTL_MXSUPERIO_PING: {
            ((LPDWORD)lpOutBuffer)[0] = 0x01000001;
            if (lpBytesReturned) *lpBytesReturned = 4;
            break;
        }
        case IOCTL_MXSUPERIO_READ: {
            superio_lpc_packet* packet = (superio_lpc_packet*)(lpInBuffer);
            if (lpBytesReturned) *lpBytesReturned = sizeof *packet;
            packet->data = 0xff;

            if (packet->chip != LPC_CONFIGURATION) goto W83627UHG_processed;

            // TODO: Is just emulating the chip ID enough?

            // Global registers
            switch (packet->reg) {
                case 0x20:  // Chip ID MSB
                    packet->data = 0xa2;
                    goto W83627UHG_processed;
                case 0x21:  // Chip ID LSB
                    packet->data = 0x32;
                    goto W83627UHG_processed;
            }
            // Device specific registers
            switch (packet->device) {
                //
            }
        W83627UHG_processed:
            log_misc(plfMxSuperio, "read: chip:%d device:%d reg:%02x data:%02x", packet->chip,
                     packet->device, packet->reg, packet->data);
            break;
        }
        case IOCTL_MXSUPERIO_WRITE: {
            superio_lpc_packet* packet = (superio_lpc_packet*)(lpInBuffer);
            if (lpBytesReturned) *lpBytesReturned = sizeof *packet;

            log_misc(plfMxSuperio, "write: chip:%d device:%d reg:%02x data:%02x", packet->chip,
                     packet->device, packet->reg, packet->data);

            // TODO: Are there any writes we need to process?

            break;
        }
        case IOCTL_MXSUPERIO_HWMONITOR_LPC_READ: {
            // TODO: This code is all for W83791D, however W83627UHG is more appropriate in some
            // cases. Is this an AAL vs AAM difference? Where was the W83791D usage derived from?

            // reg 0x48 = LPC Chip ID
            //   index = 0,1
            //   data > 0x0f, data < 0x70

            if (lpc_packet->index == LPC_CONFIGURATION) {
                lpc_out->data = hwmon_read(lpc_packet);
            } else {
                lpc_out->data = 0xff;
            }
            log_misc(plfMxSuperio, "amHmLpcReadByte Index=0x%02x Reg=0x%02x Data=0x%02x",
                     lpc_packet->index, lpc_packet->reg, lpc_out->data);

            if (lpBytesReturned) *lpBytesReturned = sizeof *lpc_out;
        } break;
        case IOCTL_MXSUPERIO_HWMONITOR_LPC_WRITE: {
            log_misc(plfMxSuperio, "amHmLpcWriteByte Index=0x%02x Reg=0x%02x Data=0x%02b",
                     lpc_packet->index, lpc_packet->reg, lpc_packet->data);

            if (lpc_packet->index == LPC_CONFIGURATION) {
                hwmon_write(lpc_packet);
            }
            if (lpBytesReturned) *lpBytesReturned = 0;
        } break;
        default:
            log_warning(plfMxSuperio, "unhandled 0x%08x", dwIoControlCode);
            return FALSE;
    }

    return TRUE;
}

void setup_mxsuperio() {
    file_hook_t* mxsuperio = new_file_hook(L"\\\\.\\mxsuperio");
    mxsuperio->DeviceIoControl = &mxsuperio_DeviceIoControl;

    hook_file(mxsuperio);
}
