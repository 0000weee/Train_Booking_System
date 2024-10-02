#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define MAX_EVENTS 10
#define READ_SIZE 1024
#define PORT 8080

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main() {
    int server_fd, client_fd, epoll_fd, event_count, i;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    struct epoll_event event, events[MAX_EVENTS];
    char buffer[READ_SIZE];

    // 創建服務器套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        error("socket failed");
    }

    // 設置服務器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 綁定端口
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        error("bind failed");
    }

    // 監聽連接
    if (listen(server_fd, 10) == -1) {
        error("listen failed");
    }

    // 創建 epoll 實例
    if ((epoll_fd = epoll_create1(0)) == -1) {
        error("epoll_create1 failed");
    }

    // 將服務器套接字加到 epoll 實例中
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        error("epoll_ctl failed");
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        // 等待事件發生
        event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (event_count == -1) {
            error("epoll_wait failed");
        }

        for (i = 0; i < event_count; i++) {
            if (events[i].data.fd == server_fd) {
                // 接受新連接
                if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
                    error("accept failed");
                }

                printf("New connection accepted\n");

                // 將新連接添加到 epoll 實例中
                event.events = EPOLLIN;
                event.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                    error("epoll_ctl ADD client failed");
                }
            } else {
                // 處理客戶端數據
                memset(buffer, 0, READ_SIZE);
                ssize_t bytes_read = read(events[i].data.fd, buffer, READ_SIZE);
                if (bytes_read <= 0) {
                    // 客戶端斷開連接
                    printf("Client disconnected\n");
                    close(events[i].data.fd);
                } else {
                    printf("Received: %s\n", buffer);
                    // 回寫數據给客戶端
                    write(events[i].data.fd, buffer, strlen(buffer));
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
