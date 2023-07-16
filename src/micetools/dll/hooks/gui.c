#include "gui.h"

#include <detours.h>

#include "../input.h"

// These APIs were added in Windows 8. Because they're events sent to the WndProc, we're safe to
// just re-define them ourselves, and get progressive enhancement!
#if (WINVER < 0x0602)
#define WM_NCPOINTERUPDATE 0x0241
#define WM_NCPOINTERDOWN 0x0242
#define WM_NCPOINTERUP 0x0243
#define WM_POINTERUPDATE 0x0245
#define WM_POINTERDOWN 0x0246
#define WM_POINTERUP 0x0247
#define WM_POINTERENTER 0x0249
#define WM_POINTERLEAVE 0x024A
#define WM_POINTERACTIVATE 0x024B
#define WM_POINTERCAPTURECHANGED 0x024C

#define POINTER_FLAG_INCONTACT 0x00000004
#define POINTER_FLAG_FIRSTBUTTON 0x00000010
#define POINTER_FLAG_CANCELED 0x00008000
#define POINTER_FLAG_DOWN 0x00010000
#define POINTER_FLAG_UP 0x00040000

#define GET_POINTERID_WPARAM(wParam) (LOWORD(wParam))

enum tagPOINTER_INPUT_TYPE {
    PT_POINTER = 1,   // Generic pointer
    PT_TOUCH = 2,     // Touch
    PT_PEN = 3,       // Pen
    PT_MOUSE = 4,     // Mouse
    PT_TOUCHPAD = 5,  // Touchpad
};

typedef DWORD POINTER_INPUT_TYPE;
typedef UINT32 POINTER_FLAGS;

typedef enum tagPOINTER_BUTTON_CHANGE_TYPE {
    POINTER_CHANGE_NONE,
    POINTER_CHANGE_FIRSTBUTTON_DOWN,
    POINTER_CHANGE_FIRSTBUTTON_UP,
    POINTER_CHANGE_SECONDBUTTON_DOWN,
    POINTER_CHANGE_SECONDBUTTON_UP,
    POINTER_CHANGE_THIRDBUTTON_DOWN,
    POINTER_CHANGE_THIRDBUTTON_UP,
    POINTER_CHANGE_FOURTHBUTTON_DOWN,
    POINTER_CHANGE_FOURTHBUTTON_UP,
    POINTER_CHANGE_FIFTHBUTTON_DOWN,
    POINTER_CHANGE_FIFTHBUTTON_UP,
} POINTER_BUTTON_CHANGE_TYPE;

typedef struct tagPOINTER_INFO {
    POINTER_INPUT_TYPE pointerType;
    UINT32 pointerId;
    UINT32 frameId;
    POINTER_FLAGS pointerFlags;
    HANDLE sourceDevice;
    HWND hwndTarget;
    POINT ptPixelLocation;
    POINT ptHimetricLocation;
    POINT ptPixelLocationRaw;
    POINT ptHimetricLocationRaw;
    DWORD dwTime;
    UINT32 historyCount;
    INT32 InputData;
    DWORD dwKeyStates;
    UINT64 PerformanceCount;
    POINTER_BUTTON_CHANGE_TYPE ButtonChangeType;
} POINTER_INFO;

