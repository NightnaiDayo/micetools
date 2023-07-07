#include <Windows.h>

DWORD NvidiaDebugLevel = 0;

/* ============= AMD ============= */
__declspec(dllexport) int amvGetAvailableDriverEx_Amd(LPVOID pGpuHandle, DWORD dwGpuHandleSize) {
    return 0;
}
__declspec(dllexport) int amvGetAvailableDriver_Amd() {
    return amvGetAvailableDriverEx_Amd(NULL, 4);
}
__declspec(dllexport) int amvSetCustomDisplaySettingEx_Amd(LPVOID pGpuHandle,
                                                              DWORD dwGpuHandleSize,
                                                              LPWORD NV_SetCustomDisplaySetting) {
    return 0;
}
__declspec(dllexport) int amvSetCustomDisplaySetting_Amd(LPVOID pGpuHandle) {
    return amvSetCustomDisplaySettingEx_Amd(NULL, 4, pGpuHandle);
}

__declspec(dllexport) int amvSetDisplayModeEx_Amd(LPVOID pGpuHandle, DWORD dwGpuHandleSize,
                                                     BYTE displayMode) {
    return 0;
}
__declspec(dllexport) int amvSetDisplayMode_Amd(BYTE displayMode) {
    return amvSetDisplayModeEx_Amd(NULL, 4, displayMode);
}

/* ============= Nvidia ============= */
__declspec(dllexport) int amvFinalize_Nvidia() { return 0; }
__declspec(dllexport) int amvGetAvailableDriverEx_Nvidia(LPVOID pGpuHandle, DWORD dwGpuHandleSize) {
    return 0;
}
__declspec(dllexport) int amvGetAvailableDriver_Nvidia() {
    return amvGetAvailableDriverEx_Nvidia(NULL, 4);
}
__declspec(dllexport) int amvGetConnectedDisplaysEx_Nvidia(LPVOID pGpuHandle, DWORD dwGpuHandleSize,
                                                           LPBYTE displayType) {
    // TODO: Find a use of this to figure out what we should spoof!
    if (displayType) *displayType = 0;
    return 0;
}
__declspec(dllexport) int amvGetConnectedDisplays_Nvidia(LPBYTE displayType) {
    return amvGetConnectedDisplaysEx_Nvidia(NULL, 4, displayType);
}
__declspec(dllexport) int amvGetCurrentPstateEx_Nvidia(LPVOID pGpuHandle, DWORD dwGpuHandleSize,
                                                       DWORD* pPstate) {
    // TODO: Find a use of this to figure out what we should spoof!
    if (pPstate) *pPstate = 0;
    return 0;
}
__declspec(dllexport) int amvGetCurrentPstate_Nvidia(DWORD* pPstate) {
    return amvGetCurrentPstateEx_Nvidia(NULL, 4, pPstate);
}
__declspec(dllexport) int amvGetamvNvidiaDebugLevel() { return NvidiaDebugLevel; }
__declspec(dllexport) char* amvGetamvNvidiaVersion() {
    return "\namv_nvidia Ver.1.05 Build:Jul 22 2011 13:53:26\n";
}
__declspec(dllexport) int amvInitialize_Nvidia(LPVOID pGpuHandle, DWORD dwGpuHandleSize) {
    return 0;
}
__declspec(dllexport) int amvSetCustomDisplaySetting2ndEx_Nvidia(
    LPVOID pGpuHandle, DWORD dwGpuHandleSize, LPWORD NV_SetCustomDisplaySetting) {
    return 0;
}
__declspec(dllexport) int amvSetCustomDisplaySetting2nd_Nvidia(LPVOID pGpuHandle) {
    return amvSetCustomDisplaySetting2ndEx_Nvidia(NULL, 4, pGpuHandle);
}
__declspec(dllexport) int amvSetCustomDisplaySettingEx_Nvidia(LPVOID pGpuHandle,
                                                              DWORD dwGpuHandleSize,
                                                              LPWORD NV_SetCustomDisplaySetting) {
    return 0;
}
__declspec(dllexport) int amvSetCustomDisplaySetting_Nvidia(LPVOID pGpuHandle) {
    return amvSetCustomDisplaySettingEx_Nvidia(NULL, 4, pGpuHandle);
}
__declspec(dllexport) int amvSetDisplayModeEx_Nvidia(LPVOID pGpuHandle, DWORD dwGpuHandleSize,
                                                     BYTE displayMode) {
    return 0;
}
__declspec(dllexport) int amvSetDisplayMode_Nvidia(BYTE displayMode) {
    return amvSetDisplayModeEx_Nvidia(NULL, 4, displayMode);
}
__declspec(dllexport) int amvSetamvNvidiaDebugLevel(DWORD debugLevel) {
    NvidiaDebugLevel = debugLevel;
    return NvidiaDebugLevel;
}
