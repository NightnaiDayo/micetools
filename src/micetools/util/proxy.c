#include <Windows.h>
#include <Ws2tcpip.h>
#include <process.h>
#include <psapi.h>
#include <stdio.h>
#include <string.h>
#include <winbase.h>
#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Psapi.lib")

#define UPSTREAM_HOST "127.0.0.1"
#define UPSTREAM_PORT 6969

FILE *log_file;
SOCKET upstream;

void client_worker(SOCKET sock) {
    char recv_buf[512];
    while (1) {
        // Prompt
        int nout = recv(upstream, recv_buf, sizeof recv_buf, 0);
        send(sock, recv_buf, nout, 0);

        int nin = recv(sock, recv_buf, sizeof recv_buf, 0);
        if (nin == -1) break;

        fprintf(log_file, "sys->kc %d:", nin);
        for (int i = 0; i < nin; i++) {
            fprintf(log_file, " %02x", recv_buf[i]);
        }
        fprintf(log_file, ": %.*s", nin, recv_buf);
        fprintf(log_file, "\n");

        send(upstream, recv_buf, nin, 0);
        // Response
        nout = recv(upstream, recv_buf, sizeof recv_buf, 0);

        fprintf(log_file, "kc->sys %d:", nout);
        for (int i = 0; i < nout; i++) {
            fprintf(log_file, " %02x", recv_buf[i]);
        }
        fprintf(log_file, ": %.*s", nout, recv_buf);
        fprintf(log_file, "\n");

        send(sock, recv_buf, nout, 0);
    }
}

void killProcessByName(const char *exeName) {
    DWORD procList[0xffff];
    DWORD procCount;
    char processName[MAX_PATH];

    if (!EnumProcesses(procList, sizeof(procList), &procCount)) return;

    procCount = procCount / sizeof(DWORD);

    for (DWORD procIdx = 0; procIdx < procCount; procIdx++) {
        HANDLE procHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procList[procIdx]);
        GetProcessImageFileName(procHandle, processName, sizeof(processName));
        if (strstr(processName, exeName)) TerminateProcess(procHandle, 0);
        CloseHandle(procHandle);
    }
}
int main() {
    STARTUPINFO si = { sizeof si };
    PROCESS_INFORMATION pi;
    killProcessByName("mxkeychip-port-patched.exe");
    if (!CreateProcessA(NULL, "S:/mxkeychip-port-patched.exe", NULL, NULL, FALSE, 0, NULL, NULL,
                        &si, &pi)) {
        printf("Failed to spawn mxkc: %03x\n", GetLastError());
    }
    struct sockaddr_in name;

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = inet_addr(UPSTREAM_HOST);
    name.sin_port = htons(UPSTREAM_PORT);
    while (1) {
        SOCKET upstream = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (connect(upstream, &name, sizeof name) < 0) {
            closesocket(upstream);
            printf("Still waiting %d\n", GetLastError());
            continue;
        }
        puts("Connected!");
        closesocket(upstream);
        break;
    }

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = inet_addr("127.0.0.1");
    name.sin_port = htons(40106);
    bind(s, &name, sizeof name);

    listen(s, 1);

    while (1) {
        if (fopen_s(&log_file, "E:\\kc.log", "a"))
            if (fopen_s(&log_file, "D:\\kc.log", "a"))
                if (fopen_s(&log_file, "kc.log", "a")) printf("Failed to open log %d\n", errno);

        struct sockaddr_in addr;
        int addr_len;
        SOCKET sock = accept(s, &addr, &addr_len);
        fprintf(log_file, "Connection open %s:%d\n", inet_ntoa(addr.sin_addr), addr.sin_port);
        printf("Connection open %s:%d\n", inet_ntoa(addr.sin_addr), addr.sin_port);

        if (addr.sin_addr.s_addr == inet_addr("127.0.0.1")) {
            upstream = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            name.sin_family = AF_INET;
            name.sin_addr.s_addr = inet_addr(UPSTREAM_HOST);
            name.sin_port = htons(UPSTREAM_PORT);
            if (connect(upstream, &name, sizeof name) < 0)
                puts("mxkc connect failed!");
            else
                client_worker(sock);

            closesocket(upstream);
        }

        fprintf(log_file, "Connection close %s:%d\n", inet_ntoa(addr.sin_addr), addr.sin_port);
        printf("Connection close %s:%d\n", inet_ntoa(addr.sin_addr), addr.sin_port);

        closesocket(sock);
        fclose(log_file);
    }

    closesocket(s);
    WSACleanup();

    HANDLE mxkeychip = OpenProcess(PROCESS_TERMINATE, FALSE, pi.dwProcessId);
    TerminateProcess(mxkeychip, 1);
    CloseHandle(mxkeychip);
    return 0;
}
