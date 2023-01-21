#pragma once
#include "common.h"

static HWND(WINAPI* TrueCreateWindowExA)(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName,
                                         DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
                                         HWND hWndParent, HMENU hMenu, HINSTANCE hInstance,
                                         LPVOID lpParam);
static HWND(WINAPI* TrueCreateWindowExW)(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName,
                                         DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
                                         HWND hWndParent, HMENU hMenu, HINSTANCE hInstance,
                                         LPVOID lpParam);
static BOOL(WINAPI* TrueSetSystemCursor)(HCURSOR hcur, DWORD id);
static IDirect3D9*(WINAPI* TrueDirect3DCreate9)(UINT SDKVersion);
static int(WINAPI* TrueGetSystemMetrics)(int nIndex);

#define _GetSystemMetrics (TrueGetSystemMetrics ? TrueGetSystemMetrics : GetSystemMetrics)

void draw_rect(IDirect3DDevice9* dev, int x, int y, int w, int h, unsigned char r, unsigned char g,
               unsigned char b);

typedef VOID(FnEndScene)(IDirect3DDevice9* dev);

typedef struct end_scene_hook {
    FnEndScene* hook;
    struct end_scene_hook* next;
} end_scene_hook_t;
end_scene_hook_t* end_scene_hook_list;

void register_gui_hook(FnEndScene* end_scene);
void hook_gui();
void setup_hud_gui();
