#include <Windows.h>

int osuExecProcess(LPSTR command);
int osuMountPartition(LPCSTR lpTargetPath, LPCSTR lpszVolumeMountPoint);
int osuOpenDevice(DWORD dwDrive, LPHANDLE lphDevice);
int osuReadSector(HANDLE hDevice, DWORD dwLBA, DWORD dwSectors, LPVOID lpBuffer);
int osuSystemReboot(void);
int osuSetActivePartition(BYTE partition);
