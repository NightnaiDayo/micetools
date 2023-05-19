#include "network.h"

int WINAPI Fake_connect(SOCKET s, const SOCKADDR* name, int namelen) {
    ULONG addr = _byteswap_ulong(((SOCKADDR_IN*)name)->sin_addr.S_un.S_addr);
    USHORT port = _byteswap_ushort(((SOCKADDR_IN*)name)->sin_port);
    // Poorly exclude nxauth and mxgcatcher.
    // TODO: better
    if (port != 40190 && port != 40110 && port != 40102 && port != 40103) {
        log_info(plfNetwork, "connect(%hhu.%hhu.%hhu.%hhu:%hu)", (addr >> 24) & 0xff,
                 (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff, port);
    }
    return True_connect(s, name, namelen);
}

int WINAPI Fake_bind(SOCKET s, const SOCKADDR* name, int namelen) {
    ULONG addr = _byteswap_ulong(((SOCKADDR_IN*)name)->sin_addr.S_un.S_addr);
    USHORT port = _byteswap_ushort(((SOCKADDR_IN*)name)->sin_port);
    log_info(plfNetwork, "bind(%hhu.%hhu.%hhu.%hhu:%hu)", (addr >> 24) & 0xff, (addr >> 16) & 0xff,
             (addr >> 8) & 0xff, addr & 0xff, port);
    return True_bind(s, name, namelen);
}

#define IF_INDEX 1
// Sega prefix
#define MAC_PREFIX_0 0xD8
#define MAC_PREFIX_1 0xBB
#define MAC_PREFIX_2 0xC1
DWORD WINAPI FakeGetIfTable(PMIB_IFTABLE pIfTable, PULONG pdwSize, BOOL bOrder) {
    log_misc(plfNetwork, "Injecting fake IfTable");

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
    row->dwPhysAddrLen = 6;
    row->bPhysAddr[0] = MAC_PREFIX_0;
    row->bPhysAddr[1] = MAC_PREFIX_1;
    row->bPhysAddr[2] = MAC_PREFIX_2;
    row->bPhysAddr[3] = (MiceConfig.network.mac >> 16) & 0xff;
    row->bPhysAddr[4] = (MiceConfig.network.mac >> 8) & 0xff;
    row->bPhysAddr[5] = MiceConfig.network.mac & 0xff;
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
    const unsigned int* address;
} dns;

dns INTERCEPT_DNS[] = {
    // Startup
    { "naominet.jp", &(MiceConfig.network.naominet_jp) },
    // Billing
    { "ib.naominet.jp", &(MiceConfig.network.ib_naominet_jp) },
    // Aime
    { "aime.naominet.jp", &(MiceConfig.network.aime_naominet_jp) },
    // Routers (ping targets)
    { "tenporouter.loc", &(MiceConfig.network.tenporouter_loc) },
    { "bbrouter.loc", &(MiceConfig.network.bbrouter_loc) },  // Must match tenporouter
    { "mobirouter.loc", &(MiceConfig.network.mobirouter_loc) },
    { "dslrouter.loc", &(MiceConfig.network.dslrouter_loc) },
};

DNS_RECORDA dummy_record;

DNS_STATUS WINAPI FakeDnsQuery_A(PCSTR pszName, WORD wType, DWORD Options, PVOID pExtra,
                                 PDNS_RECORDA* ppQueryResults, PVOID* pReserved) {
    if (ppQueryResults) {
        for (size_t i = 0; i < sizeof INTERCEPT_DNS / sizeof INTERCEPT_DNS[0]; i++) {
            if (strcmp(pszName, INTERCEPT_DNS[i].name) == 0) {
                printf("%08x\n", MiceConfig.network.naominet_jp);

                log_info(plfNetwork, "DNS Replacing %s with %08x", pszName,
                         *INTERCEPT_DNS[i].address);

                // We only support replacing at most one address, but that's all we'll ever need to!
                (*ppQueryResults) = &dummy_record;
                (*ppQueryResults)->pNext = NULL;
                (*ppQueryResults)->wType = DNS_TYPE_A;
                (*ppQueryResults)->Data.A.IpAddress = _byteswap_ulong(*INTERCEPT_DNS[i].address);

                return ERROR_SUCCESS;
            }
        }
    }
    log_info(plfNetwork, "DNS passthrough for %s", pszName);
    return TrueDnsQuery_A(pszName, wType, Options, pExtra, ppQueryResults, pReserved);
};

INT WSAAPI FakeWSAStringToAddressA(LPSTR AddressString, INT AddressFamily,
                                   LPWSAPROTOCOL_INFOA lpProtocolInfo, LPSOCKADDR lpAddress,
                                   LPINT lpAddressLength) {
    log_misc(plfNetwork, "WSA DNS lookup for %s", AddressString);
    for (size_t i = 0; i < sizeof INTERCEPT_DNS / sizeof INTERCEPT_DNS[0]; i++) {
        if (strcmp(AddressString, INTERCEPT_DNS[i].name) == 0) {
            log_info(plfNetwork, "WSA DNS Replacing %s with %08x", AddressString,
                     *INTERCEPT_DNS[i].address);

            lpAddress->sa_family = AF_INET;
            PULONG addr = &(((SOCKADDR_IN*)lpAddress)->sin_addr.S_un.S_addr);
            *addr = _byteswap_ulong(*INTERCEPT_DNS[i].address);
            return ERROR_SUCCESS;
        }
    }
    log_info(plfNetwork, "WSA DNS passthrough for %s", AddressString);
    return TrueWSAStringToAddressA(AddressString, AddressFamily, lpProtocolInfo, lpAddress,
                                   lpAddressLength);
}

INT WINAPI Fake_getaddrinfo(PCSTR pNodeName, PCSTR pServiceName, const ADDRINFOA* pHints,
                            PADDRINFOA* ppResult) {
    if (!pNodeName) return True_getaddrinfo(pNodeName, pServiceName, pHints, ppResult);

    for (size_t i = 0; i < sizeof INTERCEPT_DNS / sizeof INTERCEPT_DNS[0]; i++) {
        if (strcmp(pNodeName, INTERCEPT_DNS[i].name) == 0) {
            char szNewAddress[16];
            sprintf_s(szNewAddress, sizeof szNewAddress, "%d.%d.%d.%d",
                      (*INTERCEPT_DNS[i].address >> 24) & 0xff,
                      (*INTERCEPT_DNS[i].address >> 16) & 0xff,
                      (*INTERCEPT_DNS[i].address >> 8) & 0xff, *INTERCEPT_DNS[i].address & 0xff);
            szNewAddress[sizeof szNewAddress - 1] = '\0';

            log_info(plfNetwork, "GAI DNS Replacing %s with %s:%s", pNodeName, szNewAddress,
                     pServiceName);
            return True_getaddrinfo(szNewAddress, pServiceName, pHints, ppResult);
        }
    }

    // Exclude a few known services
    if (strcmp(pNodeName, "0.0.0.0") != 0 && strcmp(pNodeName, "127.0.0.0") != 0 &&
        strcmp(pNodeName, "240.0.0.0") != 0)
        log_info(plfNetwork, "GAI DNS passthrough for %s:%s", pNodeName, pServiceName);
    return True_getaddrinfo(pNodeName, pServiceName, pHints, ppResult);
}

int __stdcall Fake_socket(int domain, int type, int protocol) {
    int sock = True_socket(domain, type, protocol);
    log_trace(plfNetwork, "Creating new socket: %d/%s/%d -> %d", domain,
              type == 1   ? "SOCK_STREAM"
              : type == 2 ? "SOCK_DGRAM"
              : type == 3 ? "SOCK_RAW"
              : type == 4 ? "SOCK_RDM"
              : type == 5 ? "SOCK_SEQPACKET"
                          : "Unknown",
              protocol, sock);
    return sock;
}

static SOCKADDR pingSentTo;
static unsigned char pingInfo[4];

static SOCKADDR_IN toLocalhost = {
    .sin_addr.S_un.S_addr = 0x0100007f,
    .sin_family = AF_INET,
    .sin_port = 24,
    .sin_zero = 0,
};

int __stdcall Fake_sendto(SOCKET s, const char* buf, int len, int flags, const PSOCKADDR to,
                          int tolen) {
    // Hardcoded ICMP4 ping "detection"
    // TODO: Only do this if the socket is using the ICMP protocol
    if (len == 8 && buf[0] == 0x08 && buf[1] == 0x00) {
        uint32_t addr = ((PSOCKADDR_IN)to)->sin_addr.S_un.S_addr;

        memcpy(&pingSentTo, to, sizeof pingSentTo);

        uint16_t seq = _byteswap_ushort(((uint16_t*)buf)[3]);

        memcpy(pingInfo, buf + 4, 4);

        memcpy(&toLocalhost, to, sizeof toLocalhost);
        toLocalhost.sin_addr.S_un.S_addr = 0x0100007f;  // 127.0.0.1

        log_info(plfNetwork, "(probable) Ping to: %d.%d.%d.%d (%d). Redirecting to localhost",
                 addr & 0xff, (addr >> 8) & 0xff, (addr >> 16) & 0xff, addr >> 24,
                 ((PSOCKADDR_IN)to)->sin_port, seq);

        return True_sendto(s, buf, len, flags, (PSOCKADDR)&toLocalhost, tolen);
    }

    return True_sendto(s, buf, len, flags, to, tolen);
}

ULONG WINAPI FakeGetAdaptersAddresses(ULONG Family, ULONG Flags, PVOID Reserved,
                                      PIP_ADAPTER_ADDRESSES AdapterAddresses, PULONG SizePointer) {
    if (SizePointer == NULL || *SizePointer < sizeof *AdapterAddresses) {
        if (SizePointer != NULL) *SizePointer = sizeof *AdapterAddresses;
        return ERROR_BUFFER_OVERFLOW;
    }
    AdapterAddresses->PhysicalAddressLength = 6;
    AdapterAddresses->PhysicalAddress[0] = MAC_PREFIX_0;
    AdapterAddresses->PhysicalAddress[1] = MAC_PREFIX_1;
    AdapterAddresses->PhysicalAddress[2] = MAC_PREFIX_2;
    AdapterAddresses->PhysicalAddress[3] = (MiceConfig.network.mac >> 16) & 0xff;
    AdapterAddresses->PhysicalAddress[4] = (MiceConfig.network.mac >> 8) & 0xff;
    AdapterAddresses->PhysicalAddress[5] = MiceConfig.network.mac & 0xff;

    return ERROR_SUCCESS;
};

void hook_network() {
    hook("Ws2_32.dll", "connect", Fake_connect, (void**)&True_connect);
    hook("Ws2_32.dll", "socket", Fake_socket, (void**)&True_socket);
    hook("Ws2_32.dll", "bind", Fake_bind, (void**)&True_bind);
    hook("Ws2_32.dll", "getaddrinfo", Fake_getaddrinfo, (void**)&True_getaddrinfo);
    // hook("Ws2_32.dll", "sendto", Fake_sendto, (void**)&True_sendto);
    hook("Ws2_32.dll", "WSAStringToAddressA", FakeWSAStringToAddressA,
         (void**)&TrueWSAStringToAddressA);
    hook("Iphlpapi.dll", "GetIfTable", FakeGetIfTable, (void**)&TrueGetIfTable);
    hook("Dnsapi.dll", "DnsQuery_A", FakeDnsQuery_A, (void**)&TrueDnsQuery_A);
    hook("Iphlpapi.dll", "GetAdaptersAddresses", FakeGetAdaptersAddresses, NULL);
}
