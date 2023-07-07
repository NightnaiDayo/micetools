#pragma once

#include <Windows.h>
#define CINTERFACE
#define INITGUID
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

typedef enum MICE_BUTTON_BINDING_TYPE {
    MICE_BB_TYPE_UNBOUND,
    MICE_BB_TYPE_GET_ASYNC_KEY,
    MICE_BB_TYPE_DI_JOY,
} MICE_BUTTON_BINDING_TYPE;
typedef enum MICE_BUTTON_MAJOR {
    MICE_BUTTON_MAJOR_AXIS,
    MICE_BUTTON_MAJOR_DPAD,
    MICE_BUTTON_MAJOR_BUTTON,
} MICE_BUTTON_MAJOR;
#define MICE_DPAD_UP 0     // POV == 0
#define MICE_DPAD_DOWN 1   // POV = 18000
#define MICE_DPAD_LEFT 2   // POV == 27000
#define MICE_DPAD_RIGHT 3  // POV == 9000

typedef struct MICE_BUTTON_BINDING {
    MICE_BUTTON_BINDING_TYPE m_Type;
    BOOL m_Invert;
    union {
        struct {
            CHAR m_Key;
        } m_AsyncKey;
        struct {
            GUID m_lpDeviceGuid;
            MICE_BUTTON_MAJOR m_ButtonMajor;
            DWORD m_ButtonMinor;
        } m_DIJoy;
    };
} MICE_BUTTON_BINDING, *PMICE_BUTTON_BINDING;

typedef union MICE_DI_STATE {
    DIJOYSTATE m_Joystate;
} MICE_DI_STATE, *PMICE_DI_STATE;

typedef struct MICE_DI_DEVICE {
    LPDIRECTINPUTDEVICE8A m_lpInput;
    GUID m_GUID;
    CHAR m_szName[MAX_PATH];
    DWORD m_dwType;
    MICE_DI_STATE m_State;
    MICE_DI_STATE m_LastState;
    struct MICE_DI_DEVICE* m_Next;
} MICE_DI_DEVICE, *PMICE_DI_DEVICE;

int MiceInputInit(HWND hWnd);
void MiceInputDestroy(void);

int MiceInputGrabDevices(HWND hWnd);
void MiceInputPollDevices(void);
void MiceInputFreeDevices(void);

BOOL MiceInputGetButtonState(PMICE_BUTTON_BINDING lpBinding);
BOOL MiceInputGetNewBinding(PMICE_BUTTON_BINDING lpBinding);