// Our target API at compile time didn't support these functions, but these functions are only ever
// called if we've already received a touch window event, meaning we're actually running on a newer
// version of windows. That means we're safe to load in these functions and use them!
static HMODULE hmUser32 = NULL;
static BOOL(WINAPI* lpGetPointerType)(UINT32 pointerId, POINTER_INPUT_TYPE* pointerType) = NULL;
static BOOL MockGetPointerType(UINT32 pointerId, POINTER_INPUT_TYPE* pointerType) {
    puts("Loading mock");
    if (!hmUser32) hmUser32 = LoadLibraryA("User32.dll");
    if (!hmUser32) return FALSE;
    puts("Loading function");
    if (!lpGetPointerType) lpGetPointerType = (void*)GetProcAddress(hmUser32, "GetPointerType");
    if (!lpGetPointerType) return FALSE;
    puts("Calling");
    return lpGetPointerType(pointerId, pointerType);
}
static BOOL(WINAPI* lpGetPointerInfo)(UINT32 pointerId, POINTER_INFO* pointerInfo) = NULL;
static BOOL MockGetPointerInfo(UINT32 pointerId, POINTER_INFO* pointerInfo) {
    if (!hmUser32) hmUser32 = LoadLibraryA("User32.dll");
    if (!hmUser32) return FALSE;
    if (!lpGetPointerInfo) lpGetPointerInfo = (void*)GetProcAddress(hmUser32, "GetPointerInfo");
    if (!lpGetPointerInfo) return FALSE;
    return lpGetPointerInfo(pointerId, pointerInfo);
}
#else
// If our target API supports these, no mocking is needed
#define MockGetPointerType GetPointerType
#define MockGetPointerInfo GetPointerInfo
#endif

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HWND mainWindow = NULL;

static unsigned int hookType;
static IDirect3DDevice9* g_pd3dDevice = NULL;

static HWND window;
BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam) {
    DWORD wndProcId;
    GetWindowThreadProcessId(handle, &wndProcId);

    if (GetCurrentProcessId() != wndProcId) return TRUE;

    window = handle;
    return FALSE;
}
HWND GetProcessWindow() {
    window = NULL;
    EnumWindows(EnumWindowsCallback, 0);
    return window;
}

static const LONG frameFlags = WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
LONG UnFrameWindow(HWND hwnd) {
    SetLastError(ERROR_SUCCESS);
    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    if (GetLastError() != ERROR_SUCCESS) return style;

    RECT rect;
    if (!GetClientRect(hwnd, &rect)) return style;

    // Nothing to style
    if ((style & frameFlags) == 0) return style;

    LONG newStyle = style & ~(frameFlags);
    if (!AdjustWindowRect(&rect, newStyle, FALSE)) return style;

    SetWindowLongW(hwnd, GWL_STYLE, newStyle);
    if (!SetWindowPos(hwnd, HWND_TOP, rect.left, rect.top, rect.right - rect.left,
                      rect.bottom - rect.top, SWP_FRAMECHANGED | SWP_NOMOVE))
        return style;
    return newStyle;
}

LONG FrameWindow(HWND hwnd) {
    SetLastError(ERROR_SUCCESS);
    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    if (GetLastError() != ERROR_SUCCESS) return style;

    // Nothing to do
    if ((style & frameFlags) == frameFlags) return style;

    RECT rect;
    if (!GetClientRect(hwnd, &rect)) return style;

    LONG newStyle = style | frameFlags;
    if (!AdjustWindowRect(&rect, newStyle, FALSE)) return style;

    SetWindowLongW(hwnd, GWL_STYLE, newStyle);

    if (!SetWindowPos(hwnd, HWND_TOP, rect.left, rect.top, rect.right - rect.left,
                      rect.bottom - rect.top, SWP_FRAMECHANGED | SWP_NOMOVE))
        return style;
    return newStyle;
}

BOOL UnadjustWindowRect(LPRECT prc, DWORD dwStyle, BOOL fMenu) {
    RECT rc;
    SetRectEmpty(&rc);
    BOOL fRc = AdjustWindowRect(&rc, dwStyle, fMenu);
    if (fRc) {
        prc->left -= rc.left;
        prc->top -= rc.top;
        prc->right -= rc.right;
        prc->bottom -= rc.bottom;
    }
    return fRc;
}

