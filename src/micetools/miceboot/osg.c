#include "osg.h"

#include <memory.h>
#include <stdlib.h>

mxdraw *mxdrawZero(mxdraw *this, OSU *osu) {
    this->hasState = 0;
    this->s1 = NULL;
    this->s2 = NULL;
    this->hInstance = NULL;
    this->nCmdShow = 0;
    this->hWnd = NULL;
    this->hBitmap = NULL;
    this->hDc = NULL;
    this->dispWidth = 0;
    this->dispHeight = 0;
    this->font1 = NULL;
    this->font2 = NULL;
    this->font3 = NULL;
    ShowCursor(0);
    if (osu != NULL) {
        this->hInstance = osu->hInstance;
        this->nCmdShow = osu->nShowCmd;
        this->s1 = osu->s1;
        this->s2 = osu->s2;
        this->hasState = 1;
    }
    return this;
}

void mxdrawCleanup(mxdraw *this) {
    ShowCursor(1);
    DeleteObject(this->font1);
    DeleteObject(this->font2);
    DeleteObject(this->font3);
    DeleteObject(this->hBitmap);
    ReleaseDC(this->hWnd, this->hDc);
}

void peekDispatchLoop(void) {
    MSG msg;
    PeekMessageA(&msg, NULL, 0, 0, 0);
    while (msg.message != WM_QUIT) {
        if (PeekMessageA(&msg, NULL, 0, 0, 1)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }
}

COLORREF GetColour(colour col) {
    switch (col) {
        case Black:
            return 0x000000;
        case Red:
            return 0x0000ff;
        case Blue:
            return 0xc80000;
        case Green:
            return 0xb4ffb4;
        case Grey:
            return 0x646464;
        default:
            return 0xffffff;
    }
}

void mxdrawR1(mxdraw *this) {
    RECT rect;
    rect.bottom = this->dispHeight;
    rect.left = 0;
    rect.top = 0;
    rect.right = this->dispWidth;

    EnterCriticalSection(&this->s1->criticalSection);
    HBRUSH hbr = CreateSolidBrush(GetColour(this->s2->backgroundColour));
    LeaveCriticalSection(&this->s1->criticalSection);

    FillRect(this->hDc, &rect, hbr);
    DeleteObject(hbr);
}
void mxdrawR2(mxdraw *this) {
    if (!this->s1->show_top) return;

    switch (this->s1->top_text_font) {
        case 1:
            SelectObject(this->hDc, this->font3);
            break;
        case 2:
            SelectObject(this->hDc, this->font1);
            break;
        default:
            SelectObject(this->hDc, this->font2);
            break;
    }

    SetTextAlign(this->hDc, TA_CENTER + TA_BASELINE);
    SetTextColor(this->hDc, GetColour(this->s1->top_text_colour));
    SetBkMode(this->hDc, TRANSPARENT);

    EnterCriticalSection(&this->s1->criticalSection);
    if (this->s1->str1)
        TextOutA(this->hDc, this->dispWidth / 2, this->dispHeight / 6, this->s1->str1,
                 strlen(this->s1->str1));
    LeaveCriticalSection(&this->s1->criticalSection);
}
void mxdrawR3(mxdraw *this) {
    if (!this->s1->show_top) return;

    switch (this->s1->top_text_font) {
        case 1:
            SelectObject(this->hDc, this->font3);
            break;
        case 2:
            SelectObject(this->hDc, this->font1);
            break;
        default:
            SelectObject(this->hDc, this->font2);
            break;
    }

    SetTextAlign(this->hDc, TA_CENTER + TA_BASELINE);
    SetTextColor(this->hDc, GetColour(this->s1->top_text_colour));
    SetBkMode(this->hDc, TRANSPARENT);

    EnterCriticalSection(&this->s1->criticalSection);
    if (this->s1->str2)
        TextOutA(this->hDc, this->dispWidth / 2, this->dispHeight / 3, this->s1->str2,
                 strlen(this->s1->str2));
    LeaveCriticalSection(&this->s1->criticalSection);
}
void mxdrawR4(mxdraw *this) {
    static int _counter = 0;

    SetTextAlign(this->hDc, TA_CENTER + TA_BASELINE);
    switch (this->s2->status_font) {
        case 1:
            SelectObject(this->hDc, this->font3);
            break;
        case 2:
            SelectObject(this->hDc, this->font1);
            break;
        default:
            SelectObject(this->hDc, this->font2);
            break;
    }

    SetTextColor(this->hDc, GetColour(this->s2->status_colour));
    SetBkMode(this->hDc, TRANSPARENT);
    EnterCriticalSection(&this->s1->criticalSection);
    if (this->s2->status_use_counter == 0 || _counter > 1) {
        if (this->s2->status_str)
            TextOutA(this->hDc, this->dispWidth / 2, this->dispHeight / 2, this->s2->status_str,
                     strlen(this->s2->status_str));
    }
    LeaveCriticalSection(&this->s1->criticalSection);
    _counter = (_counter + 1) % 7;
}
void mxdrawR5(mxdraw *this) {
    static int _counter = 0;

    SetTextAlign(this->hDc, TA_CENTER + TA_TOP);
    switch (this->s2->power_font) {
        case 1:
            SelectObject(this->hDc, this->font3);
            break;
        case 2:
            SelectObject(this->hDc, this->font1);
            break;
        default:
            SelectObject(this->hDc, this->font2);
            break;
    }

    SetTextColor(this->hDc, GetColour(this->s2->power_colour));

    EnterCriticalSection(&this->s1->criticalSection);
    if (this->s2->hide_power != 1 || _counter > 1) {
        if (this->s2->str2)
            TextOutA(this->hDc, (int)this->dispWidth / 2, (this->dispHeight * 4) / 6,
                     this->s2->str2, strlen(this->s2->str2));
    }
    LeaveCriticalSection(&this->s1->criticalSection);
    _counter = (_counter + 1) % 7;
}
void mxdrawR6(mxdraw *this) {
    if (!this->s1->debugEnable) return;

    SetTextAlign(this->hDc, TA_LEFT + TA_BASELINE);
    SelectObject(this->hDc, this->font3);
    SetTextColor(this->hDc, GetColour(this->s2->debug_colour));

    EnterCriticalSection(&this->s1->criticalSection);
    if (this->s2->debug_text)
        TextOutA(this->hDc, this->dispWidth / 10, (this->dispHeight * 10) / 11,
                 this->s2->debug_text, strlen(this->s2->debug_text));
    LeaveCriticalSection(&this->s1->criticalSection);
}

void mxdrawR7(mxdraw *this) {
    SetTextAlign(this->hDc, TA_LEFT + TA_BASELINE);
    SelectObject(this->hDc, this->font3);
    SetTextColor(this->hDc, GetColour(this->s2->line5_colour));

    EnterCriticalSection(&this->s1->criticalSection);
    if (this->s2->line5)
        TextOutA(this->hDc, 0, this->dispHeight, this->s2->line5, strlen(this->s2->line5));
    LeaveCriticalSection(&this->s1->criticalSection);
}

void mxdrawRender(mxdraw *this) {
    this->dispWidth = GetSystemMetrics(0);
    this->dispHeight = GetSystemMetrics(1);
    if (this->dispWidth != 640 || this->dispHeight != 480) {
        // iVar1 = amSysDataGetDotClockTiming(640, 480, local_38);
        // if (iVar1 == 0) {
        //     amPlatformSetDisplayResolution(local_38, extraout_EDX, (ushort *)local_38);
        // }
    }

    mxdrawR1(this);  // Clear background
    mxdrawR2(this);  // s1, str1
    mxdrawR3(this);  // s1, str2
    mxdrawR4(this);  // s2, status_str
    mxdrawR5(this);  // s2, str2
    mxdrawR6(this);  // s2, debug_text
    mxdrawR7(this);  // s2, line5

    HDC hdc = GetDC(this->hWnd);
    BitBlt(hdc, 0, 0, this->dispWidth, this->dispHeight, this->hDc, 0, 0, SRCCOPY);
    ReleaseDC(this->hWnd, hdc);
    SetWindowPos(this->hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_CREATE) {
        SetTimer(hWnd, 15000, 0x80, NULL);
    } else if (uMsg == WM_DESTROY) {
        RemovePropA(hWnd, "prp");
        PostQuitMessage(0);
    } else if (uMsg == WM_TIMER) {
        mxdraw *this = (mxdraw *)GetPropA(hWnd, "prp");
        mxdrawRender(this);
        if (this->s2->only_sends_if == 1) {
            SendMessageA(hWnd, WM_DESTROY, 0, 0);
        }
    }
    return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

int mxdrawSetup(mxdraw *this) {
    if (this->hasState == 0) return 1;

    WNDCLASSEXA class;
    class.hInstance = this->hInstance;
    class.cbSize = 0x30;
    class.style = 3;
    class.lpfnWndProc = WndProc;
    class.cbClsExtra = 0;
    class.cbWndExtra = 0;
    class.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    class.hCursor = LoadCursorA(NULL, IDC_ARROW);
    class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    class.lpszMenuName = NULL;
    class.lpszClassName = "MxDraw";
    class.hIconSm = LoadIconA(NULL, IDI_APPLICATION);
    if (!RegisterClassExA(&class)) {
        if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return 2;
    }

    this->dispWidth = GetSystemMetrics(SM_CXSCREEN);
    this->dispHeight = GetSystemMetrics(SM_CYSCREEN);
    if (this->dispWidth != 640 || this->dispHeight != 480) {
        // TODO: This
        // setres_640_480();
        this->dispWidth = GetSystemMetrics(SM_CXSCREEN);
        this->dispHeight = GetSystemMetrics(SM_CYSCREEN);
    }

    this->hWnd =
        CreateWindowExA(0, "MxDraw", "MxDraw", WS_DISABLED | WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT,
                        this->dispWidth, this->dispHeight, NULL, NULL, this->hInstance, NULL);
    SetPropA(this->hWnd, "prp", this);

    this->font1 = CreateFontA((int)(this->dispHeight * 0.125f), 0, 0, 0, 700, FALSE, FALSE, FALSE,
                              0, 7, 0, 0, 1, "Tahoma bold");
    this->font2 = CreateFontA((int)(this->dispHeight * 0.09f), 0, 0, 0, 700, FALSE, FALSE, FALSE, 0,
                              7, 0, 0, 1, "Tahoma bold");
    this->font3 = CreateFontA((int)(this->dispHeight * 0.03125f), 0, 0, 0, 700, FALSE, FALSE, FALSE,
                              0, 7, 0, 0, 1, "Tahoma bold");

    ShowWindow(this->hWnd, this->nCmdShow);
    UpdateWindow(this->hWnd);
    HDC hdc = GetDC(this->hWnd);
    this->hBitmap = CreateCompatibleBitmap(hdc, this->dispWidth, this->dispHeight);
    this->hDc = CreateCompatibleDC(hdc);
    ReleaseDC(this->hWnd, hdc);
    SelectObject(this->hDc, this->hBitmap);
    return 0;
}

DWORD mxdrawThread(OSU *osu) {
    mxdraw *this = (mxdraw *)malloc(sizeof *this);
    if (this != NULL) mxdrawZero(this, osu);
    mxdrawSetup(this);
    peekDispatchLoop();
    if (this != NULL) {
        mxdrawCleanup(this);
        free(this);
    }
    ExitThread(1);
}

BOOL osgInitDraw(OSU *lpOsu) {
    OSU *lpParameter;
    HANDLE pvVar1;

    lpParameter = lpOsu;
    if (lpOsu == NULL) return FALSE;

    InitializeCriticalSection(&lpOsu->s1->criticalSection);
    EnterCriticalSection(&lpParameter->s1->criticalSection);
    lpParameter->s2->only_sends_if = 0;
    LeaveCriticalSection(&lpParameter->s1->criticalSection);
    pvVar1 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mxdrawThread, lpParameter, 0,
                          (LPDWORD)&lpOsu);
    lpParameter->handle = pvVar1;
    if (pvVar1 == NULL) {
        lpParameter->field3_0xc = 0;
        return FALSE;
    }
    lpParameter->field3_0xc = 1;
    return TRUE;
}

BOOL osgExit(OSU *this) {
    if (this->field3_0xc != 1) return FALSE;

    EnterCriticalSection(&this->s1->criticalSection);
    this->s2->only_sends_if = 1;
    LeaveCriticalSection(&this->s1->criticalSection);
    WaitForSingleObject(this->handle, INFINITE);
    CloseHandle(this->handle);
    DeleteCriticalSection(&this->s1->criticalSection);
    this->field3_0xc = 0;

    return TRUE;
}
