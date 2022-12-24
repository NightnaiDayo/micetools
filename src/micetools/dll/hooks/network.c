#include "network.h"

int WINAPI Fake_connect(SOCKET s, const SOCKADDR* name, int namelen) {
    ULONG addr = _byteswap_ulong(((SOCKADDR_IN*)name)->sin_addr.S_un.S_addr);
    USHORT port = _byteswap_ushort(((SOCKADDR_IN*)name)->sin_port);
    log_info("connect", "%hhu.%hhu.%hhu.%hhu:%hu", (addr >> 24) & 0xff, (addr >> 16) & 0xff,
             (addr >> 8) & 0xff, addr & 0xff, port);
    return True_connect(s, name, namelen);
}

static uint8_t spoof_mac[6] = { 0xD8, 0xBB, 0xC1, 0x0A, 0x2F, 0x1D };
#define IF_INDEX 1
DWORD WINAPI FakeGetIfTable(PMIB_IFTABLE pIfTable, PULONG pdwSize, BOOL bOrder) {
    log_info("network", "Injecting fake IfTable");

    MIB_IFROW* row;
    uint32_t nbytes;

    if (pdwSize == NULL) return ERROR_INVALID_PARAMETER;

    nbytes = *pdwSize;
    *pdwSize = sizeof(*row) + sizeof(DWORD);

    if (pIfTable == NULL || nbytes < sizeof(*row) + sizeof(DWORD)) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    pIfTable->dwNumEntries = 1;

    row = pIfTable->table;
    memset(row, 0, sizeof(*row));

    wcscpy_s(row->wszName, _countof(row->wszName), L"RING2 Ethernet");
    row->dwIndex = IF_INDEX;
    row->dwType = IF_TYPE_ETHERNET_CSMACD;
    row->dwMtu = 4200;
    row->dwSpeed = 1000000000;
    row->dwPhysAddrLen = sizeof(spoof_mac);
    memcpy(row->bPhysAddr, spoof_mac, sizeof(spoof_mac));
    row->dwAdminStatus = 1;
    row->dwOperStatus = IF_OPER_STATUS_OPERATIONAL;

    return ERROR_SUCCESS;

    // DWORD ret = TrueGetIfTable(pIfTable, pdwSize, bOrder);
    // if (ret == NO_ERROR) {
    //     for (size_t i = 0; i < pIfTable->dwNumEntries; i++) {
    //         pIfTable->table[i].bPhysAddr[0] = 0x00;
    //         pIfTable->table[i].bPhysAddr[1] = 0xD0;
    //         pIfTable->table[i].bPhysAddr[2] = 0xF1;
    //     }
    // }
    // return ret;
}

typedef struct {
    char* name;
    unsigned char address[4];
} dns;
dns INTERCEPT_DNS[] = {
    // Startup
    { "naominet.jp", { 10, 79, 140, 238 } },
    // Billing
    { "ib.naominet.jp", { 10, 79, 140, 238 } },
    // Aime
    { "aime.naominet.jp", { 10, 79, 140, 238 } },
    // Routers (ping targets)
    { "tenporouter.loc", { 10, 79, 140, 238 } },
    { "bbrouter.loc", { 10, 79, 140, 238 } },  // Must match tenporouter
    { "mobirouter.loc", { 10, 79, 140, 238 } },
    { "dslrouter.loc", { 10, 79, 140, 238 } },
};

DNS_RECORDA dummy_record;

DNS_STATUS WINAPI FakeDnsQuery_A(PCSTR pszName, WORD wType, DWORD Options, PVOID pExtra,
                                 PDNS_RECORDA* ppQueryResults, PVOID* pReserved) {
    if (ppQueryResults) {
        for (size_t i = 0; i < sizeof INTERCEPT_DNS / sizeof INTERCEPT_DNS[0]; i++) {
            if (strcmp(pszName, INTERCEPT_DNS[i].name) == 0) {
#define spoof (INTERCEPT_DNS[i].address)
                log_info("dns", "Replacing %s with %hhu.%hhu.%hhu.%hhu", pszName, spoof[0],
                         spoof[1], spoof[2], spoof[3]);

                // We only support replacing at most one address, but that's all we'll ever need to!
                (*ppQueryResults) = &dummy_record;
                (*ppQueryResults)->pNext = NULL;
                (*ppQueryResults)->wType = DNS_TYPE_A;
                (*ppQueryResults)->Data.A.IpAddress =
                    (spoof[0]) | (spoof[1] << 8) | (spoof[2] << 16) | (spoof[3] << 24);

                return ERROR_SUCCESS;
#undef spoof
            }
        }
    }
    log_warning("dns", "DNS passthrough for %s", pszName);
    return TrueDnsQuery_A(pszName, wType, Options, pExtra, ppQueryResults, pReserved);
};

INT WSAAPI FakeWSAStringToAddressA(LPSTR AddressString, INT AddressFamily,
                                   LPWSAPROTOCOL_INFOA lpProtocolInfo, LPSOCKADDR lpAddress,
                                   LPINT lpAddressLength) {
    log_misc("dns", "(WSA)DNS lookup for %s", AddressString);
    for (size_t i = 0; i < sizeof INTERCEPT_DNS / sizeof INTERCEPT_DNS[0]; i++) {
        if (strcmp(AddressString, INTERCEPT_DNS[i].name) == 0) {
#define spoof (INTERCEPT_DNS[i].address)
            log_info("dns", "(WSA)Replacing %s with %hhu.%hhu.%hhu.%hhu", AddressString, spoof[0],
                     spoof[1], spoof[2], spoof[3]);

            lpAddress->sa_family = AF_INET;
            // ... :)
            lpAddress->sa_data[2] = spoof[0];
            lpAddress->sa_data[3] = spoof[1];
            lpAddress->sa_data[4] = spoof[2];
            lpAddress->sa_data[5] = spoof[3];

            return ERROR_SUCCESS;
#undef spoof
        }
    }
    log_warning("dns", "(WSA)DNS passthrough for %s", AddressString);
    return TrueWSAStringToAddressA(AddressString, AddressFamily, lpProtocolInfo, lpAddress,
                                   lpAddressLength);
}

void hook_network() {
    hook("Ws2_32.dll", "connect", Fake_connect, (void**)&True_connect, 5);
    hook("Ws2_32.dll", "WSAStringToAddressA", FakeWSAStringToAddressA,
         (void**)&TrueWSAStringToAddressA, 7);
    hook("Iphlpapi.dll", "GetIfTable", FakeGetIfTable, (void**)&TrueGetIfTable, 5);
    hook("Dnsapi.dll", "DnsQuery_A", FakeDnsQuery_A, (void**)&TrueDnsQuery_A, 5);
}
