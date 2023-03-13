#include "../hooks/gui.h"
#ifndef CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#endif
#include "../devices/smb_at24c64an.h"
#include "../key_config.h"
#include "cimgui.h"
#include "imgui/backends/GL/freeglut.h"
#include "imgui_memory_editor.h"

static HWND window;

/* UI Constants */
const ImVec4 DISABLED_COL = { .7f, .7f, .7f, 1.0f };
const ImVec4 WARN_COL = { 1.0f, 1.0f, .1f, 1.0f };
const ImVec4 DANGER_COL = { 1.0f, .1f, .1f, 1.0f };

struct {
    const char* name;
    int index;
    int pinNum;
} JVS_BUTTON_NAMES[JVS_BUTTON_PAIR_COUNT] = {
    { "Coin", 45 },   { "Start", 17 },  { "Service", 41 }, { "UP", 23 },     { "DOWN", 25 },
    { "LEFT", 21 },   { "RIGHT", 19 },  { "Push 1", 27 },  { "Push 2", 29 }, { "Push 3", 31 },
    { "Push 4", 33 }, { "Push 5", 35 }, { "Push 6", 37 },  { "Push 7", 39 }, { "Push 8", 47 },
};

const char* RESOLUTION_NAMES[] = {
    "Unspecified", "640x480",   "1024x600", "1024x768",
    "1280x720",    "1280x1024", "1360x768", "1920x1080",
};
const char* ORIENTATION_NAMES[] = {
    "Landscape",
    "Portrait",
};

/* ImGui useful globals */
static ImVec2 vec0 = { 0 };
static ImVec2 vec10 = { 1, 0 };

/* ImGui externs */
extern bool ImGui_ImplWin32_Init(void* hwnd);
extern void ImGui_ImplWin32_Shutdown();
extern void ImGui_ImplWin32_NewFrame();

extern bool ImGui_ImplDX9_Init(IDirect3DDevice9* device);
extern void ImGui_ImplDX9_Shutdown();
extern void ImGui_ImplDX9_NewFrame();
extern void ImGui_ImplDX9_RenderDrawData(ImDrawData* draw_data);
extern bool ImGui_ImplDX9_GetCP(IDirect3DDevice9* pDevice, D3DDEVICE_CREATION_PARAMETERS* CP);

extern bool ImGui_ImplGLUT_Init();
extern void ImGui_ImplGLUT_InstallFuncs();
extern void ImGui_ImplGLUT_Shutdown();
extern void ImGui_ImplGLUT_NewFrame();

extern void ImGui_ImplGLUT_ReshapeFunc(int w, int h);
extern void ImGui_ImplGLUT_MotionFunc(int x, int y);
extern void ImGui_ImplGLUT_MouseFunc(int button, int state, int x, int y);
extern void ImGui_ImplGLUT_MouseWheelFunc(int button, int dir, int x, int y);
extern void ImGui_ImplGLUT_KeyboardFunc(unsigned char c, int x, int y);
extern void ImGui_ImplGLUT_KeyboardUpFunc(unsigned char c, int x, int y);
extern void ImGui_ImplGLUT_SpecialFunc(int key, int x, int y);
extern void ImGui_ImplGLUT_SpecialUpFunc(int key, int x, int y);

extern bool ImGui_ImplOpenGL3_Init(const char* glsl_version);
extern void ImGui_ImplOpenGL3_Shutdown();
extern void ImGui_ImplOpenGL3_NewFrame();
extern void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data);

// TODO: Implement log viewing
extern CRITICAL_SECTION logger_lock;

void InitImGui(unsigned int hookType, IDirect3DDevice9* pDevice) {
    if (hookType == UI_HOOK_DX9) {
        D3DDEVICE_CREATION_PARAMETERS CP;
        ImGui_ImplDX9_GetCP(pDevice, &CP);
        window = CP.hFocusWindow;
    } else if (hookType == UI_HOOK_GLUT) {
        window = GetActiveWindow();
    }

    igCreateContext(NULL);
    ImGuiIO* io = igGetIO();
    io->IniFilename = NULL;
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImFontAtlas_AddFontDefault(io->Fonts, NULL);
    // ImFontAtlas_Build(io->Fonts);

    if (hookType == UI_HOOK_DX9) {
        ImGui_ImplWin32_Init(window);
        ImGui_ImplDX9_Init(pDevice);
    } else if (hookType == UI_HOOK_GLUT) {
        ImGui_ImplGLUT_Init();
        ImGui_ImplOpenGL3_Init(NULL);  // TODO: This

        /**
         * ! Important we can't use InstallFuncs, because it replaces the
         * ! reshape func, which games use. We handle this manuall ourself.
         *
         * // ImGui_ImplGLUT_InstallFuncs();
         * // glutReshapeFunc(ImGui_ImplGLUT_ReshapeFunc);
         */
        glutMotionFunc(ImGui_ImplGLUT_MotionFunc);
        glutPassiveMotionFunc(ImGui_ImplGLUT_MotionFunc);
        glutMouseFunc(ImGui_ImplGLUT_MouseFunc);
        glutKeyboardFunc(ImGui_ImplGLUT_KeyboardFunc);
        glutKeyboardUpFunc(ImGui_ImplGLUT_KeyboardUpFunc);
        glutSpecialFunc(ImGui_ImplGLUT_SpecialFunc);
        glutSpecialUpFunc(ImGui_ImplGLUT_SpecialUpFunc);
    }
}