BOOL GetD3D9Device(IDirect3DDevice9Vtbl* pTable, size_t Size) {
    if (!pTable) return false;

    IDirect3D9* pD3D =
        (TrueDirect3DCreate9 ? TrueDirect3DCreate9 : Direct3DCreate9)(D3D_SDK_VERSION);
    if (!pD3D) return false;

    IDirect3DDevice9* pDummyDevice = NULL;
    D3DPRESENT_PARAMETERS d3dpp = { 0 };
    d3dpp.Windowed = false;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = GetProcessWindow();

    HRESULT dummyDeviceCreated =
        pD3D->lpVtbl->CreateDevice(pD3D, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow,
                                   D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice);

    if (dummyDeviceCreated != S_OK) {
        d3dpp.Windowed = !d3dpp.Windowed;

        dummyDeviceCreated = pD3D->lpVtbl->CreateDevice(
            pD3D, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice);
        if (dummyDeviceCreated != S_OK) {
            pD3D->lpVtbl->Release(pD3D);
            return false;
        }
    }

    memcpy(pTable, *(void***)pDummyDevice, Size);

    pDummyDevice->lpVtbl->Release(pDummyDevice);
    pD3D->lpVtbl->Release(pD3D);
    return true;
}

static STDMETHOD(TruePresent)(IDirect3DDevice9* This, CONST RECT* pSourceRect,
                              CONST RECT* pDestRect, HWND hDestWindowOverride,
                              CONST RGNDATA* pDirtyRegion);

void draw_rect(IDirect3DDevice9* dev, int x, int y, int w, int h, unsigned char r, unsigned char g,
               unsigned char b) {
    D3DCOLOR rectColor = D3DCOLOR_XRGB(r, g, b);
    D3DRECT BarRect = { x, y, x + w, y + h };

    dev->lpVtbl->Clear(dev, 1, &BarRect, D3DCLEAR_TARGET | D3DCLEAR_TARGET, rectColor, 0, 0);
}

