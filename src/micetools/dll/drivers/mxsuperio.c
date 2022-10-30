#include "mx.h"
#include "smbus.h"
#include "w83791d.h"
#include "w83627uhg.h"

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

BOOL mxsuperio_DeviceIoControl(void* file, DWORD dwIoControlCode, LPVOID lpInBuffer,
                               DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                               LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
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
        case IOCTL_MXSUPERIO_READ: {
            BYTE chip = ((LPBYTE)lpInBuffer)[0];
            BYTE device = ((LPBYTE)lpInBuffer)[1];
            BYTE index = ((LPBYTE)lpInBuffer)[2];
            BYTE* data = &((LPBYTE)lpInBuffer)[3];

            log_misc("mxsuperio", "read: chip:%d device:%d index:%02x", chip, device, index);

            /*
            {
                 IN BYTE chip select (0 = base 0x2e, 1 = base 0x4e)
                 IN BYTE logical device (0 = FDC, 11 = HW mon)
                 IN BYTE index
                OUT BYTE read value
            }

            The 846-5004D I have here is at base address 0x4e
            */

            /*
             from micedump:
             0.20: 00 0b 20 ff
             0.21: 00 0b 21 ff
             1.20: 01 0b 20 a2
             1.21: 01 0b 21 32
            */

            *data = 0xff;
            if (chip == 1) {
                if (device == 11) {
                    switch (index) {
                        case 0x20:
                            *data = 0xa2;
                            break;
                        case 0x21:
                            *data = 0x32;
                            break;
                    }
                }
            }

            if (lpBytesReturned) *lpBytesReturned = 4;
            break;
        }
        case IOCTL_MXSUPERIO_WRITE:
            log_misc("mxsuperio",
                     "DeviceIoControl(<mxsuperio>, <write>, 0x%p, 0x%x, -, "
                     "0x%x, -, -)",
                     lpInBuffer, nInBufferSize, nOutBufferSize);

            /*
            {
                 IN BYTE chip select (0 = base 0x2e, 1 = base 0x4e)
                 IN BYTE logical device (0 = FPC, 11 = HW mon)
                 IN BYTE index
                 IN BYTE write value
            }
            */
            if (lpBytesReturned) *lpBytesReturned = 4;
            break;
        case IOCTL_MXSUPERIO_HWMONITOR_LPC_READ: {
            // TODO: This code is all for W83791D, however W83627UHG is more appropriate in some
            // cases. Is this an AAL vs AAM difference? Where was the W83791D usage derived from?
            BYTE index = lpc_packet->index;
            BYTE reg = lpc_packet->reg;

            switch (w83791d_bank) {
                case 0x00: {
                    switch (reg) {
                        case W83627UHG_REG_FAN_CONF:
                            lpc_out->data = w83627uhg_fanconf;
                            break;
                        case W83627UHG_REG_FAN_CONF_2:
                            lpc_out->data = w83627uhg_fanconf_2;
                            break;
                        case W83627UHG_REG_CPUFANOUT_TEMP_SRC_SLCT:
                            lpc_out->data = w83627uhg_cpufanout_src_slct;
                            break;
                        case W83627UHG_REG_SYSFANOUT_TEMP_SRC_SLCT:
                            lpc_out->data = w83627uhg_sysfanout_src_slct;
                            break;
                        case W83627UHG_REG_SYSTIN_TARGET_TEMP:
                            lpc_out->data = w83627uhg_systin_target;
                            break;
                        case W83627UHG_REG_CPUTIN_TARGET_TEMP:
                            lpc_out->data = w83627uhg_cputin_target;
                            break;
                        case W83627UHG_REG_TARGET_TEMP_TOLERANCE:
                            lpc_out->data = w83627uhg_target_tolerance;
                            break;

                        case W83791D_REG_WCHIPID:
                            lpc_out->data = 0x71;  // Observed: c1
                            break;
                        case W83791D_REG_CHIPMAN:
                            if (w83791d_4f_high)
                                lpc_out->data = 0x5c;  // High byte
                            else
                                lpc_out->data = 0xa3;  // Low byte
                            break;
                        case W83791D_REG_I2C_ADDR:
                            lpc_out->data = 0x11;  // Observed: 2d
                            break;
                        case W83791D_REG_BANK:
                            lpc_out->data = w83791d_bank;
                            break;

                        case W83791D_REG_CONFIG:
                            lpc_out->data = w83791d_config;
                            break;

                        case W83791D_REG_BEEP_CTRL_0:
                            // No beeping :(
                            lpc_out->data = 0x00;
                            break;

                        // TODO: No idea how to read any of these. Pulled from real system
                        case W83791D_REG_TEMP1_0:
                            lpc_out->data = 0x1a;
                            break;
                        case W83791D_RAM_VCOREA:
                            lpc_out->data = 0x76;
                            break;
                        case W83791D_RAM_VNIR0:
                            lpc_out->data = 0x86;
                            break;
                        case W83791D_RAM_VDD5:
                            lpc_out->data = 0x94;
                            break;
                        case W83791D_RAM_12VIN:
                            lpc_out->data = 0xbe;
                            break;
                        case W83791D_RAM_N12VIN:
                            lpc_out->data = 0xd1;
                            break;

                        case W83791D_RAM_FAN1:
                        case W83791D_RAM_FAN2:
                        case W83791D_RAM_FAN3:
                            lpc_out->data = 0x00;  // Fan no spinny!
                            break;

                        case W83791D_VID_FAN_DIV:
                            // Boths fans divide by 2 (01)
                            lpc_out->data = 0b00000101;
                            break;

                        case W83791D_VBAT_MONITOR_CONTROL:
                            lpc_out->data = w83791d_vbat_monitor_control;
                            break;

                        default:
                            log_error("mxsuperio", "Unknown HM b0 register: read 0x%02x", reg);
                            return FALSE;
                            // exit(1);
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
            log_misc("mxsuperio", "amHmLpcReadByte Index=0x%02x Reg=0x%02x Data=0x%02x", index, reg,
                     lpc_out->data);

            if (lpBytesReturned) *lpBytesReturned = sizeof(mxsuperio_lpc_packet);
        } break;
        case IOCTL_MXSUPERIO_HWMONITOR_LPC_WRITE: {
            BYTE index = lpc_packet->index;
            BYTE reg = lpc_packet->reg;
            log_misc("mxsuperio", "amHmLpcWriteByte Index=0x%02x Reg=0x%02x Data=0x%02b", index,
                     reg, lpc_packet->data);

            switch (w83791d_bank) {
                case 0x00: {
                    switch (reg) {
                        case W83627UHG_REG_FAN_CONF:
                            w83627uhg_fanconf = lpc_packet->data;
                            break;
                        case W83627UHG_REG_FAN_CONF_2:
                            w83627uhg_fanconf_2 = lpc_packet->data;
                            break;
                        case W83627UHG_REG_CPUFANOUT_VALUE_SELECT:
                            // Just no-op this
                            break;
                        case W83627UHG_REG_CPUFANOUT_TEMP_SRC_SLCT:
                            w83627uhg_cpufanout_src_slct = lpc_packet->data;
                            break;
                        case W83627UHG_REG_SYSFANOUT_TEMP_SRC_SLCT:
                            w83627uhg_sysfanout_src_slct = lpc_packet->data;
                            break;
                        case W83627UHG_REG_SYSTIN_TARGET_TEMP:
                            w83627uhg_systin_target = lpc_packet->data;
                            break;
                        case W83627UHG_REG_CPUTIN_TARGET_TEMP:
                            w83627uhg_cputin_target = lpc_packet->data;
                            break;
                        case W83627UHG_REG_TARGET_TEMP_TOLERANCE:
                            w83627uhg_target_tolerance = lpc_packet->data;
                            break;
                        case W83627UHG_REG_SYSFANOUT_STOP_VALUE:
                        case W83627UHG_REG_SYSFANOUT_START_VALUE:
                            break;

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
                            log_error("mxsuperio", "Unknown HM b0 register: write 0x%02x", reg);
                            // exit(1);
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
