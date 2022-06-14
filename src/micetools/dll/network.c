#include "network.h"

#include "../lib/mice/mice.h"

int WINAPI Fake_connect(SOCKET s, const SOCKADDR* name, int namelen) {
    ULONG addr = _byteswap_ulong(((SOCKADDR_IN*)name)->sin_addr);
    USHORT port = _byteswap_ushort(((SOCKADDR_IN*)name)->sin_port);
    log_info("connect", "%hhu.%hhu.%hhu.%hhu:%hu", (addr >> 24) & 0xff, (addr >> 16) & 0xff, (addr >> 8) & 0xff,
             addr & 0xff, port);
    return True_connect(s, name, namelen);
}

DWORD WINAPI FakeGetIfTable(PMIB_IFTABLE pIfTable, PULONG pdwSize, BOOL bOrder) {
    DWORD ret = TrueGetIfTable(pIfTable, pdwSize, bOrder);
    if (ret == NO_ERROR) {
        for (size_t i = 0; i < pIfTable->dwNumEntries; i++) {
            pIfTable->table[i].bPhysAddr[0] = 0x00;
            pIfTable->table[i].bPhysAddr[1] = 0xD0;
            pIfTable->table[i].bPhysAddr[2] = 0xF1;
        }
    }
    return ret;
}
const char* INTERCEPT_DNS[] = {
    "naominet.jp",       // Startup
    "ib.naominet.jp",    // Billing
    "aime.naominet.jp",  // Aime (duh)
    "tenporouter.loc",   // Routers
    "bbrouter.loc",      //
    "mobirouter.loc",    //
    "dslrouter.loc",     //
};

DNS_RECORDA dummy_record;

DNS_STATUS WINAPI FakeDnsQuery_A(PCSTR pszName, WORD wType, DWORD Options, PVOID pExtra, PDNS_RECORDA* ppQueryResults,
                                 PVOID* pReserved) {
    log_misc("dns", "DNS lookup for %s", pszName);
    if (ppQueryResults) {
        for (size_t i = 0; i < sizeof INTERCEPT_DNS / sizeof INTERCEPT_DNS[0]; i++) {
            if (strcmp(pszName, INTERCEPT_DNS[i]) == 0) {
                log_info("dns", "Replacing %s with 10.0.0.4", pszName);

                // We only support replacing at most one address, but that's all we'll ever need to!
                (*ppQueryResults) = &dummy_record;
                (*ppQueryResults)->pNext = NULL;
                (*ppQueryResults)->wType = DNS_TYPE_A;
                (*ppQueryResults)->Data.A.IpAddress = 0x0400000a;

                return ERROR_SUCCESS;
            }
        }
    }
    return TrueDnsQuery_A(pszName, wType, Options, pExtra, ppQueryResults, pReserved);
};

void hook_network() {
    hook("Ws2_32.dll", "connect", Fake_connect, (void**)&True_connect, 5);
    hook("Iphlpapi.dll", "GetIfTable", FakeGetIfTable, (void**)&TrueGetIfTable, 5);
    hook("Dnsapi.dll", "DnsQuery_A", FakeDnsQuery_A, (void**)&TrueDnsQuery_A, 5);
}