void hud_fps() {
    if (igBegin("FPS", NULL,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoFocusOnAppearing |
                    ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoNavInputs)) {
        ImGuiIO* io = igGetIO();

        ImVec2 win_pos;
        win_pos.x = io->DisplaySize.x - 120;
        win_pos.y = 10;
        igSetWindowPos_Vec2(win_pos, ImGuiCond_Once);

        igText("FPS: %.1f", io->Framerate);
        igText(" dT: %.2fms", 1000 / io->Framerate);
    }

    igEnd();
}
void hud_eeprom(ImGuiKey open_key) {
    static MemoryEditor editor;
    static bool has_init = false;
    if (!has_init) {
        MemoryEditor_Init(&editor);
        editor.Open = false;
        has_init = true;
    }
    if (igIsKeyPressed_Bool(open_key, false)) editor.Open = !editor.Open;

    // TODO: Less hacky :)
    extern BYTE EEPROM_DATA[0x2000];

    if (editor.Open)
        MemoryEditor_DrawWindow(&editor, "EEPROM Editor", EEPROM_DATA, sizeof EEPROM_DATA, 0x000);
}
void hud_sram(ImGuiKey open_key) {
    static MemoryEditor editor;
    static bool has_init = false;
    if (!has_init) {
        MemoryEditor_Init(&editor);
        editor.Open = false;
        has_init = true;
    }
    if (igIsKeyPressed_Bool(open_key, false)) editor.Open = !editor.Open;

    // TODO: Less hacky :)
    extern LPBYTE SRAM_DATA;
    if (editor.Open)
        MemoryEditor_DrawWindow(&editor, "SRAM Editor", SRAM_DATA, 1024 * 1024, 0x0000);
}

void igHelpMarker(const char* text) {
    igTextDisabled("(?)");
    if (igIsItemHovered(0)) {
        igBeginTooltip();
        igPushTextWrapPos(igGetFontSize() * 35.0f);
        igTextUnformatted(text, NULL);
        igPopTextWrapPos();
        igEndTooltip();
    }
}

void AddSetting(const char* name, const char* help) {
    if (help) {
        igHelpMarker(help);
        igSameLine(0.0, -1.0);
    }
    igTextUnformatted(name, NULL);
    igNextColumn();
}

bool AddSettingInt(const char* name, const char* help, int* value) {
    igPushID_Int((int)value);

    AddSetting(name, help);

    bool changed = igInputInt("", value, 1, 1, ImGuiInputTextFlags_None);
    igNextColumn();
    igPopID();
    return changed;
}

bool AddSettingBool(const char* name, const char* help, bool* value) {
    igPushID_Int((int)value);

    AddSetting(name, help);

    bool changed = igCheckbox("", value);

    igNextColumn();
    igPopID();

    if (changed) save_current_config();
    return changed;
}

bool AddSettingString(const char* name, const char* help, char** value) {
    igPushID_Int((int)value);

    AddSetting(name, help);

    char buffer[512];
    snprintf(buffer, _countof(buffer), "%s", *value);
    buffer[_countof(buffer) - 1] = '\0';

    bool changed = igInputText("", buffer, _countof(buffer) - 1, ImGuiInputFlags_None, NULL, NULL);
    if (changed) {
        int newLen = strlen(buffer) + 1;
        *value = realloc(*value, newLen);
        memcpy(*value, buffer, newLen);
        save_current_config();
    }

    igNextColumn();
    igPopID();
    return changed;
}

bool AddSettingIPv4(const char* name, const char* help, unsigned int* value) {
    igPushID_Int((int)value);

    AddSetting(name, help);

    char buffer[17];

    snprintf(buffer, _countof(buffer), "%hhu.%hhu.%hhu.%hhu", (*value >> 24) & 0xff,
             (*value >> 16) & 0xff, (*value >> 8) & 0xff, (*value) & 0xff);
    buffer[_countof(buffer) - 1] = '\0';

    bool changed = igInputText("", buffer, _countof(buffer) - 1, ImGuiInputFlags_None, NULL, NULL);
    if (changed) {
        unsigned char a, b, c, d;
        size_t n = 0;
        if (sscanf(buffer, "%hhu.%hhu.%hhu.%hhu%n", &a, &b, &c, &d, &n) == 4 &&
            n == strlen(buffer)) {
            *value = (a << 24) | (b << 16) | (c << 8) | d;
            save_current_config();
        } else {
            changed = false;
        }
    }

    igNextColumn();
    igPopID();
    return changed;
}

