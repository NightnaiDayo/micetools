#include "gui.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HWND mainWindow;

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

BOOL UnFrameWindow(HWND hwnd) {
    SetLastError(ERROR_SUCCESS);
    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    if (GetLastError() != ERROR_SUCCESS) return FALSE;

    RECT rect;
    if (!GetClientRect(hwnd, &rect)) return FALSE;

    style &= ~(WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU);
    if (!AdjustWindowRect(&rect, style, FALSE)) return FALSE;

    SetWindowLongW(hwnd, GWL_STYLE, style);
    if (!SetWindowPos(hwnd, HWND_TOP, rect.left, rect.top, rect.right - rect.left,
                      rect.bottom - rect.top, SWP_FRAMECHANGED | SWP_NOMOVE))
        return FALSE;
    return TRUE;
}

BOOL FrameWindow(HWND hwnd) {
    SetLastError(ERROR_SUCCESS);
    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    if (GetLastError() != ERROR_SUCCESS) return FALSE;

    RECT rect;
    if (!GetClientRect(hwnd, &rect)) return FALSE;

    style |= WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
    if (!AdjustWindowRect(&rect, style, FALSE)) return FALSE;

    SetWindowLongW(hwnd, GWL_STYLE, style);

    if (!SetWindowPos(hwnd, HWND_TOP, rect.left, rect.top, rect.right - rect.left,
                      rect.bottom - rect.top, SWP_FRAMECHANGED | SWP_NOMOVE))
        return FALSE;
    return TRUE;
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

    end_scene_hook_t* head = end_scene_hook_list;
    while (head != NULL) {
        head->hook(pDevice);
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

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass,
                         DWORD_PTR dwRefData) {
    ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void post_win_create(HWND hWnd) {
    // Don't double-hook!
    if (TrueEndScene != NULL) return;

    mainWindow = hWnd;

    void* d3d9Device[119];
    if (GetD3D9Device(d3d9Device, sizeof(d3d9Device))) {
        *((PVOID*)&TrueEndScene) = CreateHook32((PVOID)d3d9Device[42], (PVOID)hkEndScene);
    }

    if (hWnd && !SetWindowSubclass(hWnd, WndProc, (int)&WndProc, (DWORD_PTR)NULL)) {
        log_error("gui", "failed to SetWindowSubclass(%d)", GetLastError());
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

void SetupWindowPosition(int* X, int* Y, int* nWidth, int* nHeight) {
    monitorIndex = 0;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)NULL);

    if (MiceConfig.window.w) *nWidth = MiceConfig.window.w;
    if (MiceConfig.window.h) *nHeight = MiceConfig.window.h;

    if (MiceConfig.window.centre) {
        *X = ((monitorRect.right - monitorRect.left) - *nWidth) / 2;
        *Y = ((monitorRect.bottom - monitorRect.top) - *nHeight) / 2;
    } else {
        *X = MiceConfig.window.x;
        *Y = MiceConfig.window.y;
    }
    *X += monitorRect.left;
    *Y += monitorRect.top;
}

HWND WINAPI FakeCreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName,
                                DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
                                HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
    // Pass-through for system stuff
    if (lpWindowName == NULL || strcmp(lpWindowName, "OleMainThreadWndName") == 0 ||
        strcmp(lpWindowName, "CicMarshalWnd") == 0) {
        return TrueCreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth,
                                   nHeight, hWndParent, hMenu, hInstance, lpParam);
    }

    SetupWindowPosition(&X, &Y, &nWidth, &nHeight);
    HWND hWnd = TrueCreateWindowExA(dwExStyle, lpClassName, "Micetools", dwStyle, X, Y, nWidth,
                                    nHeight, hWndParent, hMenu, hInstance, lpParam);
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

    SetupWindowPosition(&X, &Y, &nWidth, &nHeight);
    HWND hWnd = TrueCreateWindowExW(dwExStyle, lpClassName, L"Micetools", dwStyle, X, Y, nWidth,
                                    nHeight, hWndParent, hMenu, hInstance, lpParam);
    post_win_create(hWnd);
    return hWnd;
}

static HRESULT(STDMETHODCALLTYPE* TrueCreateDevice)(IDirect3D9* this, UINT Adapter,
                                                    D3DDEVTYPE DeviceType, HWND hFocusWindow,
                                                    DWORD BehaviorFlags,
                                                    D3DPRESENT_PARAMETERS* pPresentationParameters,
                                                    IDirect3DDevice9** ppReturnedDeviceInterface);

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
    GetWindowRect(hFocusWindow, &winRect);
    int w = MiceConfig.window.w ? MiceConfig.window.w : (winRect.right - winRect.left);
    int h = MiceConfig.window.h ? MiceConfig.window.h : (winRect.bottom - winRect.top);
    int x = MiceConfig.window.x;
    int y = MiceConfig.window.y;
    if (MiceConfig.window.centre) {
        x = ((monitorRect.right - monitorRect.left) - w) / 2;
        y = ((monitorRect.bottom - monitorRect.top) - h) / 2;
    }

    x += monitorRect.left;
    y += monitorRect.top;

    SetWindowPos(hFocusWindow, HWND_TOP, x, y, w, h, 0);

    return TrueCreateDevice(this, Adapter, DeviceType, hFocusWindow, BehaviorFlags,
                            pPresentationParameters, ppReturnedDeviceInterface);
}

IDirect3D9* WINAPI FakeDirect3DCreate9(UINT SDKVersion) {
    IDirect3D9* pD3D = TrueDirect3DCreate9(D3D_SDK_VERSION);

    TrueCreateDevice = pD3D->lpVtbl->CreateDevice;
    DWORD patch = (DWORD)&FakeCreateDevice;
    patch_at(&pD3D->lpVtbl->CreateDevice, (char*)&patch, 4);

    return pD3D;
};

int WINAPI FakeGetSystemMetrics(int nIndex) {
    int real = TrueGetSystemMetrics(nIndex);
    if (nIndex == SM_CXSCREEN && MiceConfig.window.w) return MiceConfig.window.w;
    if (nIndex == SM_CYSCREEN && MiceConfig.window.h) return MiceConfig.window.h;
    return nIndex;
}

void hook_gui() {
    hook("User32.dll", "CreateWindowExA", FakeCreateWindowExA, (void**)&TrueCreateWindowExA);
    hook("User32.dll", "CreateWindowExW", FakeCreateWindowExW, (void**)&TrueCreateWindowExW);
    hook("User32.dll", "GetSystemMetrics", FakeGetSystemMetrics, (void**)&TrueGetSystemMetrics);
    hook("D3d9.dll", "Direct3DCreate9", FakeDirect3DCreate9, (void**)&TrueDirect3DCreate9);
}
