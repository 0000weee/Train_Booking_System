#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define READ_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[READ_SIZE];
    char message[READ_SIZE];

    // 創建客戶套接字
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    // 設置服務器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // 轉換IP地址
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    // 連接服務器
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    while (1) {
        printf("Enter message: ");
        fgets(message, READ_SIZE, stdin);
        send(sock, message, strlen(message), 0);
        memset(buffer, 0, READ_SIZE);
        read(sock, buffer, READ_SIZE);
        printf("Server response: %s\n", buffer);
    }

    close(sock);
    return 0;
}
