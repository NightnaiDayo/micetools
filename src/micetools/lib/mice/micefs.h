#pragma once

#include <Windows.h>

#define RING_MOUNT_OS "C:"
#define RING_MOUNT_EXTEND "Y:"
#define RING_MOUNT_RECOVERY "Z:"
#define RING_MOUNT_SYSTEM "S:"
#define RING_MOUNT_ORIGINAL "O:"
#define RING_MOUNT_PATCH "P:"
#define RING_MOUNT_OS_UPDATE "W:"
#define RING_MOUNT_OS_DEFAULT_DRVIERS "V:"
#define RING_MOUNT_EXTEND_VOL "W:"
#define RING_MOUNT_EXTEND2_VOL "V:"
#define RING_MOUNT_GAME "X:"
#define RING_MOUNT_APM "I:"
#define RING_MOUNT_AM_LOG "L:"
#define RING_MOUNT_DVD "Q:"
#define RING_MOUNT_DEV "Z:"

typedef enum {
    MICE_FS_NS_WIN32,
    MICE_FS_NS_DEVICE,
} MICE_FS_NAMESPACE;

typedef struct MICE_FS_PATH MICE_FS_PATH, *PMICE_FS_PATH;
struct MICE_FS_PATH {
    MICE_FS_NAMESPACE m_dwNamespace;
    LPCSTR m_lpComponents;
};

typedef struct MICE_FS_LAYER MICE_FS_LAYER, *PMICE_FS_LAYER;
struct MICE_FS_LAYER {
    LPCSTR m_lpMountPoint;
    LPCSTR m_lpTargetPath;
    PMICE_FS_LAYER m_Next;
};

typedef struct {
    LPSTR m_lpPath;
    DWORD m_dwPath;
    DWORD m_dwComponentStart;
    DWORD m_dwWorkPtr;
    CHAR m_cSave;
} MICE_FS_PATH_TOK, *PMICE_FS_PATH_TOK;

/**
 * @brief Initialise the Mice filesystem emulation stack
 */
BOOL MiceFSInit(VOID);

/**
 * @brief Get the currently emulated CWD
 */
LPCSTR MiceFSGetCwd(VOID);
/**
 * @brief Set the currently emulated CWD
 *
 * @param lpNewCwd New CWD
 */
VOID MiceFSSetCwd(LPCSTR lpNewCwd);

/**
 * @brief Add a new layer to the emulation stack
 * Layers added later will be used preferentially.
 *
 * @param lpMountPoint Virtual path to mount this layer at
 * @param lpTargetPath Real location that will serve files for this layer
 * @return BOOL Success. Use GetLastError to identify the cause of failiure.
 */
BOOL MiceFSAddLayer(_In_z_ LPCSTR lpMountPoint, _In_z_ LPCSTR lpTargetPath);

/**
 * @brief Add layers for all drive letters backed by ".\dev\[letter]"
 * If used, this should be the first set of layers added, to act as a fallback
 *
 * @return BOOL Success. As this function adds 26 layers, and failiure could occur at any time,
 * failiure of this function should be considered a fatal failiure and MiceFS should be considered
 * to be in an indeterminate state.
 */
BOOL MiceFSAddDevLayers(VOID);

/**
 * @brief Add layers for all RingEdge system partitions.
 *
 * @return BOOL Success. See MiceFSAddDevLayers rearding when this is FALSE.
 */
BOOL MiceFSAddRingedgeLayers(BOOL bIsOsupdate);

/**
 * @brief Tokenise a path, returning each individual token (think, strtok)
 *
 * The first call to this function should provide lpPath and dwPath to initialise the operation.
 * Subsequence calls should provide NULL and 0 respectively. The "MiceFSPathTokNextA" macro aids in
 * this.
 *
 * Empty strings are defined as having no components, and as such NULL will be returned on the first
 * call.
 *
 * Multiple slashes in a row will be combined, rather than returning an empty component.
 *
 * Special handling is applied for paths starting with slashes. The first component returned will be
 * a 0-length string. This is the only time this function returns 0-length strings. The second
 * component will contain all but the first slash.
 * That is, \\.\demo will return "", "\.", "demo", NULL. Tihis is done to allow handling of path
 * namespaces.
 *
 * @param lpPath Path to split. This buffer will be modified!
 * @param dwPath Length of lpPath including the null termination
 * @param lpWork Pointer to a work structure to identify this path operation
 * @return LPSTR The path component, or NULL when the operation is completed.
 */
LPSTR MiceFSPathTokA(_In_reads_opt_z_(dwPath) LPSTR lpPath, _In_ DWORD dwPath,
                     _In_ PMICE_FS_PATH_TOK lpWork);
#define MiceFSPathTokNextA(lpWork) MiceFSPathTokA(NULL, 0, lpWork)

/**
 * @brief Return the full path in a tokenising buffer, starting with the current component
 *
 * @param lpWork Work buffer
 * @param lpPath Destination buffer
 * @param dwPath Size of lpPath
 */
VOID MiceFSPathTokRestA(_In_ PMICE_FS_PATH_TOK lpWork, _Out_writes_z_(dwPath) LPSTR lpPath,
                        _In_ DWORD dwPath);

/**
 * @brief Attempt to a path against a prefix
 *
 * @param lpPath The path to test
 * @param lpPrefix The prefix to match against
 * @param lpTail (Optional) Received the tail of lpPath after lpPrefix
 * @param dwTail Size of lptail
 */
BOOL MiceFSMatchPathA(_In_z_ LPCSTR lpPath, _In_z_ LPCSTR lpPrefix,
                      _Out_writes_z_(dwTail) LPSTR lpTail, _In_ DWORD dwTail);

/**
 * @brief Redirect a virtual path to a real path using Mice FS layers.
 *
 * The first layer to match with an existing file on disk will be selected.
 *
 * If redirection does not occur, the value of pszRedirected is unchanged.
 *
 * It is safe to (lpFilename, &lpFilename) as an argument pair.
 *
 * @param lpFileName Virtual filename
 * @param pszRedirected Redirected real filename
 * @return BOOL If redirection occured
 */
BOOL MiceFSRedirectPathA(_In_z_ LPCSTR lpFileName, _Out_ LPCSTR* pszRedirected);
/**
 * @brief Wide variant of MiceFSRedirectPathA
 */
BOOL MiceFSRedirectPathW(_In_z_ LPCWSTR lpFileName, _Out_ LPCWSTR* pszRedirected);
