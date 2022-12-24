#include "../hooks/gui.h"

#ifndef CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#endif
#include "cimgui.h"

BOOL initialized = false;
BOOL haveFirstFrame = false;
static HWND window;

extern bool ImGui_ImplWin32_Init(void* hwnd);
extern void ImGui_ImplWin32_Shutdown();
extern void ImGui_ImplWin32_NewFrame();

extern bool ImGui_ImplDX9_Init(IDirect3DDevice9* device);
extern void ImGui_ImplDX9_Shutdown();
extern void ImGui_ImplDX9_NewFrame();
extern void ImGui_ImplDX9_RenderDrawData(ImDrawData* draw_data);
extern bool ImGui_ImplDX9_GetCP(IDirect3DDevice9* pDevice, D3DDEVICE_CREATION_PARAMETERS* CP);

extern CRITICAL_SECTION logger_lock;

void InitImGui(IDirect3DDevice9* pDevice) {
    D3DDEVICE_CREATION_PARAMETERS CP;
    ImGui_ImplDX9_GetCP(pDevice, &CP);
    window = CP.hFocusWindow;

    igCreateContext(NULL);
    ImGuiIO* io = igGetIO();
    io->IniFilename = NULL;
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImFontAtlas_AddFontDefault(io->Fonts, NULL);

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX9_Init(pDevice);
    initialized = true;
    return;
}

extern int jvs_buttons[2][16];
extern BOOL JVS_SENSE;
void jvs_tab() {
    int coin_count_1 = 0;
    int coin_count_2 = 0;

	igText("Player 1 Coins: %d", coin_count_1);
	igText("Player 2 Coins: %d", coin_count_2);
	igSeparator();
	igText("Sense: %d", JVS_SENSE);
	igSeparator();
	ImVec4 white = { 1.0, 1.0, 1.0, 1.0 };
	ImVec4 red = { 1.0, 0.0, 0.0, 1.0 };

	igColumns(5, NULL, true);
	// for (auto s : scancodes) {
	// 	igTextColored((GetKeyState(s.second) < 0) ? red : white, "%s: %x", s.first, s.second);
	// 	igNextColumn();
	// }
	igColumns(1, NULL, true);
	igColumns(8, NULL, true);
	for (int player = 0; player < 2; player++) {
		for (int button = 0; button < 16; button++) {
			int scan = jvs_buttons[player][button];
			igTextColored((GetKeyState(scan) < 0) ? red : white, "p%d_%02d: %02x", player, button, scan);
			igNextColumn();
		}
	}
	igColumns(1, NULL, true);

	igSeparator();
	igText("JVS Count");
	// for (auto c : jvsCount) {
	// 	igText("%x: %d", c.first, c.second);
	// }
	igSeparator();
	igText("Comio Count");
	// for (auto c : comioCount) {
	// 	igText("%x: %d", c.first, c.second);
	// }
}


void hud_gui(IDirect3DDevice9* dev) {
    static bool showMenu = false;

    // if (GetAsyncKeyState(scancodes["openMenu"]) & 1) {
    //     showMenu = !showMenu;
    // }

    if (!initialized) {
        InitImGui(dev);
        return;
    }
    // if (!showMenu) return;

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();

    igNewFrame();

    igBegin("maimai control", NULL, 0);
    if (!haveFirstFrame) {
        ImVec2 size = { 600, 700 };
        igSetWindowSize_Vec2(size, 0);
        haveFirstFrame = true;
    }

    EnterCriticalSection(&logger_lock);

    if (igBeginTabBar("MainTabs", 0)) {
        if (igBeginTabItem("Logs", NULL, 0)) {
            // log_tab();
            igEndTabItem();
        }
        if (igBeginTabItem("JVS", NULL, 0)) {
            jvs_tab();
            igEndTabItem();
        }
        igEndTabBar();
    }

    LeaveCriticalSection(&logger_lock);

    igEnd();

    igEndFrame();
    igRender();
    ImGui_ImplDX9_RenderDrawData(igGetDrawData());
}

void setup_hud_gui() { register_gui_hook(&hud_gui); }
