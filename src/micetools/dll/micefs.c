#include "micefs.h"

#pragma comment(lib, "Shlwapi.lib")
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hooks/files.h"

static char miceFSCwd[MAX_PATH + 1];

/* Filesystem layer storage. When locating files, the first found result will be used. New items are
 * inserted at the start of the list. */
static MICE_FS_LAYER miceFsRoot = {
    .m_lpMountPoint = NULL,
    .m_lpTargetPath = NULL,
    .m_Next = NULL,
};

static volatile LONG miceFsLockCount = 0;
#define MiceTakeLockExclusive(lock)                                     \
    do {                                                                \
        LONG __miceLockCount = (lock);                                  \
        if (__miceLockCount == 0) {                                     \
            if (InterlockedCompareExchange(&(lock), -1, 0) == 0) break; \
        }                                                               \
        SwitchToThread();                                               \
    } while (1)
#define MiceReleaseLockExclusive(lock) \
    do {                               \
        (lock) = 0;                    \
    } while (0)

#define MiceTakeLockShared(lock)                                                             \
    do {                                                                                     \
        LONG __miceLockCount = (lock);                                                       \
        if (__miceLockCount != -1) {                                                         \
            if (InterlockedCompareExchange(&(lock), __miceLockCount + 1, __miceLockCount) == \
                __miceLockCount)                                                             \
                break;                                                                       \
        }                                                                                    \
        SwitchToThread();                                                                    \
    } while (1)
#define MiceReleaseLockShared(lock) \
    do {                            \
        (lock)--;                   \
    } while (0)

BOOL MiceFSInit(VOID) {
    // By not using MiceFSSetCwd we can save a buffer allocation
    _GetCurrentDirectoryA(sizeof miceFSCwd, miceFSCwd);
    return TRUE;
}

LPCSTR MiceFSGetCwd(VOID) { return miceFSCwd; }
VOID MiceFSSetCwd(LPCSTR lpNewCwd) { strcpy_s(miceFSCwd, sizeof miceFSCwd, lpNewCwd); }