bool igDipsw(const char* label, bool* value) {
    if (igGetCurrentWindow()->SkipItems) return false;

    const ImGuiID id = igGetID_Str(label);

    const float square_sz = igGetFrameHeight();
    ImVec2 tl;
    igGetCursorScreenPos(&tl);

    ImVec2 br;
    br.x = tl.x + square_sz / 1.5f;
    br.y = tl.y + square_sz;

    bool hovered, held;
    ImRect rect;
    rect.Min = tl;
    rect.Max = br;
    igItemSize_Rect(rect, igGetCurrentContext()->Style.FramePadding.y);
    if (!igItemAdd(rect, id, NULL, ImGuiItemFlags_None)) return false;

    bool pressed = igButtonBehavior(rect, id, &hovered, &held, ImGuiButtonFlags_None);
    if (pressed) {
        *value = !(*value);
        igMarkItemEdited(id);
    }

    ImDrawList* draw_list = igGetWindowDrawList();
    ImDrawList_AddRectFilled(draw_list, tl, br,
                             igGetColorU32_Col((held && hovered) ? ImGuiCol_FrameBgActive
                                               : hovered         ? ImGuiCol_FrameBgHovered
                                                                 : ImGuiCol_FrameBg,
                                               1),
                             0, ImDrawFlags_None);

    int pad = (int)(square_sz / 8.0f);

    tl.x += pad;
    tl.y += pad;
    br.x -= pad;
    br.y -= pad;
    if (*value) {
        ImDrawList_AddRectFilled(draw_list, tl, br, igGetColorU32_Col(ImGuiCol_CheckMark, 0.3f), 0,
                                 ImDrawFlags_None);
        tl.y += square_sz / 2.0f;
        ImDrawList_AddRectFilled(draw_list, tl, br, igGetColorU32_Col(ImGuiCol_CheckMark, 1), 0,
                                 ImDrawFlags_None);
    } else {
        br.y -= square_sz / 2.0f;
        ImDrawList_AddRectFilled(draw_list, tl, br, igGetColorU32_Col(ImGuiCol_CheckMark, 1), 0,
                                 ImDrawFlags_None);
    }

    return pressed;
}

