#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define MAX_CLIENTS 30
#define PORT 8080
#define BUFFER_SIZE 1024

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main() {
    int server_fd, client_fd, max_fd, activity, i, sd, valread;
    int client_socket[MAX_CLIENTS];
    int max_clients = MAX_CLIENTS;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    fd_set readfds;

    // 初始化所有客戶端 socket 为 0（表示空閒）
    for (i = 0; i < max_clients; i++) {
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

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        // 清空文件描述符集
        FD_ZERO(&readfds);

        // 將服務器 socket 添加到文件描述符集中
        FD_SET(server_fd, &readfds);
        max_fd = server_fd;

        // 添加现有的客戶端到文件描述符集中
        for (i = 0; i < max_clients; i++) {
            sd = client_socket[i];

            // 如果是有效的 socket 描述符，則添加到集合中
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }

            // 紀錄最大的文件描述符值
            if (sd > max_fd) {
                max_fd = sd;
            }
        }

        // 使用 select 監聽事件
        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            error("select error");
        }

        // 檢查是否有新的連接
        if (FD_ISSET(server_fd, &readfds)) {
            if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
                error("accept error");
            }

            printf("New connection accepted\n");

            // 將新連接加入到客戶端數組中
            for (i = 0; i < max_clients; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = client_fd;
                    printf("Added to client list at index %d\n", i);
                    break;
                }
            }
        }

        // 處理客戶端的數據
        for (i = 0; i < max_clients; i++) {
            sd = client_socket[i];

            if (FD_ISSET(sd, &readfds)) {
                memset(buffer, 0, BUFFER_SIZE);
                if ((valread = read(sd, buffer, BUFFER_SIZE)) == 0) {
                    // 客戶端斷開連接
                    printf("Client disconnected\n");
                    close(sd);
                    client_socket[i] = 0;
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
