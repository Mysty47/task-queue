#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345
#define BATCH_SIZE 100000

int main() {
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_in server_addr;
    int task;
    int received = 0;
    clock_t start = clock();

    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "Socket creation failed.\n");
        WSACleanup();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Connection to producer failed.\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    fprintf(stderr, "Connected to producer at %s:%d\n", SERVER_IP, SERVER_PORT);

    while (1) {
        int bytes = recv(sock, (char*)&task, sizeof(int), 0);
        if (bytes <= 0) {
            fprintf(stderr, "Producer disconnected. Exiting.\n");
            break;
        }

        received++;

        if (received % BATCH_SIZE == 0) {
            clock_t now = clock();
            double elapsed = (double)(now - start) / CLOCKS_PER_SEC;
            double tasks_per_sec = BATCH_SIZE / elapsed;
            double ms_per_task = 1000.0 / tasks_per_sec;

            printf("[Consumer] %d tasks | %.0f tasks/sec | %.3f ms/task\n",
                   received, tasks_per_sec, ms_per_task);

            start = now;
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
