#include "gui.h"

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
BOOL GetD3D9Device(void** pTable, size_t Size) {
    if (!pTable) return false;

    IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
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

        dummyDeviceCreated = pD3D->lpVtbl->CreateDevice(pD3D, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow,
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

void draw_rect(IDirect3DDevice9* dev, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
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
    // ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
HWND WINAPI FakeCreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y,
                                int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance,
                                LPVOID lpParam) {
    HWND hWnd = TrueCreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent,
                                    hMenu, hInstance, lpParam);

    void* d3d9Device[119];

    if (GetD3D9Device(d3d9Device, sizeof(d3d9Device))) {
        *((PVOID*)&TrueEndScene) = CreateHook((PVOID)d3d9Device[42], (PVOID)hkEndScene, 7);
    }

    if (hWnd && !SetWindowSubclass(hWnd, WndProc, (int)&WndProc, (DWORD_PTR)NULL)) {
        log_error("gui", "failed to SetWindowSubclass(%d)", GetLastError());
    }

    return hWnd;
}

void hook_gui() {
    //
    hook("User32.dll", "CreateWindowExA", FakeCreateWindowExA, (void**)&TrueCreateWindowExA, 7);
}