HRESULT STDMETHODCALLTYPE hkPresent(IDirect3DDevice9* This, CONST RECT* pSourceRect,
                                    CONST RECT* pDestRect, HWND hDestWindowOverride,
                                    CONST RGNDATA* pDirtyRegion) {
    hookType = UI_HOOK_DX9;

    if (g_pd3dDevice != NULL) g_pd3dDevice->lpVtbl->BeginScene(g_pd3dDevice);

    end_scene_hook_t* head = end_scene_hook_list;
    while (head != NULL) {
        head->hook(hookType, This);
        head = head->next;
    }

    if (g_pd3dDevice != NULL) g_pd3dDevice->lpVtbl->EndScene(g_pd3dDevice);

    return TruePresent(This, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

void register_gui_hook(FnEndScene* end_scene) {
    end_scene_hook_t** head = &end_scene_hook_list;
    while (*head != NULL) {
        head = &((*head)->next);
    }
    end_scene_hook_t* hook = malloc(sizeof(end_scene_hook_t));
    hook->hook = end_scene;
    hook->next = NULL;
    *head = hook;
}

MICE_DA_NEW(g_activeTouches, ACTIVE_TOUCH)

void SetTouch(UINT id, INT x, INT y, BOOL state) {
    if (state) {
        MICE_DA_ITER(g_activeTouches, ACTIVE_TOUCH, i) {
            if (i->m_Id == id) {
                i->m_X = x;
                i->m_Y = y;
                return;
            }
        }
        MICE_DA_ITER_END
        ACTIVE_TOUCH touch = { .m_Id = id, .m_X = x, .m_Y = y };
        MiceDAPush(g_activeTouches, &touch);
    } else {
        MICE_DA_ITER(g_activeTouches, ACTIVE_TOUCH, i) {
            if (i->m_Id == id) MICE_DA_REMOVE_CURRENT(g_activeTouches);
        }
        MICE_DA_ITER_END
    }
}

void TouchHandler(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
    POINTER_INFO pointerInfo = { sizeof(POINTER_INFO) };
    POINTER_INPUT_TYPE pointerType = 0;

    if (MockGetPointerType(pointerId, &pointerType) &&
        MockGetPointerInfo(pointerId, &pointerInfo)) {
        POINT point = pointerInfo.ptPixelLocation;
        if (pointerType == PT_TOUCH && ScreenToClient(hWnd, &point)) {
            SetTouch(pointerInfo.pointerId, point.x, point.y,
                     !!(pointerInfo.pointerFlags & POINTER_FLAG_INCONTACT));  // |
        }
    }
}

DWORD changeCursorState = (DWORD)-1;
WNDPROC OriginalWndProc = NULL;
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass,
                         DWORD_PTR dwRefData) {
    if (hookType == UI_HOOK_DX9) {
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
    }

    if (changeCursorState == 1) {
        while (ShowCursor(TRUE) < 0)
            ;
        changeCursorState = (DWORD)-1;
    } else if (changeCursorState == 0) {
        while (ShowCursor(FALSE) >= 0)
            ;
        changeCursorState = (DWORD)-1;
    }

    switch (uMsg) {
        case WM_NCPOINTERUPDATE:
        case WM_NCPOINTERDOWN:
        case WM_NCPOINTERUP:
        case WM_POINTERUPDATE:
        case WM_POINTERDOWN:
        case WM_POINTERUP:
        case WM_POINTERENTER:
        case WM_POINTERLEAVE:
        case WM_POINTERACTIVATE:
        case WM_POINTERCAPTURECHANGED:
            TouchHandler(hWnd, wParam, lParam);
            break;
    }

    if (OriginalWndProc) {
        return CallWindowProcA(OriginalWndProc, hWnd, uMsg, wParam, lParam);
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void post_win_create(HWND hWnd) {
    // Don't double-hook!
    if (TruePresent != NULL) return;

    mainWindow = hWnd;

    if (hookType == UI_HOOK_DX9) {
        IDirect3DDevice9Vtbl d3d9Device;
        if (GetD3D9Device(&d3d9Device, sizeof(d3d9Device))) {
            TruePresent = d3d9Device.Present;

            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourAttach((PVOID*)&TruePresent, hkPresent);
            DetourTransactionCommit();
        }
        // MiceInputInit will be called by our D3D9 hooks instead
    } else {
        MiceInputInit(hWnd);
    }

    if (hWnd) {
        OriginalWndProc = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_WNDPROC);
        if (OriginalWndProc == NULL) log_warning(plfGUI, "GWLP_WNDPROC NULL");

        if (!SetWindowSubclass(hWnd, WndProc, (UINT_PTR)&WndProc, (DWORD_PTR)NULL)) {
            log_error(plfGUI, "failed to SetWindowSubclass(%d)", GetLastError());
        }
    }
}

RECT monitorRect = { 0 };
int monitorIndex = 0;
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor,
                              LPARAM dwData) {
    if (monitorIndex == MiceConfig.window.adaptor)
        memcpy(&monitorRect, lprcMonitor, sizeof monitorRect);
    monitorIndex++;
    return TRUE;
}

void SetupWindowPosition(LPRECT lpRect, DWORD dwStyle) {
    UnadjustWindowRect(lpRect, dwStyle, FALSE);

    int x = lpRect->left;
    int y = lpRect->top;
    int w = lpRect->right - x;
    int h = lpRect->bottom - y;

    monitorIndex = 0;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)NULL);

    if (!MiceConfig.window.nosize) {
        if (MiceConfig.window.w) w = MiceConfig.window.w;
        if (MiceConfig.window.h) h = MiceConfig.window.h;
    }

    if (MiceConfig.window.centre) {
        x = ((monitorRect.right - monitorRect.left) - w) / 2;
        y = ((monitorRect.bottom - monitorRect.top) - h) / 2;

        // if ((dwStyle & frameFlags) == frameFlags) {
        //     RECT adjustedRect;
        //     memcpy(&adjustedRect, lpRect, sizeof *lpRect);
        //     UnadjustWindowRect(&adjustedRect, dwStyle, FALSE);
        //     // We're going to only adjust y, on the assumption x is unchanged, and even if it
        //     is
        //     // it'll be symmetic. y has the titlebar to worry about.
        //     int outerH = adjustedRect.bottom - adjustedRect.top;

        //     y += (outerH - h) / 2;
        // }
    } else {
        x = MiceConfig.window.x;
        y = MiceConfig.window.y;
    }
    x += monitorRect.left;
    y += monitorRect.top;

    lpRect->left = x;
    lpRect->right = x + w;
    lpRect->top = y;
    lpRect->bottom = y + h;

    AdjustWindowRect(lpRect, dwStyle, FALSE);
}

