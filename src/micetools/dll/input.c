#include "input.h"
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#include <GuidDef.h>
#include <Objbase.h>
#include <stdio.h>

#include "../lib/mice/log.h"

static PMICE_DI_DEVICE _MiceDIDevices = NULL;
static LPDIRECTINPUT8A _MiceDI = NULL;

void MiceInputPollDevices(void) {
    PMICE_DI_DEVICE lpDevice = _MiceDIDevices;
    while (lpDevice != NULL) {
        lpDevice->m_lpInput->lpVtbl->Poll(lpDevice->m_lpInput);

        switch (lpDevice->m_dwType) {
            case DI8DEVTYPE_JOYSTICK:
            case DI8DEVTYPE_GAMEPAD:
                lpDevice->m_lpInput->lpVtbl->GetDeviceState(lpDevice->m_lpInput,
                                                            sizeof lpDevice->m_State.m_Joystate,
                                                            &lpDevice->m_State.m_Joystate);
                break;
        }

        lpDevice = lpDevice->m_Next;
    }
}
void MiceInputFreeDevices(void) {
    PMICE_DI_DEVICE lpDevice = _MiceDIDevices;
    while (lpDevice != NULL) {
        lpDevice->m_lpInput->lpVtbl->Unacquire(lpDevice->m_lpInput);
        lpDevice->m_lpInput->lpVtbl->Release(lpDevice->m_lpInput);

        PMICE_DI_DEVICE next = lpDevice->m_Next;
        free(lpDevice);
        lpDevice = next;
    }
    _MiceDIDevices = NULL;
}

BOOL MiceInputGetNewBinding(PMICE_BUTTON_BINDING lpBinding) {
    MiceInputPollDevices();

    PMICE_DI_DEVICE lpDevice = _MiceDIDevices;
    while (lpDevice != NULL) {
        PMICE_DI_STATE state = &lpDevice->m_State;
        PMICE_DI_STATE lastState = &lpDevice->m_LastState;
        if (lpBinding != NULL) {
            switch (lpDevice->m_dwType) {
                case DI8DEVTYPE_JOYSTICK:
                case DI8DEVTYPE_GAMEPAD:
                    for (int button = 0; button < 32; button++) {
                        if (state->m_Joystate.rgbButtons[button] &&
                            !lastState->m_Joystate.rgbButtons[button]) {
                            lpBinding->m_Type = MICE_BB_TYPE_DI_JOY;
                            lpBinding->m_DIJoy.m_lpDeviceGuid = lpDevice->m_GUID;
                            lpBinding->m_DIJoy.m_ButtonMajor = MICE_BUTTON_MAJOR_BUTTON;
                            lpBinding->m_DIJoy.m_ButtonMinor = button;
                            return TRUE;
                        }
                    }

                    if (state->m_Joystate.rgdwPOV[0] != lastState->m_Joystate.rgdwPOV[0]) {
                        BOOL bound = FALSE;
                        int to = -1;
                        if (state->m_Joystate.rgdwPOV[0] == 0) {
                            to = MICE_DPAD_UP;
                            bound = TRUE;
                        } else if (state->m_Joystate.rgdwPOV[0] == 18000) {
                            to = MICE_DPAD_DOWN;
                            bound = TRUE;
                        } else if (state->m_Joystate.rgdwPOV[0] == 27000) {
                            to = MICE_DPAD_LEFT;
                            bound = TRUE;
                        } else if (state->m_Joystate.rgdwPOV[0] == 9000) {
                            to = MICE_DPAD_RIGHT;
                            bound = TRUE;
                        }
                        if (bound) {
                            lpBinding->m_Type = MICE_BB_TYPE_DI_JOY;
                            lpBinding->m_DIJoy.m_lpDeviceGuid = lpDevice->m_GUID;
                            lpBinding->m_DIJoy.m_ButtonMajor = MICE_BUTTON_MAJOR_DPAD;
                            lpBinding->m_DIJoy.m_ButtonMinor = to;
                            return TRUE;
                        }
                    }

                    // TODO: Axes
                    break;
            }
        }
        memcpy(lastState, state, sizeof *lastState);

        lpDevice = lpDevice->m_Next;
    }

    // Skip the mouse buttons
    for (int i = 8; i < 0xff; i++) {
        if (GetAsyncKeyState(i) < 0) {
            lpBinding->m_AsyncKey.m_Key = (CHAR)i;
            lpBinding->m_Type = MICE_BB_TYPE_GET_ASYNC_KEY;
            return TRUE;
        }
    }

    return FALSE;
}

