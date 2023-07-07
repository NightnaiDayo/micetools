#include "gui.h"

#include <detours.h>

#include "../input.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HWND mainWindow = NULL;

static unsigned int hookType;

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

BOOL GetD3D9Device(void** pTable, size_t Size) {
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

static HRESULT(WINAPI* TrueEndScene)(IDirect3DDevice9*);

void draw_rect(IDirect3DDevice9* dev, int x, int y, int w, int h, unsigned char r, unsigned char g,
               unsigned char b) {
    D3DCOLOR rectColor = D3DCOLOR_XRGB(r, g, b);
    D3DRECT BarRect = { x, y, x + w, y + h };

    dev->lpVtbl->Clear(dev, 1, &BarRect, D3DCLEAR_TARGET | D3DCLEAR_TARGET, rectColor, 0, 0);
}

HRESULT __stdcall hkEndScene(IDirect3DDevice9* pDevice) {
    static bool showMenu = false;
    hookType = UI_HOOK_DX9;

    end_scene_hook_t* head = end_scene_hook_list;
    while (head != NULL) {
        head->hook(hookType, pDevice);
        head = head->next;
    }
    return TrueEndScene(pDevice);
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

DWORD changeCursorState = (DWORD)-1;
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass,
                         DWORD_PTR dwRefData) {
    ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

    if (changeCursorState == 1) {
        while (ShowCursor(TRUE) < 0)
            ;
        changeCursorState = (DWORD)-1;
    } else if (changeCursorState == 0) {
        while (ShowCursor(FALSE) >= 0)
            ;
        changeCursorState = (DWORD)-1;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void post_win_create(HWND hWnd) {
    // Don't double-hook!
    if (TrueEndScene != NULL) return;

    mainWindow = hWnd;

    if (hookType == UI_HOOK_DX9) {
        void* d3d9Device[119];
        if (GetD3D9Device(d3d9Device, sizeof(d3d9Device))) {
            TrueEndScene = d3d9Device[42];

            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourAttach((PVOID*)&TrueEndScene, hkEndScene);
            DetourTransactionCommit();
        }

        if (hWnd && !SetWindowSubclass(hWnd, WndProc, (UINT_PTR)&WndProc, (DWORD_PTR)NULL)) {
            log_error(plfGUI, "failed to SetWindowSubclass(%d)", GetLastError());
        }
    } else {
        MiceInputInit(hWnd);
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
        //     // We're going to only adjust y, on the assumption x is unchanged, and even if it is
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
        pPresentationParameters->FullScreen_RefreshRateInHz = 0;
    } else if (pPresentationParameters->Windowed) {
        D3DDISPLAYMODE d3ddm;
        this->lpVtbl->GetAdapterDisplayMode(this, Adapter, &d3ddm);
        pPresentationParameters->Windowed = FALSE;
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