HWND WINAPI FakeCreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName,
                                DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
                                HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
    // Pass-through for system stuff
    if (lpWindowName != NULL && (strcmp(lpWindowName, "OleMainThreadWndName") == 0 ||
                                 strcmp(lpWindowName, "CicMarshalWnd") == 0)) {
        return TrueCreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth,
                                   nHeight, hWndParent, hMenu, hInstance, lpParam);
    }

    RECT winRect;
    winRect.left = X;
    winRect.right = X + nWidth;
    winRect.top = Y;
    winRect.bottom = Y + nHeight;
    SetupWindowPosition(&winRect, dwStyle);
    HWND hWnd =
        TrueCreateWindowExA(dwExStyle, lpClassName, "Micetools", dwStyle, winRect.left, winRect.top,
                            winRect.right - winRect.left, winRect.bottom - winRect.top, hWndParent,
                            hMenu, hInstance, lpParam);
    post_win_create(hWnd);
    return hWnd;
}
HWND WINAPI FakeCreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName,
                                DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
                                HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
    // Pass-through for system stuff
    if (lpWindowName == NULL || wcscmp(lpWindowName, L"OleMainThreadWndName") == 0 ||
        wcscmp(lpWindowName, L"CicMarshalWnd") == 0) {
        return TrueCreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth,
                                   nHeight, hWndParent, hMenu, hInstance, lpParam);
    }

    RECT winRect;
    winRect.left = X;
    winRect.right = X + nWidth;
    winRect.top = Y;
    winRect.bottom = Y + nHeight;
    SetupWindowPosition(&winRect, dwStyle);
    HWND hWnd =
        TrueCreateWindowExW(dwExStyle, lpClassName, L"Micetools", dwStyle, winRect.left,
                            winRect.top, winRect.right - winRect.left, winRect.bottom - winRect.top,
                            hWndParent, hMenu, hInstance, lpParam);
    post_win_create(hWnd);
    return hWnd;
}

static HRESULT(STDMETHODCALLTYPE* TrueCreateDevice)(IDirect3D9* this, UINT Adapter,
                                                    D3DDEVTYPE DeviceType, HWND hFocusWindow,
                                                    DWORD BehaviorFlags,
                                                    D3DPRESENT_PARAMETERS* pPresentationParameters,
                                                    IDirect3DDevice9** ppReturnedDeviceInterface);
static HRESULT(STDMETHODCALLTYPE* TrueEnumAdapterModes)(IDirect3D9* this, UINT Adapter,
                                                        D3DFORMAT Format, UINT Mode,
                                                        D3DDISPLAYMODE* pMode);

