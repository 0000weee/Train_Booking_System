// handle_input() 處理byte>512
// handle_input() 不負責處理status不做哪些事情 的判斷

// select、poll
// multithread 關掉connect或是提供service
// accept_connect可能要改
// talnet
// 開好幾個talent連線，server有收到東西進來，寫回去
// 某client輸入111,哪server會回傳給同一個client 

// 選用poll

int main(){
    //create pollfd array,and initialize
    struct pollfd fds[MAX_CLIENTS + 1];
    memset(fds, 0, sizeof(fds));
    // Add server listen_fd to the poll set, [0].fd is listen, [0].event is pollin
    fds[0].fd = svr.listen_fd;
    fds[0].events = POLLIN;
    int nfds = 1;
    while (1) {
        int poll_count = poll(fds, nfds, -1);

        if (poll_count < 0) {
            ERR_EXIT("poll error");
        }

        // Check for new connections on the listening socket
        if (fds[0].revents & POLLIN) {
            conn_fd = accept_conn();
            if (conn_fd >= 0) {
                // Add new client connection to the poll set
                fds[nfds].fd = conn_fd;
                fds[nfds].events = POLLIN;
                nfds++;
                printf("New connection: fd %d\n", conn_fd);
            }
        }

        // Check for I/O on existing connections
        for (i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                int ret = handle_read(&requestP[fds[i].fd]);
                if (ret < 0) {
                    fprintf(stderr, "bad request from %s\n", requestP[fds[i].fd].host);
                    continue;
                }

                // Process request
#ifdef READ_SERVER
                sprintf(buf, "READ: %s : %s", "Header", requestP[fds[i].fd].buf);
#elif defined WRITE_SERVER
                sprintf(buf, "WRITE: %s : %s", "Header", requestP[fds[i].fd].buf);
#endif
                write(requestP[fds[i].fd].conn_fd, buf, strlen(buf));

                // Close and remove the connection
                close(requestP[fds[i].fd].conn_fd);
                free_request(&requestP[fds[i].fd]);
                fds[i] = fds[nfds - 1];  // Move last entry to the current slot
                nfds--;
                i--;  // Ensure we don't skip the next fd
            }
        }
    }

    close(svr.listen_fd);
    for (i = 0; i < TRAIN_NUM; i++)
        close(trains[i].file_fd);
}