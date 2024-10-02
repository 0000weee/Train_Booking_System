#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <poll.h>

#define PORT 8080
#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main() {
    int server_fd, client_fd, addrlen, new_socket, activity, i, sd;
    int client_socket[MAX_CLIENTS];
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];

    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds = 1;  // 初始只包含 server_fd

    // 初始化所有客戶端 socket 为 0（表示空閒）
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_socket[i] = 0;
    }

    // 創建服務器 socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        error("socket failed");
    }

    // 設置服務器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 绑定端口
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        error("bind failed");
    }

    // 監聽
    if (listen(server_fd, 10) < 0) {
        error("listen failed");
    }

    addrlen = sizeof(server_addr);
    printf("Server listening on port %d...\n", PORT);

    // 初始化 poll 文件描述符集
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    while (1) {
        // 使用 poll 監聽事件
        activity = poll(fds, nfds, -1);

        if (activity < 0 && errno != EINTR) {
            error("poll error");
        }

        // 檢查是否有新的連接
        if (fds[0].revents & POLLIN) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&addrlen)) < 0) {
                error("accept error");
            }

            printf("New connection accepted\n");

            // 將新連接加入到 client_socket 數組中
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    fds[nfds].fd = new_socket;
                    fds[nfds].events = POLLIN;
                    nfds++;
                    printf("Adding to list of sockets as %d\n", i);
                    break;
                }
            }
        }

        // 處理客戶端的數據
        for (i = 1; i < nfds; i++) {
            sd = fds[i].fd;
            if (fds[i].revents & POLLIN) {
                memset(buffer, 0, BUFFER_SIZE);
                int valread = read(sd, buffer, BUFFER_SIZE);
                if (valread == 0) {
                    // 客戶端斷開連接
                    printf("Client disconnected\n");
                    close(sd);
                    client_socket[i - 1] = 0;
                    fds[i] = fds[nfds - 1];
                    nfds--;
                    i--;  // 更新索引以避免跳過下一個
                } else {
                    // 回寫數據給客戶端
                    buffer[valread] = '\0';
                    printf("Received: %s\n", buffer);
                    send(sd, buffer, strlen(buffer), 0);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