extern RECT monitorRect;
HRESULT STDMETHODCALLTYPE FakeCreateDevice(IDirect3D9* this, UINT Adapter, D3DDEVTYPE DeviceType,
                                           HWND hFocusWindow, DWORD BehaviorFlags,
                                           D3DPRESENT_PARAMETERS* pPresentationParameters,
                                           IDirect3DDevice9** ppReturnedDeviceInterface) {
    if (MiceConfig.window.windowed) {
        pPresentationParameters->Windowed = TRUE;
        pPresentationParameters->PresentationInterval = D3DPRESENT_INTERVAL_ONE;
        pPresentationParameters->FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    } else /* if (pPresentationParameters->Windowed) */ {
        D3DDISPLAYMODE d3ddm;
        this->lpVtbl->GetAdapterDisplayMode(this, Adapter, &d3ddm);
        pPresentationParameters->Windowed = FALSE;
        pPresentationParameters->PresentationInterval = D3DPRESENT_INTERVAL_ONE;
        pPresentationParameters->FullScreen_RefreshRateInHz = d3ddm.RefreshRate;
    }

    if (MiceConfig.window.borderless)
        UnFrameWindow(hFocusWindow);
    else
        FrameWindow(hFocusWindow);
    Adapter = MiceConfig.window.adaptor;

    RECT winRect;
    if (!GetWindowRect(hFocusWindow, &winRect)) {
        log_error(plfGUI, "GetWindowRect failed: %d", GetLastError());
    } else {
        DWORD dwStyle = GetWindowLongW(hFocusWindow, GWL_STYLE);
        SetupWindowPosition(&winRect, dwStyle);

        // I have no idea why, but this is causing crashes???
        SetWindowPos(hFocusWindow, HWND_TOP, winRect.left, winRect.top,
                     winRect.right - winRect.left, winRect.bottom - winRect.top, 0);
    }

    HRESULT res = TrueCreateDevice(this, Adapter, DeviceType, hFocusWindow, BehaviorFlags,
                                   pPresentationParameters, ppReturnedDeviceInterface);
    if (res != S_OK) {
        switch (res) {
            case D3DERR_DEVICELOST:
                log_error(plfD3D9, "CreateDevice failed: Device lost");
                break;
            case D3DERR_INVALIDCALL:
                log_error(plfD3D9, "CreateDevice failed: Invalid call");
                break;
            case D3DERR_NOTAVAILABLE:
                log_error(plfD3D9, "CreateDevice failed: Requested configuration not available");
                break;
            case D3DERR_OUTOFVIDEOMEMORY:
                log_error(plfD3D9, "CreateDevice failed: VMem exhausted");
                break;
            default:
                log_error(plfD3D9, "CreateDevice failed: %08x", res);
                break;
        }
    } else {
        g_pd3dDevice = *ppReturnedDeviceInterface;
    }
    log_info(plfD3D9, "Device created: %d", res);

    MiceInputInit(hFocusWindow);

    return res;
}

HRESULT STDMETHODCALLTYPE FakeEnumAdapterModes(IDirect3D9* this, UINT Adapter, D3DFORMAT Format,
                                               UINT Mode, D3DDISPLAYMODE* pMode) {
    HRESULT res = TrueEnumAdapterModes(this, Adapter, Format, Mode, pMode);
    if (res == S_OK) {
        // printf("Found: %dx%d @ %d\n", pMode->Width, pMode->Height, pMode->RefreshRate);
        // TODO: Is this going to ever cause issues?
        pMode->Width = MiceConfig.window.w;
        pMode->Height = MiceConfig.window.h;
        pMode->RefreshRate = 60;
    }

    // printf("Request format: %d/%d. Result: %d\n", Format, Mode, res);
    // pMode->Format = Format;
    return res;
}

IDirect3D9* WINAPI FakeDirect3DCreate9(UINT SDKVersion) {
    IDirect3D9* pD3D = TrueDirect3DCreate9(D3D_SDK_VERSION);
    hookType = UI_HOOK_DX9;

    TrueCreateDevice = pD3D->lpVtbl->CreateDevice;
    size_t patch = (size_t)&FakeCreateDevice;
    MiceHookPatchAt(&pD3D->lpVtbl->CreateDevice, (char*)&patch, sizeof patch);

    TrueEnumAdapterModes = pD3D->lpVtbl->EnumAdapterModes;
    patch = (size_t)&FakeEnumAdapterModes;
    MiceHookPatchAt(&pD3D->lpVtbl->EnumAdapterModes, (char*)&patch, sizeof patch);

    return pD3D;
};

int WINAPI FakeGetSystemMetrics(int nIndex) {
    if (nIndex == SM_CXSCREEN && MiceConfig.window.w)
        return MiceConfig.window.w;
    else if (nIndex == SM_CYSCREEN && MiceConfig.window.h)
        return MiceConfig.window.h;
    return TrueGetSystemMetrics(nIndex);
}

void __stdcall Fake_glutFullScreen(void) {
    hookType = UI_HOOK_GLUT;
    if (!MiceConfig.window.windowed) True_glutFullScreen();
}

void __stdcall Fake_glutSwapBuffers(void) {
    hookType = UI_HOOK_GLUT;
    end_scene_hook_t* head = end_scene_hook_list;
    while (head != NULL) {
        head->hook(hookType, NULL);
        head = head->next;
    }
    True_glutSwapBuffers();
}

