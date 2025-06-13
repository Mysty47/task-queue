#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 12345
#define MAX_CLIENTS 100

SOCKET clients[MAX_CLIENTS];
int client_count = 0;

void send_task_to_all_clients(int task) {
    for (int i = 0; i < client_count; ++i) {
        int result = send(clients[i], (char*)&task, sizeof(int), 0);
        if (result == SOCKET_ERROR) {
            closesocket(clients[i]);
            clients[i] = clients[--client_count];
            --i;
        }
    }
}

int main() {
    WSADATA wsaData;
    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;

    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        return 1;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        fprintf(stderr, "Socket creation failed.\n");
        WSACleanup();
        return 1;
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        fprintf(stderr, "Bind failed.\n");
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    listen(server_fd, MAX_CLIENTS);

    printf("=== PRODUCER STARTED ON PORT %d ===\n", PORT);

    int task = 0;
    int sent = 0;
    clock_t start = clock();
    double min_time = 999999;

    while (1) {
        fd_set readfds;
        struct timeval timeout = {0, 0};
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);

        int activity = select(0, &readfds, NULL, NULL, &timeout);
        if (activity > 0 && FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
            if (new_socket != INVALID_SOCKET && client_count < MAX_CLIENTS) {
                clients[client_count++] = new_socket;
                fprintf(stderr, "New consumer connected. Total clients: %d\n", client_count);
            }
        }

        if (client_count > 0) {
            send_task_to_all_clients(task++);
            sent++;
        }

        if (sent % 10000 == 0 && sent > 0) {
            clock_t now = clock();
            double elapsed = (double)(now - start) / CLOCKS_PER_SEC;
            double tasks_per_sec = 10000.0 / elapsed;
            double ms_per_task = 1000.0 / tasks_per_sec;
            if (ms_per_task < min_time) min_time = ms_per_task;

            printf("%.0f tasks/sec, %.5f ms/task (min: %.5f ms/task)\n",
                tasks_per_sec, ms_per_task, min_time);
            fflush(stdout);
            start = now;
        }
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}