void tab_settings_system() {
    igColumns(2, "SettingsSystem", true);
    igTextUnformatted("Name", NULL);
    igNextColumn();
    igTextUnformatted("Setting", NULL);
    igNextColumn();

    igSeparator();

    bool staticChanged = false;
    {
        igPushID_Int((int)&MiceConfig.sysconf.region);

        AddSetting("Region", NULL);

        bool jpn = !!(MiceConfig.sysconf.region & 1);
        bool usa = !!(MiceConfig.sysconf.region & 2);
        bool exp = !!(MiceConfig.sysconf.region & 4);
        bool chn = !!(MiceConfig.sysconf.region & 8);

        igTextUnformatted("JPN", NULL);
        igSameLine(0.0, -1.0);
        if (igRadioButton_Bool("##JPN", MiceConfig.sysconf.region == 1)) {
            MiceConfig.sysconf.region = 1;
            staticChanged = true;
            save_current_config();
        }
        igSameLine(0.0, -1.0);
        igSeparatorEx(ImGuiSeparatorFlags_Vertical);
        igSameLine(0.0, -1.0);
        igTextUnformatted("USA", NULL);
        igSameLine(0.0, -1.0);
        if (igRadioButton_Bool("##USA", MiceConfig.sysconf.region == 2)) {
            MiceConfig.sysconf.region = 2;
            staticChanged = true;
            save_current_config();
        }
        igSameLine(0.0, -1.0);
        igSeparatorEx(ImGuiSeparatorFlags_Vertical);
        igSameLine(0.0, -1.0);
        igTextUnformatted("EXP", NULL);
        igSameLine(0.0, -1.0);
        if (igRadioButton_Bool("##EXP", MiceConfig.sysconf.region == 4)) {
            MiceConfig.sysconf.region = 4;
            staticChanged = true;
            save_current_config();
        }
        igSameLine(0.0, -1.0);
        igSeparatorEx(ImGuiSeparatorFlags_Vertical);
        igSameLine(0.0, -1.0);
        igTextUnformatted("CHN", NULL);
        igSameLine(0.0, -1.0);
        if (igRadioButton_Bool("##", MiceConfig.sysconf.region == 8)) {
            MiceConfig.sysconf.region = 8;
            staticChanged = true;
            save_current_config();
        }

        igNextColumn();
        igPopID();
    }
    staticChanged |= AddSettingBool("Rental", NULL, &MiceConfig.sysconf.rental);
    staticChanged |=
        AddSettingString("Serial", "Main board serial number", &MiceConfig.sysconf.serial);
    if (staticChanged) build_eeprom();

    igSeparator();

    AddSetting("Resolution", NULL);

    const char* current_item;
    current_item = RESOLUTION_NAMES[(MiceConfig.sysconf.dipsw >> 4) & 0b111];
    if (igBeginCombo("##Resolution", current_item, ImGuiComboFlags_None)) {
        for (int i = 0; i < _countof(RESOLUTION_NAMES); i++) {
            bool is_selected = (current_item == RESOLUTION_NAMES[i]);
            if (igSelectable_Bool(RESOLUTION_NAMES[i], is_selected, ImGuiSelectableFlags_None,
                                  vec0)) {
                MiceConfig.sysconf.dipsw =
                    ((MiceConfig.sysconf.dipsw & 0b1'000'1111) | (i << 4)) & 0xff;
                save_current_config();
            }
            if (is_selected) igSetItemDefaultFocus();
        }

        igEndCombo();
    }
    igNextColumn();
    AddSetting("Orientation", NULL);
    current_item = ORIENTATION_NAMES[(MiceConfig.sysconf.dipsw >> 3) & 0b1];
    if (igBeginCombo("##Orientation", current_item, ImGuiComboFlags_None)) {
        for (int i = 0; i < _countof(ORIENTATION_NAMES); i++) {
            bool is_selected = (current_item == ORIENTATION_NAMES[i]);
            if (igSelectable_Bool(ORIENTATION_NAMES[i], is_selected, ImGuiSelectableFlags_None,
                                  vec0)) {
                MiceConfig.sysconf.dipsw =
                    ((MiceConfig.sysconf.dipsw & 0b1111'0'111) | (i << 3)) & 0xff;
                save_current_config();
            }
            if (is_selected) igSetItemDefaultFocus();
        }

        igEndCombo();
    }
    igNextColumn();

    AddSetting("DIP Switches", "Linked with the above two settings");
    for (int i = 0; i < 8; i++) {
        bool val = MiceConfig.sysconf.dipsw & (1 << i);
        igPushID_Int(i);
        if (i != 0) igSameLine(0.0, 2);
        if (igDipsw("", &val)) {
            MiceConfig.sysconf.dipsw = (MiceConfig.sysconf.dipsw & ~(1 << i)) | (val << i);
            save_current_config();
        }
        igPopID();
    }
    igNextColumn();

    igEndColumns();
}

void tab_settings_window() {
    igColumns(2, "SettingsWindow", true);
    igTextUnformatted("Name", NULL);
    igNextColumn();
    igTextUnformatted("Setting", NULL);
    igNextColumn();

    igSeparator();

    bool changed = false;

    changed |=
        AddSettingBool("Windowed", "Forces games into windowed mode", &MiceConfig.window.windowed);
    changed |= AddSettingBool("Borderless", "Should windowed games run borderless",
                              &MiceConfig.window.borderless);
    changed |= AddSettingInt("Adaptor", "Display adaptor to use", &MiceConfig.window.adaptor);
    changed |= AddSettingBool("Centre", "Centre the window. X and Y are used otherwise",
                              &MiceConfig.window.centre);
    igBeginDisabled(MiceConfig.window.centre);
    changed |= AddSettingInt("X", "Window position X", &MiceConfig.window.x);
    changed |= AddSettingInt("Y", "Window position Y", &MiceConfig.window.y);
    igEndDisabled();
    changed |=
        AddSettingBool("No sizing", "Don't change window resolution", &MiceConfig.window.nosize);
    igBeginDisabled(MiceConfig.window.nosize);
    changed |= AddSettingBool("Use DIPSW", "Use the DIP switches for resolution",
                              &MiceConfig.window.dipsw);
    igBeginDisabled(MiceConfig.window.dipsw);
    changed |= AddSettingInt("W", "Window width (0 to unset)", &MiceConfig.window.w);
    changed |= AddSettingInt("H", "Window height (0 to unset)", &MiceConfig.window.h);
    igEndDisabled();
    igEndDisabled();

    if (changed) save_current_config();

    igEndColumns();
}

