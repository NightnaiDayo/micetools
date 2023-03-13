#pragma once
#include "../common.h"

static int(WINAPI* True_socket)(int domain, int type, int protocol);
static int(WINAPI* True_connect)(SOCKET s, const SOCKADDR* name, int namelen);
static int(WINAPI* True_bind)(SOCKET s, const SOCKADDR* addr, int namelen);
static int(WINAPI* True_sendto)(SOCKET s, const char* buf, int len, int flags,
                                const struct sockaddr* to, int tolen);

static DWORD(WINAPI* TrueGetIfTable)(PMIB_IFTABLE pIfTable, PULONG pdwSize, BOOL bOrder);

static DNS_STATUS(WINAPI* TrueDnsQuery_A)(PCSTR pszName, WORD wType, DWORD Options, PVOID pExtra,
                                          PDNS_RECORDA* ppQueryResults, PVOID* pReserved);

static INT(WSAAPI* TrueWSAStringToAddressA)(LPSTR AddressString, INT AddressFamily,
                                            LPWSAPROTOCOL_INFOA lpProtocolInfo,
                                            LPSOCKADDR lpAddress, LPINT lpAddressLength);

void hook_network();