BOOL MiceFSAddLayer(LPCSTR lpMountPoint, LPCSTR lpTargetPath) {
    DWORD dwMountLength = strlen(lpMountPoint) + 1;
    DWORD dwTargetlength = strlen(lpTargetPath) + 1;
    if (dwMountLength <= 1 || dwTargetlength <= 1) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    PMICE_FS_LAYER layer = malloc(sizeof *layer);
    if (layer == NULL) return FALSE;

    LPSTR szMountPoint = malloc(MAX_PATH + 1);
    if (szMountPoint == NULL) {
        free(layer);
        return FALSE;
    }
    LPSTR szTargetPath = malloc(MAX_PATH + 1);
    if (szTargetPath == NULL) {
        free(szMountPoint);
        free(layer);
        return FALSE;
    }

    ZeroMemory(szMountPoint, MAX_PATH + 1);
    if (!GetFullPathNameA(lpMountPoint, MAX_PATH + 1, szMountPoint, NULL)) {
        free(szTargetPath);
        free(szMountPoint);
        free(layer);
        return FALSE;
    }
    ZeroMemory(szTargetPath, MAX_PATH + 1);
    if (!GetFullPathNameA(lpTargetPath, MAX_PATH + 1, szTargetPath, NULL)) {
        free(szTargetPath);
        free(szMountPoint);
        free(layer);
        return FALSE;
    }

    // Mount and target are promoted to LPCSTR from LPSTR here
    layer->m_lpMountPoint = szMountPoint;
    layer->m_lpTargetPath = szTargetPath;

    // Commit ourselves to the FS
    MiceTakeLockExclusive(miceFsLockCount);
    layer->m_Next = miceFsRoot.m_Next;
    miceFsRoot.m_Next = layer;
    MiceReleaseLockExclusive(miceFsLockCount);

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

BOOL MiceFSAddDevLayers(VOID) {
    CHAR szMountPoint[4];
    CHAR szTargetPath[MAX_PATH + 1];
    ZeroMemory(szTargetPath, sizeof szTargetPath);
    strcpy_s(szTargetPath, sizeof szTargetPath, MiceIpcRelativePath("dev\\x\\"));
    DWORD index = strlen(szTargetPath) - 2;

    CHAR szCurrentDir[MAX_PATH + 1];
    _GetCurrentDirectoryA(sizeof szCurrentDir, szCurrentDir);

    // See below TODO
    if (szCurrentDir[0] == RING_MOUNT_APPDATA[0]) {
        log_error(plfBoot,
                  "Booting on the %s drive is currently unsupported. There's a potential for it to "
                  "do nasty things to your computer at the moment.",
                  RING_MOUNT_APPDATA);
        exit(0);
    }

    BOOL success = TRUE;
    for (int i = 0; i < 26; i++) {
        // TODO: DON'T SKIP C
        /**
         * Note: This is currently skipped because things such as DLLs, COM
         * registrations, etc. are all stored on C.
         * I have, however, run into an issue with a game (I forget which)
         * where _not_ hooking C caused issues.
         * This will likely require fine-grained whitelists.
         */
        if (i == 2) continue;
        // TODO: Cleaner way to not skip the current drive?
        // Currently skipped because of games that compile shaders at runtime (eg SDCM), however
        // this could cause issues if a game is running on a drive letter that we really need to
        // hook. _Are_ there any drive letters that would prove fatal? Appdata, probably. To
        // protect users from themselves, we're currently rejecting the use of Y: as a boot drive
        // entirely. This is ugly, and I hate it very much, but I have bigger fish to fry for now.
        if (i == szCurrentDir[0] - 'A' || i == szCurrentDir[0] - 'a') continue;

        szMountPoint[0] = 'A' + (char)i;
        szMountPoint[1] = ':';
        szMountPoint[2] = '\\';
        szMountPoint[3] = '\0';

        szTargetPath[index] = 'a' + (char)i;

        success &= MiceFSAddLayer(szMountPoint, szTargetPath);
    }
    return success;
}
BOOL MiceFSAddRingedgeLayers(BOOL bIsOsupdate) {
    /**
     * Note: Layers are searched in reverse order. Most lookups will be served by:
     * 1. original
     * 2. pathch
     * 3. extend
     * so those are added last!
     */
    BOOL success = TRUE;

    // TODO: Configurable!
    MiceFSAddLayer("R:", "H:\\Arcades\\USBs\\FiNALE_DL\\usb");
    MiceFSAddLayer("S:", "C:\\Users\\Nathan\\Desktop\\ac_research\\sega\\S-Drive");
    MiceFSAddLayer("C:\\System\\Execute", "C:\\Users\\Nathan\\Desktop\\ac_research\\sega\\S-Drive");

    // Windows file systems
    // TODO: See note about the C: drive above
    // success &= MiceFSAddLayer(RING_MOUNT_OS, MiceIpcRelativePath("mount\\os"));
    // success &= MiceFSAddLayer(RING_MOUNT_APPDATA, MiceIpcRelativePath("mount\\extend"));
    success &= MiceFSAddLayer(RING_MOUNT_RECOVERY, MiceIpcRelativePath("mount\\os_recovery"));

    // External media
    success &= MiceFSAddLayer(RING_MOUNT_APM, MiceIpcRelativePath("mount\\apm"));
    success &= MiceFSAddLayer(RING_MOUNT_AM_LOG, MiceIpcRelativePath("mount\\am_log"));
    success &= MiceFSAddLayer(RING_MOUNT_DVD, MiceIpcRelativePath("mount\\dvd"));
    success &= MiceFSAddLayer(RING_MOUNT_DEV, MiceIpcRelativePath("mount\\dev"));

    success &= MiceFSAddLayer("C:\\Documents and Settings\\AppUser",
                              MiceIpcRelativePath("mount\\appuser"));
    success &= MiceFSAddLayer("C:\\Documents and Settings\\SystemUser",
                              MiceIpcRelativePath("mount\\systemuser"));

    success &=
        MiceFSAddLayer("C:\\DOCUME~1\\SYSTEM~1\\LOCALS~1\\Temp\\", MiceIpcRelativePath("temp"));

    // TrueCrypt Volumes
    success &= MiceFSAddLayer(RING_MOUNT_SYSTEM, MiceIpcRelativePath("mount\\system"));
    if (bIsOsupdate) {
        success &= MiceFSAddLayer(RING_MOUNT_OS_UPDATE, MiceIpcRelativePath("mount\\os_update"));
        success &= MiceFSAddLayer(RING_MOUNT_OS_DEFAULT_DRVIERS,
                                  MiceIpcRelativePath("mount\\os_default_drivers"));
    } else {
        // success &= MiceFSAddLayer(RING_MOUNT_EXTEND_VOL,
        // MiceIpcRelativePath("mount\\extend/extend")); success &=
        // MiceFSAddLayer(RING_MOUNT_EXTEND2_VOL, MiceIpcRelativePath("mount\\extend/extend2"));
    }
    success &= MiceFSAddLayer(RING_MOUNT_PATCH, MiceIpcRelativePath("mount\\patch"));
    success &= MiceFSAddLayer(RING_MOUNT_ORIGINAL, MiceIpcRelativePath("mount\\original"));

    // geminifs
    success &= MiceFSAddLayer(RING_MOUNT_GAME, MiceIpcRelativePath("mount\\patch"));
    success &= MiceFSAddLayer(RING_MOUNT_GAME, MiceIpcRelativePath("mount\\original"));

    // Make directories that we're going to need
    make_dirs(MiceIpcRelativePath("mount\\appuser\\temp"));
    make_dirs(MiceIpcRelativePath("temp"));
    // Directories games read and write
    make_dirs(MiceIpcRelativePath("dev\\w"));
    make_dirs(MiceIpcRelativePath("dev\\v"));
    make_dirs(MiceIpcRelativePath("dev\\y"));
    make_dirs(MiceIpcRelativePath("dev\\x"));

    return success;
}

LPSTR MiceFSPathTokA(LPSTR lpPath, DWORD dwPath, PMICE_FS_PATH_TOK lpWork) {
    DWORD dwWorkPtr = 0;
    LPSTR lpPathStart;

    if (lpPath != NULL) {
        // A zero-length string has no tokens (as oppoed to a single empty token)
        if (lpPath[0] == '\0') {
            SetLastError(ERROR_SUCCESS);
            return NULL;
        }

        lpWork->m_lpPath = lpPath;
        lpPathStart = lpPath;
        lpWork->m_dwWorkPtr = dwWorkPtr;
        lpWork->m_dwPath = dwPath;
    } else {
        // We just finished processing the last token!
        if (lpWork->m_cSave == '\0') {
            SetLastError(ERROR_SUCCESS);
            return NULL;
        }

        dwWorkPtr = lpWork->m_dwWorkPtr;
        lpPath = lpWork->m_lpPath;
        lpPath[dwWorkPtr++] = lpWork->m_cSave;
        lpPathStart = lpPath + dwWorkPtr;
    }
    DWORD dwComponentStart = dwWorkPtr;

    for (; dwWorkPtr < lpWork->m_dwPath; dwWorkPtr++) {
        CHAR cWork = lpPath[dwWorkPtr];
        if (cWork == '\0' || cWork == '\\' || cWork == '/') {
            // Omit 0-length components, with the exception of the very first position in the
            // string, as that would indicate a double-backslash prefix, which is syntax for a path
            // namespace and should not be treated the same as a single backslash there.
            if (dwWorkPtr == dwComponentStart && dwWorkPtr != 0) {
                // Special case: Trailing slashes
                if (cWork == '\0') {
                    SetLastError(ERROR_SUCCESS);
                    return NULL;
                }
                dwComponentStart++;
                continue;
            }

            lpWork->m_dwComponentStart = dwComponentStart;
            lpWork->m_dwWorkPtr = dwWorkPtr;
            lpWork->m_cSave = cWork;
            lpPath[dwWorkPtr] = '\0';

            return lpPathStart;
        }
    }
    // Will only occur if lpPath was not null terminated within dwPath bytes. lpPathStart at this
    // point does not point to a null-terminated string, and therefore we cannot return it.
    // This is technically an error condition, so allow diligent users to catch this
    SetLastError(ERROR_BUFFER_OVERFLOW);
    return NULL;
}

VOID MiceFSPathTokRestA(PMICE_FS_PATH_TOK lpWork, LPSTR lpPath, DWORD dwPath) {
    lpWork->m_lpPath[lpWork->m_dwWorkPtr] = lpWork->m_cSave;
    memcpy_s(lpPath, dwPath, lpWork->m_lpPath + lpWork->m_dwComponentStart,
             lpWork->m_dwPath - lpWork->m_dwComponentStart);
}

BOOL MiceFSMatchPathA(LPCSTR lpPath, LPCSTR lpPrefix, LPSTR lpTail, DWORD dwTail) {
    CHAR szPath[MAX_PATH + 1];
    CHAR szPrefix[MAX_PATH + 1];

    strcpy_s(szPath, sizeof szPath, lpPath);
    strcpy_s(szPrefix, sizeof szPrefix, lpPrefix);

    MICE_FS_PATH_TOK pathTok;
    MICE_FS_PATH_TOK matchTok;
    LPSTR pathComponent = MiceFSPathTokA(szPath, _countof(szPath), &pathTok);
    LPSTR prefixComponent = MiceFSPathTokA(szPrefix, _countof(szPrefix), &matchTok);

    while (pathComponent && prefixComponent) {
        if (lstrcmpiA(pathComponent, prefixComponent) != 0) return FALSE;

        pathComponent = MiceFSPathTokNextA(&pathTok);
        prefixComponent = MiceFSPathTokNextA(&matchTok);
    }

    // We still had parts of the prefic left!
    if (prefixComponent) return FALSE;

    if (lpTail && dwTail) {
        if (pathComponent)
            MiceFSPathTokRestA(&pathTok, lpTail, dwTail);
        else
            lpTail[0] = '\0';
    }

    return TRUE;
}

inline static DWORD MiceFSGetFullPathNameA(LPCSTR lpFileName, DWORD nBufferLength, LPSTR lpBuffer,
                                           LPSTR* lpFilePart) {
    if (PathIsRelativeA(lpFileName)) {
        CHAR newPath[MAX_PATH + 1];
        PathCombineA(newPath, miceFSCwd, lpFileName);
        return GetFullPathNameA(newPath, nBufferLength, lpBuffer, lpFilePart);
    }
    return GetFullPathNameA(lpFileName, nBufferLength, lpBuffer, lpFilePart);
}

// TODO: Ewwwwwwww why are these globals (it's a bodge to not need free)
static char _redirected_path[MAX_PATH + 1];
static wchar_t _redirected_path_w[MAX_PATH + 1];
BOOL MiceFSRedirectPathA(LPCSTR lpFilename, LPCSTR* pszRedirected) {
    CHAR szExpanded[MAX_PATH + 1];
    if (!MiceFSGetFullPathNameA(lpFilename, sizeof szExpanded, szExpanded, NULL)) return FALSE;

    MiceTakeLockShared(miceFsLockCount);

    PMICE_FS_LAYER layer = miceFsRoot.m_Next;
    CHAR szTail[MAX_PATH + 1];
    wchar_t szwPath[MAX_PATH + 1];
    BOOL bFound = FALSE;
    while (layer) {
        if (MiceFSMatchPathA(szExpanded, layer->m_lpMountPoint, szTail, sizeof szTail)) {
            _redirected_path[0] = '\0';
            PathCombineA(_redirected_path, layer->m_lpTargetPath, szTail);
            if (pszRedirected) *pszRedirected = _redirected_path;

            MultiByteToWideChar(0, 0, _redirected_path, -1, szwPath, _countof(szwPath));

            if (FileExistsW(szwPath)) {
                MiceReleaseLockShared(miceFsLockCount);
                return TRUE;
            }
            bFound = TRUE;
        }

        layer = layer->m_Next;
    }
    MiceReleaseLockShared(miceFsLockCount);
    return bFound;
}

// TODO: This is kinda awful, because we do two roundtrips through string conversion...
BOOL MiceFSRedirectPathW(LPCWSTR lpFileName, LPCWSTR* pszRedirected) {
    char szFileName[MAX_PATH] = { 0 };
    WideCharToMultiByte(CP_ACP, 0, lpFileName, wcslen(lpFileName), szFileName, sizeof szFileName,
                        NULL, NULL);

    LPCSTR szRedirected;
    BOOL bRedirected = MiceFSRedirectPathA(szFileName, &szRedirected);

    if (!bRedirected) return FALSE;

    MultiByteToWideChar(CP_ACP, 0, szRedirected, -1, _redirected_path_w, sizeof _redirected_path_w);
    if (pszRedirected) *pszRedirected = _redirected_path_w;

    return bRedirected;
}

void MiceFSDebugPrintLayers(void) {
    MiceTakeLockShared(miceFsLockCount);

    PMICE_FS_LAYER lpLayer = &miceFsRoot;
    while (lpLayer = lpLayer->m_Next) {
        printf("%s -> %s\n", lpLayer->m_lpMountPoint, lpLayer->m_lpTargetPath);
    }

    MiceReleaseLockShared(miceFsLockCount);
}
BOOL MiceFSDebugTraceRedirectPathA(LPCSTR lpFilename, LPCSTR* pszRedirected) {
    printf("Tracing redirect of %s:\n", lpFilename);
    CHAR szExpanded[MAX_PATH + 1];
    if (!MiceFSGetFullPathNameA(lpFilename, sizeof szExpanded, szExpanded, NULL)) {
        printf(":: Redirect ended early with no action taken.");
        return FALSE;
    }

    MiceTakeLockShared(miceFsLockCount);

    PMICE_FS_LAYER layer = miceFsRoot.m_Next;
    CHAR szTail[MAX_PATH + 1];
    wchar_t szwPath[MAX_PATH + 1];
    BOOL bFound = FALSE;
    while (layer) {
        if (MiceFSMatchPathA(szExpanded, layer->m_lpMountPoint, szTail, sizeof szTail)) {
            _redirected_path[0] = '\0';
            PathCombineA(_redirected_path, layer->m_lpTargetPath, szTail);
            if (pszRedirected) *pszRedirected = _redirected_path;

            MultiByteToWideChar(0, 0, _redirected_path, -1, szwPath, _countof(szwPath));

            printf("-> %s\n", _redirected_path);
            if (FileExistsW(szwPath)) {
                MiceReleaseLockShared(miceFsLockCount);
                printf(":: Redirect ended with valid redirection found.");
                return TRUE;
            }
            bFound = TRUE;
        }

        layer = layer->m_Next;
    }

    MiceReleaseLockShared(miceFsLockCount);
    printf(":: Redirect ended with no redirection found.");
    return FALSE;
}
