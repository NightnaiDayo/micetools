#include "network.h"

int WINAPI Fake_connect(SOCKET s, const SOCKADDR* name, int namelen) {
    ULONG addr = _byteswap_ulong(((SOCKADDR_IN*)name)->sin_addr.S_un.S_addr);
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

unsigned char SPOOF_IP[4] = { 10, 0, 0, 4 };
DNS_STATUS WINAPI FakeDnsQuery_A(PCSTR pszName, WORD wType, DWORD Options, PVOID pExtra, PDNS_RECORDA* ppQueryResults,
                                 PVOID* pReserved) {
    if (ppQueryResults) {
        for (size_t i = 0; i < sizeof INTERCEPT_DNS / sizeof INTERCEPT_DNS[0]; i++) {
            if (strcmp(pszName, INTERCEPT_DNS[i]) == 0) {
                log_info("dns", "Replacing %s with %hhu.%hhu.%hhu.%hhu", pszName, SPOOF_IP[0], SPOOF_IP[1], SPOOF_IP[2],
                         SPOOF_IP[3]);

                // We only support replacing at most one address, but that's all we'll ever need to!
                (*ppQueryResults) = &dummy_record;
                (*ppQueryResults)->pNext = NULL;
                (*ppQueryResults)->wType = DNS_TYPE_A;
                (*ppQueryResults)->Data.A.IpAddress =
                    (SPOOF_IP[0]) | (SPOOF_IP[1] << 8) | (SPOOF_IP[2] << 16) | (SPOOF_IP[3] << 24);

                return ERROR_SUCCESS;
            }
        }
    }
    log_warning("dns", "DNS passthrough for %s", pszName);
    return TrueDnsQuery_A(pszName, wType, Options, pExtra, ppQueryResults, pReserved);
};

INT WSAAPI FakeWSAStringToAddressA(LPSTR AddressString, INT AddressFamily, LPWSAPROTOCOL_INFOA lpProtocolInfo,
                                   LPSOCKADDR lpAddress, LPINT lpAddressLength) {
    log_misc("dns", "(WSA)DNS lookup for %s", AddressString);
    for (size_t i = 0; i < sizeof INTERCEPT_DNS / sizeof INTERCEPT_DNS[0]; i++) {
        if (strcmp(AddressString, INTERCEPT_DNS[i]) == 0) {
            log_info("dns", "(WSA)Replacing %s with %hhu.%hhu.%hhu.%hhu", AddressString, SPOOF_IP[0], SPOOF_IP[1],
                     SPOOF_IP[2], SPOOF_IP[3]);

            lpAddress->sa_family = AF_INET;
            // ... :)
            lpAddress->sa_data[2] = SPOOF_IP[0];
            lpAddress->sa_data[3] = SPOOF_IP[1];
            lpAddress->sa_data[4] = SPOOF_IP[2];
            lpAddress->sa_data[5] = SPOOF_IP[3];

            return ERROR_SUCCESS;
        }
    }
    log_warning("dns", "(WSA)DNS passthrough for %s", AddressString);
    return TrueWSAStringToAddressA(AddressString, AddressFamily, lpProtocolInfo, lpAddress, lpAddressLength);
}

void hook_network() {
    hook("Ws2_32.dll", "connect", Fake_connect, (void**)&True_connect, 5);
    hook("Ws2_32.dll", "WSAStringToAddressA", FakeWSAStringToAddressA, (void**)&TrueWSAStringToAddressA, 5);
    hook("Iphlpapi.dll", "GetIfTable", FakeGetIfTable, (void**)&TrueGetIfTable, 5);
    hook("Dnsapi.dll", "DnsQuery_A", FakeDnsQuery_A, (void**)&TrueDnsQuery_A, 5);
}