static BOOL CALLBACK _DInputEnum(LPDIDEVICEINSTANCEA lpddi, LPVOID data) {
    PMICE_DI_DEVICE lpTail = _MiceDIDevices;
    PMICE_DI_DEVICE lpNew;
    if (lpTail == NULL) {
        lpNew = _MiceDIDevices = malloc(sizeof *lpNew);
    } else {
        while (lpTail->m_Next != NULL) lpTail = lpTail->m_Next;
        lpNew = lpTail->m_Next = malloc(sizeof *lpNew);
    }

    ZeroMemory(lpNew, sizeof *lpNew);
    lpNew->m_GUID = lpddi->guidInstance;
    lpNew->m_dwType = lpddi->dwDevType & 0xff;
    memcpy_s(lpNew->m_szName, sizeof lpNew->m_szName, lpddi->tszProductName,
             sizeof lpddi->tszProductName);
    lpNew->m_szName[sizeof lpNew->m_szName - 1] = '\0';

    printf("Found device: %s\n", lpNew->m_szName);

    return DIENUM_CONTINUE;
}
int MiceInputGrabDevices(HWND hWnd) {
    if (_MiceDI == NULL) return -2;

        // The first argument here is throwing warnings
#pragma warning(disable : 4090 4028)
    _MiceDI->lpVtbl->EnumDevices(_MiceDI, DI8DEVTYPE_JOYSTICK, _DInputEnum, NULL,
                                 DIEDFL_ATTACHEDONLY);
    _MiceDI->lpVtbl->EnumDevices(_MiceDI, DI8DEVTYPE_GAMEPAD, _DInputEnum, NULL,
                                 DIEDFL_ATTACHEDONLY);
#pragma warning(default : 4090 4028)

    PMICE_DI_DEVICE lpDevice = _MiceDIDevices;
    while (lpDevice != NULL) {
        _MiceDI->lpVtbl->CreateDevice(_MiceDI, &lpDevice->m_GUID, &lpDevice->m_lpInput, NULL);

        switch (lpDevice->m_dwType) {
            case DI8DEVTYPE_GAMEPAD:
            case DI8DEVTYPE_JOYSTICK:
                lpDevice->m_lpInput->lpVtbl->SetCooperativeLevel(
                    lpDevice->m_lpInput, hWnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
                lpDevice->m_lpInput->lpVtbl->SetDataFormat(lpDevice->m_lpInput, &c_dfDIJoystick);
                lpDevice->m_lpInput->lpVtbl->Acquire(lpDevice->m_lpInput);
                break;
        }
        lpDevice = lpDevice->m_Next;
    }

    return 0;
}

static BOOL _MiceInputHasInit = FALSE;
int MiceInputInit(HWND hWnd) {
    if (_MiceInputHasInit) return 0;
    _MiceInputHasInit = TRUE;

    if (_MiceDI == NULL)
        if (!(SUCCEEDED(DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION,
                                           &IID_IDirectInput8, (void **)&_MiceDI, NULL)))) {
            log_error(plfMisc, "DirectInput8Create failed: %d", GetLastError());
            _MiceDI = NULL;
            return -1;
        }
    return MiceInputGrabDevices(hWnd);
}
void MiceInputDestroy(void) {
    MiceInputFreeDevices();

    _MiceDI->lpVtbl->Release(_MiceDI);
    _MiceDI = NULL;
    _MiceInputHasInit = FALSE;
}

BOOL MiceInputGetButtonState(PMICE_BUTTON_BINDING lpBinding) {
    BOOL pressed = FALSE;
    switch (lpBinding->m_Type) {
        case MICE_BB_TYPE_GET_ASYNC_KEY: {
            pressed = (GetAsyncKeyState(lpBinding->m_AsyncKey.m_Key) < 0);
            break;
        }
        case MICE_BB_TYPE_DI_JOY: {
            PMICE_DI_DEVICE lpDevice = _MiceDIDevices;
            while (lpDevice != NULL) {
                if (IsEqualGUID(&lpDevice->m_GUID, &lpBinding->m_DIJoy.m_lpDeviceGuid)) break;
                lpDevice = lpDevice->m_Next;
            }
            if (lpDevice == NULL) break;

            switch (lpBinding->m_DIJoy.m_ButtonMajor) {
                case MICE_BUTTON_MAJOR_AXIS:
                    break;
                case MICE_BUTTON_MAJOR_DPAD: {
                    DWORD pov = lpDevice->m_State.m_Joystate.rgdwPOV[0];
                    switch (lpBinding->m_DIJoy.m_ButtonMinor) {
                        case MICE_DPAD_UP:
                            pressed = pov == 0 || pov == 4500 || pov == 31500;
                            break;
                        case MICE_DPAD_DOWN:
                            pressed = pov == 18000 || pov == 22500 || pov == 13500;
                            break;
                        case MICE_DPAD_LEFT:
                            pressed = pov == 27000 || pov == 31500 || pov == 22500;
                            break;
                        case MICE_DPAD_RIGHT:
                            pressed = pov == 9000 || pov == 4500 || pov == 13500;
                            break;
                    }
                    break;
                }
                case MICE_BUTTON_MAJOR_BUTTON: {
                    DWORD button = lpBinding->m_DIJoy.m_ButtonMinor;
                    if (button <= 32) pressed = lpDevice->m_State.m_Joystate.rgbButtons[button] > 0;
                    break;
                }
            }
        }
    }
    if (lpBinding->m_Invert) pressed = !pressed;
    return pressed;
}
