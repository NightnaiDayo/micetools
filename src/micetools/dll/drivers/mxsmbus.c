#include "../hooks/setupapi_.h"
#include "../../lib/mice/dmi.h"
#include "mx.h"
#include "smbus.h"

smbus_callback_t* smbus_devices[256] = { NULL };

void smbus_install(BYTE v_addr, smbus_callback_t* write, smbus_callback_t* read) {
    smbus_devices[v_addr << 1] = write;
    smbus_devices[(v_addr << 1) + 1] = read;
}

BOOL handle_smbus(BYTE command, WORD v_addr, WORD command_code, BYTE nbytes, BYTE* data) {
    WORD p_addr = (v_addr & 0x7fff) << 1;
    ich9_cmd_t smb_cmd = ICH9_CMD_QUICK;
    switch (command) {
        case MXSMBUS_CMD_WRITE_QUICK:
            break;
        case MXSMBUS_CMD_READ_QUICK:
            p_addr++;
            break;
        case MXSMBUS_CMD_WRITE_BYTE:
            smb_cmd = ICH9_CMD_BYTE;
            break;
        case MXSMBUS_CMD_READ_BYTE:
            p_addr++;
            smb_cmd = ICH9_CMD_BYTE;
            break;
        case MXSMBUS_CMD_WRITE_BYTE_DATA:
            smb_cmd = ICH9_CMD_BYTE_DATA;
            break;
        case MXSMBUS_CMD_READ_BYTE_DATA:
            p_addr++;
            smb_cmd = ICH9_CMD_BYTE_DATA;
            break;
        case MXSMBUS_CMD_WRITE_WORD_DATA:
            smb_cmd = ICH9_CMD_WORD_DATA;
            break;
        case MXSMBUS_CMD_READ_WORD_DATA:
            p_addr++;
            smb_cmd = ICH9_CMD_WORD_DATA;
            break;
        case MXSMBUS_CMD_WRITE_BLOCK:
            smb_cmd = ICH9_CMD_BLOCK;
            break;
        case MXSMBUS_CMD_READ_BLOCK:
            p_addr++;
            smb_cmd = ICH9_CMD_BLOCK;
            break;
        case MXSMBUS_CMD_PROCESS_CALL:
            smb_cmd = ICH9_CMD_PROCESS_CALL;
            break;
        case MXSMBUS_CMD_I2C:
            smb_cmd = ICH9_CMD_I2C_READ;
            break;
    }

    log_trace(plfMxSmbus, "Making request to %02X (virtual: %04X/%04x, cmd: %01X, code: %04X)", p_addr,
              v_addr, command, smb_cmd, command_code);

    smbus_callback_t* callback = smbus_devices[p_addr];
    if (callback == NULL) {
        log_error(plfMxSmbus, "Request to unregistered address: %02x", p_addr);
        return FALSE;
    }
    return (*callback)(smb_cmd, command_code, nbytes, data);
}

BOOL WINAPI mxsmbus_DeviceIoControl(file_context_t* ctx, DWORD dwIoControlCode, LPVOID lpInBuffer,
                             DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                             LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    // Default value
    if (lpBytesReturned) *lpBytesReturned = nOutBufferSize;

    switch (dwIoControlCode) {
        case IOCTL_MXSMBUS_GET_VERSION:
            log_misc(plfMxSmbus,
                     "DeviceIoControl(<mxsmbus>, <get version>, 0x%p, 0x%x, "
                     "-, 0x%x, -, -)",
                     lpInBuffer, nInBufferSize, nOutBufferSize);

            ((LPDWORD)lpOutBuffer)[0] = 0x01020001;
            if (lpBytesReturned) *lpBytesReturned = 4;
            break;
        case IOCTL_MXSMBUS_I2C: {
            // BYTE command = ((BYTE*)lpInBuffer)[1];
            PMXSMBUS_I2C_PACKET packet = (PMXSMBUS_I2C_PACKET)lpInBuffer;
            BYTE command = packet->command;
            if (command > 10) {
                ((BYTE*)lpOutBuffer)[0] = 0x19;
                return FALSE;
            }
            if (handle_smbus(command, packet->v_addr & 0x7fff, packet->command_code, packet->nbytes, packet->data)) {
                ((BYTE*)lpOutBuffer)[0] = 0x00;
                return TRUE;
            } else {
                ((BYTE*)lpOutBuffer)[0] = 0x01;
                return FALSE;
            };
        }
        case IOCTL_MXSMBUS_REQUEST: {
            // BYTE command = ((BYTE*)lpInBuffer)[1];
            PMXSMBUS_REQUEST_PACKET packet = (PMXSMBUS_REQUEST_PACKET)lpInBuffer;
            BYTE command = packet->command;
            if (command > 11) {
                ((BYTE*)lpOutBuffer)[0] = 0x19;
                return FALSE;
            }
            if (handle_smbus(command, packet->v_addr & 0x7f, packet->command_code, packet->nbytes, packet->data)) {
                ((BYTE*)lpOutBuffer)[0] = 0x00;
                return TRUE;
            } else {
                ((BYTE*)lpOutBuffer)[0] = 0x01;
                return FALSE;
            };
        }
        default:
            log_warning(plfMxSmbus, "unhandled 0x%08x", dwIoControlCode);
            return FALSE;
    }
    return TRUE;
}

void setup_mxsmbus() {
    file_hook_t* mxsmbus = new_file_hook(L"\\\\.\\mxsmbus");
    mxsmbus->DeviceIoControl = &mxsmbus_DeviceIoControl;

    hook_file(mxsmbus);

    if (!add_fake_device(&MXSMBUS_GUID, L"\\\\.\\mxsmbus")) {
        log_error(plfMxSmbus, "failed to install mxsmbus device");
    }
}
