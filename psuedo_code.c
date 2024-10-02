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
    int server_fd, client_fd, activity;
    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds = 1;
    // 初始化 poll 文件描述符集
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;
    while (1) {

    }    
}