void tab_settings_network() {
    igColumns(2, "SettingsNetwork", true);
    igTextUnformatted("Name", NULL);
    igNextColumn();
    igTextUnformatted("Setting", NULL);
    igNextColumn();

    igSeparator();

    bool networkChanged = false;
    networkChanged |= AddSettingIPv4("IP Address", NULL, &MiceConfig.network.ip_address);
    networkChanged |= AddSettingIPv4("Subnet Mask", NULL, &MiceConfig.network.subnet_mask);
    networkChanged |= AddSettingIPv4("Gateway", NULL, &MiceConfig.network.gateway);
    networkChanged |= AddSettingIPv4("Primary DNS", NULL, &MiceConfig.network.primary_dns);
    networkChanged |= AddSettingIPv4("Secondary DNS", NULL, &MiceConfig.network.secondary_dns);
    if (networkChanged) build_eeprom();

    igSeparator();
    igPushColumnsBackground();
    igTextUnformatted("Emulated DNS records. Routers must be pingable", NULL);
    igPopColumnsBackground();
    igSeparator();
    bool changed = false;
    changed |= AddSettingIPv4("naominet.jp", NULL, &MiceConfig.network.naominet_jp);
    changed |= AddSettingIPv4("ib.naominet.jp", NULL, &MiceConfig.network.ib_naominet_jp);
    changed |= AddSettingIPv4("aime.naominet.jp", NULL, &MiceConfig.network.aime_naominet_jp);
    changed |= AddSettingIPv4("tenporouter.loc", NULL, &MiceConfig.network.tenporouter_loc);
    changed |= AddSettingIPv4("bbrouter.loc", NULL, &MiceConfig.network.bbrouter_loc);
    changed |= AddSettingIPv4("mobirouter.loc", NULL, &MiceConfig.network.mobirouter_loc);
    changed |= AddSettingIPv4("dslrouter.loc", NULL, &MiceConfig.network.dslrouter_loc);
    if (changed) save_current_config();
    igSeparator();

    {
        AddSetting("MAC Address", "The first half of the MAC address is locked to SEGA's prefix");

        igTextUnformatted("D8:BB:C1:", NULL);

        unsigned int scan;

        char buffer[3] = { 0 };
        igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, vec10);
        snprintf(buffer, _countof(buffer), "%02X", (MiceConfig.network.mac >> 16) & 0xff);
        igSameLine(0.0, 0);
        igPushItemWidth(16);
        if (igInputText("##mac0", buffer, _countof(buffer), ImGuiInputTextFlags_CharsHexadecimal,
                        NULL, NULL)) {
            _snscanf_s(buffer, _countof(buffer), "%02x", &scan);
            MiceConfig.network.mac = (MiceConfig.network.mac & 0x00FFFF) | ((scan & 0xff) << 16);
            save_current_config();
        }
        igSameLine(0.0, 0);
        igTextUnformatted(":", NULL);
        snprintf(buffer, _countof(buffer), "%02X", (MiceConfig.network.mac >> 8) & 0xff);
        igSameLine(0.0, 0);
        igPushItemWidth(16);
        if (igInputText("##mac1", buffer, _countof(buffer), ImGuiInputTextFlags_CharsHexadecimal,
                        NULL, NULL)) {
            _snscanf_s(buffer, _countof(buffer), "%02x", &scan);
            MiceConfig.network.mac = (MiceConfig.network.mac & 0xFF00FF) | ((scan & 0xff) << 8);
            save_current_config();
        }
        igSameLine(0.0, 0);
        igTextUnformatted(":", NULL);
        snprintf(buffer, _countof(buffer), "%02X", MiceConfig.network.mac & 0xff);
        igSameLine(0.0, 0);
        igPushItemWidth(16);
        if (igInputText("##mac2", buffer, _countof(buffer), ImGuiInputTextFlags_CharsHexadecimal,
                        NULL, NULL)) {
            _snscanf_s(buffer, _countof(buffer), "%02x", &scan);
            MiceConfig.network.mac = (MiceConfig.network.mac & 0xFFFF00) | (scan & 0xff);
            save_current_config();
        }

        igPopStyleVar(1);

        igNextColumn();
    }

    igEndColumns();
}