LONG WINAPI FakeChangeDisplaySettingsExW(LPCWSTR lpszDeviceName, DEVMODEW* lpDevMode, HWND hwnd,
                                         DWORD dwflags, LPVOID lParam) {
    if (MiceConfig.window.windowed) return DISP_CHANGE_SUCCESSFUL;

    return TrueChangeDisplaySettingsExW(lpszDeviceName, lpDevMode, hwnd, dwflags, lParam);
}

void __stdcall Fake_glutInitDisplayMode(unsigned int mode) {
    hookType = UI_HOOK_GLUT;
    True_glutInitDisplayMode(mode);
}

void __stdcall Fake_glutPositionWindow(int x, int y) {
    RECT winRect;
    GetWindowRect(window, &winRect);
    DWORD dwStyle = GetWindowLongW(window, GWL_STYLE);
    SetupWindowPosition(&winRect, dwStyle);

    // True_glutPositionWindow(winRect.left, winRect.top);
    // True_glutPositionWindow(x, y);
}
void __stdcall Fake_glutReshapeWindow(int width, int height) {
    RECT winRect;
    GetWindowRect(window, &winRect);
    DWORD dwStyle = GetWindowLongW(window, GWL_STYLE);
    SetupWindowPosition(&winRect, dwStyle);

    // True_glutReshapeWindow(winRect.right - winRect.left, winRect.bottom - winRect.top);
}

LONG WINAPI FakeChangeDisplaySettingsA(DEVMODEA* lpDevMode, DWORD dwFlags) {
    return DISP_CHANGE_SUCCESSFUL;
}
LONG WINAPI FakeChangeDisplaySettingsExA(LPCSTR lpszDeviceName, DEVMODEA* lpDevMode, HWND hwnd,
                                         DWORD dwflags, LPVOID lParam) {
    return DISP_CHANGE_SUCCESSFUL;
}

HCURSOR WINAPI FakeSetCursor(HCURSOR hCursor) { return NULL; }

void hook_gui() {
    hook("User32.dll", "CreateWindowExA", FakeCreateWindowExA, (void**)&TrueCreateWindowExA);
    hook("User32.dll", "CreateWindowExW", FakeCreateWindowExW, (void**)&TrueCreateWindowExW);

    hook("User32.dll", "GetSystemMetrics", FakeGetSystemMetrics, (void**)&TrueGetSystemMetrics);
    hook("User32.dll", "ChangeDisplaySettingsExW", FakeChangeDisplaySettingsExW,
         (void**)&TrueChangeDisplaySettingsExW);
    hook("D3d9.dll", "Direct3DCreate9", FakeDirect3DCreate9, (void**)&TrueDirect3DCreate9);

    hook("User32.dll", "ChangeDisplaySettingsA", FakeChangeDisplaySettingsA, NULL);
    hook("User32.dll", "ChangeDisplaySettingsExA", FakeChangeDisplaySettingsExA, NULL);
    hook("User32.dll", "SetCursor", FakeSetCursor, (void**)&TrueSetCursor);

    if (PathFileExistsA("FREEGLUT.DLL") && FALSE) {
        // Hooked as a way to identify use of GLUT
        hook("FREEGLUT.DLL", "glutInitDisplayMode", Fake_glutInitDisplayMode,
             (void**)&True_glutInitDisplayMode);
        hook("FREEGLUT.DLL", "glutFullScreen", Fake_glutFullScreen, (void**)&True_glutFullScreen);
        hook("FREEGLUT.DLL", "glutPositionWindow", Fake_glutPositionWindow,
             (void**)&True_glutPositionWindow);
        hook("FREEGLUT.DLL", "glutReshapeWindow", Fake_glutReshapeWindow,
             (void**)&True_glutReshapeWindow);
        hook("FREEGLUT.DLL", "glutSwapBuffers", Fake_glutSwapBuffers,
             (void**)&True_glutSwapBuffers);
    }
}
