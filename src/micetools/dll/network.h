#include <Winsock2.h>
#include <Windows.h>
#include <iphlpapi.h>
#include <windns.h>


static int(WINAPI* True_connect)(SOCKET s, const SOCKADDR* name, int namelen);

static DWORD(WINAPI* TrueGetIfTable)(PMIB_IFTABLE pIfTable, PULONG pdwSize, BOOL bOrder);

static DNS_STATUS(WINAPI* TrueDnsQuery_A)(PCSTR pszName, WORD wType, DWORD Options, PVOID pExtra,
                                          PDNS_RECORDA* ppQueryResults, PVOID* pReserved);

void hook_network();