void tab_settings_adavanced() {
    igColumns(2, "SettingsNetwork", true);
    igTextUnformatted("Name", NULL);
    igNextColumn();
    igTextUnformatted("Setting", NULL);
    igNextColumn();

    igSeparator();
    igPushColumnsBackground();
    igTextColored(DANGER_COL, "Enable or disable drivers. Disabling any is not recommended.");
    igPopColumnsBackground();
    igSeparator();

    bool changed;

    changed |= AddSettingBool("\\\\.\\columba", NULL, &MiceConfig.drivers.columba);
    changed |= AddSettingBool("\\\\.\\mxsram", NULL, &MiceConfig.drivers.mxsram);
    changed |= AddSettingBool("\\\\.\\mxsuperio", NULL, &MiceConfig.drivers.mxsuperio);
    changed |= AddSettingBool("\\\\.\\mxjvs", NULL, &MiceConfig.drivers.mxjvs);
    changed |= AddSettingBool("\\\\.\\mxhwreset", NULL, &MiceConfig.drivers.mxhwreset);
    changed |= AddSettingBool("\\\\.\\mxsmbus", NULL, &MiceConfig.drivers.mxsmbus);
    changed |= AddSettingBool("\\\\.\\mxparallel", NULL, &MiceConfig.drivers.mxparallel);
    changed |= AddSettingBool("\\\\.\\platform", NULL, &MiceConfig.drivers.platform);

    igSeparator();
    igPushColumnsBackground();
    igTextColored(DANGER_COL, "Enable or disable hooks. Disabling any is not recommended.");
    igPopColumnsBackground();
    igSeparator();

    changed |=
        AddSettingBool("logging", "Hooks logging functions and the eventlog to capture game logs",
                       &MiceConfig.hooks.logging);
    changed |= AddSettingBool("gui", "Required for overlays to work", &MiceConfig.hooks.gui);
    changed |= AddSettingBool("setupapi", "Required for SRAM", &MiceConfig.hooks.setupapi);
    changed |= AddSettingBool("commio", "Emulates COM devices", &MiceConfig.hooks.commio);
    changed |= AddSettingBool("io", "Hooks all file IO operations, including driver communication",
                              &MiceConfig.hooks.io);
    changed |= AddSettingBool("processes", "Controls process spawning to re-hook children",
                              &MiceConfig.hooks.processes);
    changed |= AddSettingBool("network", "Provides a virtual network environment for the game",
                              &MiceConfig.hooks.network);
    changed |=
        AddSettingBool("time", "Some binaries try to change the system time; this handles that",
                       &MiceConfig.hooks.time);
    changed |= AddSettingBool("registry", NULL, &MiceConfig.hooks.registry);
    changed |= AddSettingBool("drives", "Provides an emulation layer for the physical game SSD",
                              &MiceConfig.hooks.drives);

    igEndColumns();

    if (changed) save_current_config();
}

void tab_main_settings() {
    if (igBeginTabBar("SettingsTabs", 0)) {
        if (igBeginTabItem("System", NULL, 0)) {
            tab_settings_system();
            igEndTabItem();
        }
        if (igBeginTabItem("Window", NULL, 0)) {
            tab_settings_window();
            igEndTabItem();
        }
        if (igBeginTabItem("Network", NULL, 0)) {
            tab_settings_network();
            igEndTabItem();
        }
        if (igBeginTabItem("Advanced", NULL, 0)) {
            tab_settings_adavanced();
            igEndTabItem();
        }

        igEndTabBar();
    }
}

