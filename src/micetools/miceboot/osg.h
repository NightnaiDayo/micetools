#include <Windows.h>

typedef enum colour {
    White = 0,
    Black = 1,
    Red = 2,
    Blue = 3,
    Green = 4,
    Grey = 5,
} colour;

typedef struct OSU_TOP {
    BOOL show_top;
    BOOL debugEnable;
    char *str1;
    char *str2;
    colour top_text_colour;
    DWORD top_text_font;
    CRITICAL_SECTION criticalSection;
} OSU_TOP;

typedef struct OSU_BOTTOM {
    colour backgroundColour;
    char *status_str;
    colour status_colour;
    DWORD status_font;
    DWORD status_use_counter;
    char *str2;
    colour power_colour;
    DWORD power_font;
    DWORD hide_power;
    char *line5;
    colour line5_colour;
    char *debug_text;
    colour debug_colour;
    DWORD only_sends_if;
} OSU_BOTTOM;

typedef struct OSU {
    HINSTANCE hInstance;
    int nShowCmd;
    HANDLE handle;
    unsigned int field3_0xc;
    OSU_TOP *s1;
    OSU_BOTTOM *s2;
} OSU;

typedef struct mxdraw {
    unsigned int hasState;
    OSU_TOP *s1;
    OSU_BOTTOM *s2;
    HINSTANCE hInstance;
    unsigned int nCmdShow;
    HWND hWnd;
    HBITMAP hBitmap;
    HDC hDc;
    unsigned int dispWidth;
    unsigned int dispHeight;
    HGDIOBJ font1;
    HGDIOBJ font2;
    HGDIOBJ font3;
} mxdraw;

mxdraw *mxdrawZero(mxdraw *this, OSU *osu);
void mxdrawCleanup(mxdraw *this);
void peekDispatchLoop(void);
void mxdrawRender(mxdraw *this);
int mxdrawSetup(mxdraw *this);
DWORD mxdrawThread(OSU *osu);

BOOL osgInitDraw(OSU *lpOsu);
BOOL osgExit(OSU *lpOsu);
