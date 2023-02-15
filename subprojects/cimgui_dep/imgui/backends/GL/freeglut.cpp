#include "freeglut.h"

#include <Windows.h>

HMODULE hFreeGLUT = 0;

#define WRAP(procName, ret)                                   \
    if (!hFreeGLUT) hFreeGLUT = LoadLibraryA("freeglut.dll"); \
    if (hFreeGLUT == INVALID_HANDLE_VALUE) return ret;        \
    return ((decltype(procName)*)(GetProcAddress(hFreeGLUT, #procName)))

void FGAPIENTRY glutPassiveMotionFunc(void (*callback)(int, int)) {
    WRAP(glutPassiveMotionFunc, )(callback);
}
void FGAPIENTRY glutKeyboardUpFunc(void (*callback)(unsigned char, int, int)) {
    WRAP(glutKeyboardUpFunc, )(callback);
}
void FGAPIENTRY glutMouseFunc(void (*callback)(int, int, int, int)) {
    WRAP(glutMouseFunc, )(callback);
}
void FGAPIENTRY glutKeyboardFunc(void (*callback)(unsigned char, int, int)) {
    WRAP(glutKeyboardFunc, )(callback);
}
void FGAPIENTRY glutSpecialFunc(void (*callback)(int, int, int)) {
    WRAP(glutSpecialFunc, )(callback);
}
void FGAPIENTRY glutMotionFunc(void (*callback)(int, int)) { WRAP(glutMotionFunc, )(callback); }
int FGAPIENTRY glutGet(unsigned int query) { WRAP(glutGet, -1)(query); }
void FGAPIENTRY glutReshapeFunc(void (*callback)(int, int)) { WRAP(glutReshapeFunc, )(callback); }
int FGAPIENTRY glutGetModifiers(void) { WRAP(glutGetModifiers, -1)(); }

void FGAPIENTRY glutSpecialUpFunc(void (*callback)(int, int, int)) {
    WRAP(glutSpecialUpFunc, )(callback);
}

void FGAPIENTRY glutMouseWheelFunc(void (*callback)(int, int, int, int)) {
    WRAP(glutMouseWheelFunc, )(callback);
}