bool igKeyBindPopup(const char* name, int* boundKey) {
    if (igBeginPopupModal(name, NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        igText("Press any button");
        if (igButton("Cancel", vec0)) {
            igCloseCurrentPopup();
            *boundKey = -1;
            igEndPopup();
            return true;
        }

        // Skip the mouse buttons
        for (int i = 8; i < 0xff; i++) {
            if (GetAsyncKeyState(i) < 0) {
                *boundKey = i;
                igCloseCurrentPopup();
                igEndPopup();
                return true;
            }
        }

        igEndPopup();
    }
    return false;
}

void AddSettingButton(int board, int id) {
    char keyName[32];
    static int currentlyBinding;

    char pinInfo[32];
    snprintf(pinInfo, _countof(pinInfo), "CN3, Pins %d/%d", JVS_BUTTON_NAMES[id].pinNum,
             JVS_BUTTON_NAMES[id].pinNum + 1);
    AddSetting(JVS_BUTTON_NAMES[id].name, pinInfo);
    if (jvsKeybindings[board].buttons[id * 2] == 0) {
        igTextColored(DISABLED_COL, "None");
    } else {
        GetKeyNameTextA(MapVirtualKey(jvsKeybindings[board].buttons[id * 2], MAPVK_VK_TO_VSC) << 16,
                        keyName, _countof(keyName));
        igTextUnformatted(keyName, NULL);
    }
    igNextColumn();
    char name[16];
    char name2[16];
    char clear[16];
    char clear2[16];
    char invertName[16];
    char invertName2[16];
    snprintf(name, _countof(name), "Bind##Bind%d", id);
    snprintf(name2, _countof(name2), "Bind##Bind2p%d", id);
    snprintf(clear, _countof(clear), "Clear##Bind%d", id);
    snprintf(clear2, _countof(clear2), "Clear##Bind2p%d", id);
    snprintf(invertName, _countof(invertName), "##Invert%d", id);
    snprintf(invertName2, _countof(invertName2), "##Invert2p%d", id);

    if (igButton(name, vec0)) {
        currentlyBinding = 1;
        igOpenPopup_Str(name, ImGuiPopupFlags_None);
    }
    if (jvsKeybindings[board].buttons[id * 2]) {
        igSameLine(0, -1);
        if (igButton(clear, vec0)) {
            jvsKeybindings[board].buttons[id * 2] = 0;
            save_current_config();
        }
    }
    igNextColumn();
    igBeginDisabled(!jvsKeybindings[board].buttons[id * 2]);
    if (igCheckbox(invertName, &jvsKeybindings[board].invert[id * 2])) save_current_config();
    igEndDisabled();
    igNextColumn();

    if (jvsKeybindings[board].buttons[id * 2 + 1] == 0) {
        igTextColored(DISABLED_COL, "None");
    } else {
        GetKeyNameTextA(MapVirtualKey(jvsKeybindings[board].buttons[id * 2 + 1], MAPVK_VK_TO_VSC)
                            << 16,
                        keyName, _countof(keyName));
        igTextUnformatted(keyName, NULL);
    }

    igNextColumn();
    if (igButton(name2, vec0)) {
        currentlyBinding = 2;
        igOpenPopup_Str(name, ImGuiPopupFlags_None);
    }
    if (jvsKeybindings[board].buttons[id * 2 + 1]) {
        igSameLine(0, -1);
        if (igButton(clear2, vec0)) {
            jvsKeybindings[board].buttons[id * 2 + 1] = 0;
            save_current_config();
        }
    }
    igNextColumn();
    igBeginDisabled(!jvsKeybindings[board].buttons[id * 2 + 1]);
    if (igCheckbox(invertName2, &jvsKeybindings[board].invert[id * 2 + 1])) save_current_config();
    igEndDisabled();

    igNextColumn();

    int boundKey;
    if (igKeyBindPopup(name, &boundKey)) {
        if (boundKey != -1) {
            jvsKeybindings[board].buttons[id * 2 + (currentlyBinding - 1)] = boundKey;
            jvsKeybindings[board].notdefault = 1;
            save_current_config();
        }
        currentlyBinding = 0;
    }
}

void tab_jvs_board(int num) {
    igPushID_Int(num);
    igColumns(7, "LogicalButtons", true);

    igSetColumnWidth(0, 100);
    igSetColumnWidth(1, 70);
    igSetColumnWidth(2, 100);
    igSetColumnWidth(3, 60);
    igSetColumnWidth(4, 70);
    igSetColumnWidth(5, 100);
    igSetColumnWidth(6, 60);

    igTextUnformatted("Button", NULL);
    igNextColumn();
    igTextUnformatted("Player 1", NULL);
    igNextColumn();
    igNextColumn();
    igTextUnformatted("Invert", NULL);
    igNextColumn();
    igTextUnformatted("Player 2", NULL);
    igNextColumn();
    igNextColumn();
    igTextUnformatted("Invert", NULL);
    igNextColumn();

    igSeparator();

    for (int i = 0; i < _countof(JVS_BUTTON_NAMES); i++) AddSettingButton(num, i);

    igSeparator();
    AddSetting("Test", NULL);
    char keyName[16];
    if (jvsKeybindings[num].test == 0) {
        igTextColored(DISABLED_COL, "None");
    } else {
        GetKeyNameTextA(MapVirtualKey(jvsKeybindings[num].test, MAPVK_VK_TO_VSC) << 16, keyName,
                        _countof(keyName));
        igTextUnformatted(keyName, NULL);
    }
    igNextColumn();
    if (igButton("Bind##JvsTest", vec0)) igOpenPopup_Str("BindJvsTest", ImGuiPopupFlags_None);
    if (jvsKeybindings[num].test) {
        igSameLine(0, -1);
        if (igButton("Clear##ClearJvsTest", vec0)) {
            jvsKeybindings[num].test = 0;
            save_current_config();
        }
    }
    int boundKey;
    if (igKeyBindPopup("BindJvsTest", &boundKey))
        if (boundKey != -1) {
            jvsKeybindings[num].test = boundKey;
            save_current_config();
        }

    igEndColumns();

    igPopID();
}

void tab_system_buttons() {
    char keyName[32];
    int boundKey;

    igColumns(3, "SystemButtons", true);

    igTextUnformatted("Button", NULL);
    igNextColumn();
    igTextUnformatted("Key", NULL);
    igNextColumn();
    igNextColumn();
    igSeparator();

    igTextUnformatted("System Test", NULL);
    igNextColumn();
    if (MiceConfig.keys.test == 0) {
        igTextColored(DISABLED_COL, "None");
    } else {
        GetKeyNameTextA(MapVirtualKey(MiceConfig.keys.test, MAPVK_VK_TO_VSC) << 16, keyName,
                        _countof(keyName));
        igTextUnformatted(keyName, NULL);
    }
    igNextColumn();
    if (igButton("Bind##BindTest", vec0)) igOpenPopup_Str("BindSysTest", ImGuiPopupFlags_None);
    if (MiceConfig.keys.test) {
        igSameLine(0, -1);
        if (igButton("Clear##ClearTest", vec0)) {
            MiceConfig.keys.test = 0;
            save_current_config();
        }
    }
    igNextColumn();
    if (igKeyBindPopup("BindSysTest", &boundKey))
        if (boundKey != -1) {
            MiceConfig.keys.test = boundKey;
            save_current_config();
        }

    igTextUnformatted("System Service", NULL);
    igNextColumn();
    if (MiceConfig.keys.service == 0) {
        igTextColored(DISABLED_COL, "None");
    } else {
        GetKeyNameTextA(MapVirtualKey(MiceConfig.keys.service, MAPVK_VK_TO_VSC) << 16, keyName,
                        _countof(keyName));
        igTextUnformatted(keyName, NULL);
    }
    igNextColumn();
    if (igButton("Bind##BindService", vec0))
        igOpenPopup_Str("BindSysService", ImGuiPopupFlags_None);
    if (MiceConfig.keys.service) {
        igSameLine(0, -1);
        if (igButton("Clear##ClearService", vec0)) {
            MiceConfig.keys.service = 0;
            save_current_config();
        }
    }
    igNextColumn();
    if (igKeyBindPopup("BindSysService", &boundKey))
        if (boundKey != -1) {
            MiceConfig.keys.service = boundKey;
            save_current_config();
        }
}

void tab_main_keybinds() {
    if (igInputInt("Number of JVS boards", &MiceConfig.keys.board_count, 1, 1,
                   ImGuiInputTextFlags_None)) {
        if (MiceConfig.keys.board_count < 0) MiceConfig.keys.board_count = 0;
        if (MiceConfig.keys.board_count > JVS_IO_MAX) MiceConfig.keys.board_count = JVS_IO_MAX;
        save_current_config();
    }

    if (igBeginTabBar("JVSBoards", 0)) {
        for (int i = 0; i < MiceConfig.keys.board_count; i++) {
            char name[32];
            snprintf(name, _countof(name), "Board %d", i + 1);
            if (igBeginTabItem(name, NULL, 0)) {
                tab_jvs_board(i);
                igEndTabItem();
            }
        }
        if (igBeginTabItem("System", NULL, 0)) {
            tab_system_buttons();
            igEndTabItem();
        }
        igEndTabBar();
    }
}

void hud_control(ImGuiKey open_key) {
    static bool isOpen = false;
    static bool oldCursor;
    if (igIsKeyPressed_Bool(open_key, false)) {
        isOpen = !isOpen;
        if (isOpen) {
            CURSORINFO ci;
            GetCursorInfo(&ci);
            oldCursor = !!(ci.flags & CURSOR_SHOWING);
            ShowCursor(TRUE);
        } else {
            ShowCursor(oldCursor);
        }
    }

    if (!isOpen) return;

    igBegin("Micetools", NULL, 0);
    static bool haveFirstFrame = false;
    if (!haveFirstFrame) {
        ImVec2 size = { 600, 450 };
        igSetWindowSize_Vec2(size, 0);
        haveFirstFrame = true;
    }

    EnterCriticalSection(&logger_lock);

    if (igBeginTabBar("MainTabs", 0)) {
        if (igBeginTabItem("Keybinds", NULL, 0)) {
            igBeginChild_Str("Keybinds", vec0, FALSE, 0);
            tab_main_keybinds();
            igEndChild();
            igEndTabItem();
        }
        if (igBeginTabItem("Settings", NULL, 0)) {
            igBeginChild_Str("Settings", vec0, FALSE, 0);
            tab_main_settings();
            igEndChild();
            igEndTabItem();
        }
        igEndTabBar();
    }

    LeaveCriticalSection(&logger_lock);

    igEnd();
}

void __stdcall hud_gui(unsigned int hookType, IDirect3DDevice9* dev) {
    ShowCursor(1);

    static bool initialized = false;
    if (!initialized) {
        InitImGui(hookType, dev);
        initialized = true;
    }

    if (hookType == UI_HOOK_DX9) {
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        igNewFrame();
    } else if (hookType == UI_HOOK_GLUT) {
        ImGui_ImplGLUT_ReshapeFunc(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGLUT_NewFrame();
    }

    static bool showFps = false;
    if (igIsKeyPressed_Bool(ImGuiKey_F12, false)) showFps = !showFps;
    if (showFps) hud_fps();

    hud_eeprom(ImGuiKey_F11);
    hud_sram(ImGuiKey_F10);
    hud_control(ImGuiKey_F4);

    if (hookType == UI_HOOK_DX9) {
        igEndFrame();
        igRender();
        ImGui_ImplDX9_RenderDrawData(igGetDrawData());
    } else if (hookType == UI_HOOK_GLUT) {
        igRender();
        ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
    }
}

void setup_hud_gui() { register_gui_hook(&hud_gui); }
