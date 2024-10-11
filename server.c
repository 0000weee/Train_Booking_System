#include "server.h"

const unsigned char IAC_IP[3] = "\xff\xf4";
const char* file_prefix = "./csie_trains/train_";
const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";
const char* welcome_banner = "======================================\n"
                             " Welcome to CSIE Train Booking System \n"
                             "======================================\n";

const char* lock_msg = ">>> Locked.\n";
const char* exit_msg = ">>> Client exit.\n";
const char* cancel_msg = ">>> You cancel the seat.\n";
const char* full_msg = ">>> The shift is fully booked.\n";
const char* seat_booked_msg = ">>> The seat is booked.\n";
const char* no_seat_msg = ">>> No seat to pay.\n";
const char* book_succ_msg = ">>> Your train booking is successful.\n";
const char* invalid_op_msg = ">>> Invalid operation.\n";

#ifdef READ_SERVER
char* read_shift_msg = "Please select the shift you want to check [902001-902005]: ";
#elif defined WRITE_SERVER
char* write_shift_msg = "Please select the shift you want to book [902001-902005]: ";
char* write_seat_msg = "Select the seat [1-40] or type \"pay\" to confirm: ";
char* write_seat_or_exit_msg = "Type \"seat\" to continue or \"exit\" to quit [seat/exit]: ";
#endif

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance

int accept_conn(void);
// accept connection

static void getfilepath(char* filepath, int extension);
// get record filepath

int handle_read(request* reqP) {
    /*  Return value:
     *      1: read successfully
     *      0: read EOF (client down)
     *     -1: read failed
     *   Handle incomplete input
     */ 

    int r;
    char buf[MAX_MSG_LEN];
    memset(buf, 0, sizeof(buf));

    // Read in request from client (non-blocking)
    r = read(reqP->conn_fd, buf, sizeof(buf));

    if (r < 0) {
        // 若沒有資料可讀且不是致命錯誤，返回以繼續讀取
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 1; // Non-blocking read, no data yet
        }
        return -1;  // 發生錯誤
    }

    if (r == 0) {
        return 0; // EOF，客戶端關閉連線
    }

    // 累積讀取到的資料
    if (reqP->buf_len + r >= MAX_MSG_LEN) {
        fprintf(stderr, "Buffer overflow.\n");
        write(reqP->conn_fd, invalid_op_msg, strlen(invalid_op_msg));
        return -1;
    }
    memmove(reqP->buf + reqP->buf_len, buf, r);
    reqP->buf_len += r;

    // 檢查是否讀取到完整訊息
    char* p1 = strstr(reqP->buf, "\015\012"); // 查找 \r\n
    if (p1 == NULL) {
        p1 = strstr(reqP->buf, "\012"); // 查找 \n
    }

    if (p1 != NULL) {
        // 找到完整訊息，處理它
        size_t len = p1 - reqP->buf + 1;
        reqP->buf[len - 1] = '\0';  // 終止符號
        reqP->buf_len = 0;  // 清空緩存
        return 1;
    }

    // 還沒有完整訊息，等待更多資料
    return 1;
}


#ifdef READ_SERVER
int print_train_info(request *reqP) {
    int train_fd = reqP->booking_info.train_fd;  // 獲取 train_fd

    if (flock(train_fd, LOCK_EX) == -1) {  // LOCK_EX 取得排他鎖
        perror("Error acquiring lock");
        return 0;
    }
    char buf[MAX_MSG_LEN];  // 用於存放讀取到的數據

    // 重置檔案指針到檔案開頭，確保每次讀取都從頭開始
    if (lseek(train_fd, 0, SEEK_SET) == -1) {
        perror("lseek failed");
        return -1;
    }

    // 讀取檔案內容到 buf
    ssize_t n_read = read(train_fd, buf, MAX_MSG_LEN);
    if (n_read == -1) {
        perror("read failed");
        return -1;
    }

    // 確保 buf 以 NULL 結尾
    buf[n_read] = '\0';

    // 複製座位信息到 reqP->buf，以便之後發送給客戶端
    strncpy(reqP->buf, buf, MAX_MSG_LEN);

    if (flock(train_fd, LOCK_UN) == -1) {
        perror("Error releasing lock");
    }
    return 0;  // 成功返回 0
}


#else
int print_train_info(request *reqP) {
    /*
     * Booking info
     * |- Shift ID: 902001
     * |- Chose seat(s): 1,2
     * |- Paid: 3,4
     */
    char buf[MAX_MSG_LEN*3];
    char chosen_seat[MAX_MSG_LEN] = "";
    char paid[MAX_MSG_LEN] = "";
    for (int i = 0; i < SEAT_NUM; i++) {
        switch (reqP->booking_info.seat_stat[i]) { // 座位的狀態
            case UNKNOWN:
                break;
            case CHOSEN:
                // 如果 chosen_seat 不是空的，則先加上逗號，然後追加座位號
                if (strlen(chosen_seat) > 0) {
                    sprintf(chosen_seat + strlen(chosen_seat), ",%d", i + 1); // 座位號 i+1，座位號從 1 開始計算
                } else {
                    sprintf(chosen_seat, "%d", i + 1); // 首個座位不需要加逗號
                }
                break;
            case PAID:
                if (strlen(paid) > 0) {
                    sprintf(paid + strlen(paid), ",%d", i + 1); // 座位號 i+1，座位號從 1 開始計算
                } else {
                    sprintf(paid, "%d", i + 1); // 首個座位不需要加逗號
                }
                break;
            default:
                break;
        }
    }

    // 在這裡你可以使用 chosen_seat，例如打印或傳遞給其他函數
    printf("User:%d Chosen seats: %s\n", reqP->client_id, chosen_seat);
    printf("User:%d Chosen seats: %s\n", reqP->client_id, paid);

    memset(buf, 0, sizeof(buf));
    sprintf(buf, "\nBooking info\n"
                 "|- Shift ID: %d\n"
                 "|- Chose seat(s): %s\n"
                 "|- Paid: %s\n\n"
                 ,reqP->booking_info.shift_id, chosen_seat, paid);

    write(reqP->conn_fd, buf, strlen(buf));
    return 0;
}

int fully_booked(int fd) {
    char buffer[FILE_LEN];
    memset(buffer, 0, FILE_LEN);

    // 先將檔案的讀取指針移回到文件的開頭
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("lseek failed");
        return 0;
    }

    // 讀取文件中的座位資料到緩衝區
    ssize_t n_read = read(fd, buffer, FILE_LEN);
    if (n_read < 0) {
        perror("fully booked read failed");
        return 0;
    }

    // 檢查座位是否已完全預訂
    for (int i = 0; i < n_read; i++) {
        if (buffer[i] == '0') {  // '0' 表示座位未被選擇或未付款
            return 0;  // 尚有座位未被選擇
        }
    }
    
    return 1;  // 所有座位均已被選擇或付款
}

void handle_shift_input(request *reqP){
    
    int train_id = atoi(reqP->buf);
    if(train_id<TRAIN_ID_START || train_id > TRAIN_ID_END){
        write(reqP->conn_fd, write_shift_msg, strlen(write_shift_msg));
    }else{
        // update booking_info
        reqP->booking_info.shift_id = train_id;
        reqP->booking_info.train_fd = trains[train_id - TRAIN_ID_START].file_fd;   

        if(fully_booked(reqP->booking_info.train_fd)){
            write(reqP->conn_fd, full_msg, strlen(full_msg));
            write(reqP->conn_fd, write_shift_msg, strlen(write_shift_msg));
        }else{ 
            print_train_info(reqP);

            reqP->status = SEAT;
            write(reqP->conn_fd, write_seat_msg, strlen(write_seat_msg));
        }
        
    }
}

void Write_Bit_To_Fd(int fd, int seat_id, enum SEAT seat_status) {
    char buffer[FILE_LEN];
    memset(buffer, 0, FILE_LEN);

    // 先將檔案的讀取指針移回到文件的開頭
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("lseek failed");
        return;
    }

    // 讀取文件中的座位資料到緩衝區
    if (read(fd, buffer, FILE_LEN) < 0) {
        perror("Read failed");
        return;
    }

    // 找到第 seat_id 個非空白字符
    int i, j = 0;
    for (i = 0; i < FILE_LEN; i++) {
        if (buffer[i] == ' ' || buffer[i] == '\n') {
            continue;
        }
        j++;
        if (j == seat_id) {
            break;
        }
    }

    // 檢查是否找到對應座位
    if (i > FILE_LEN) {
        printf("Invalid seat_id\n");
        return;
    }

    // 根據 seat_status 更新緩衝區的內容
    switch (seat_status) {
        case UNKNOWN:
            buffer[i] = '0';
            break;
        case CHOSEN:
            buffer[i] = '1';
            break;
        case PAID:
            buffer[i] = '2';
            break;
        default:
            printf("Invalid seat status\n");
            return;
    }

    // 將指針移回該座位的位置並寫入單個字元
    if (lseek(fd, i, SEEK_SET) == -1) {
        perror("lseek failed");
        return;
    }

    // 寫入更新的座位狀態
    if (write(fd, &buffer[i], 1) == -1) {
        perror("write failed");
    }
}


enum SEAT Read_Bit_From_Fd(int fd, int seat_id) {
    char buffer[FILE_LEN];
    memset(buffer, 0, FILE_LEN);

    // 先將檔案的讀取指針移回到文件的開頭
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("lseek failed");
        return UNKNOWN;
    }

    // 讀取文件中的座位資料到緩衝區
    if (read(fd, buffer, FILE_LEN) < 0) {
        perror("Read bit from fd failed");
        return UNKNOWN;
    }

    // 遍歷緩衝區，查找第 seat_id 個非空白字符
    int i, j = 0;
    for (i = 0; i < FILE_LEN; i++) {
        // 跳過空格和換行符號
        if (buffer[i] == ' ' || buffer[i] == '\n') {
            continue;
        }
        
        // 計數非空白字符
        j++;
        
        // 當找到第 seat_id 個非空白字符時退出循環
        if (j == seat_id) {
            break;
        }
    }

    // 檢查是否找到座位，如果 i 已經超過 FILE_LEN 則 seat_id 無效
    if (i > FILE_LEN) {
        printf("Invalid seat_id\n");
        return UNKNOWN;
    }

    // 返回對應的 enum SEAT 狀態
    switch (buffer[i]) {
        case '0':
            printf("UNKNOWN \n");
            return UNKNOWN;
        case '1':
            printf("CHOSEN \n");
            return CHOSEN;
        case '2':
            printf("PAID \n");
            return PAID;
        default:
            printf("Invalid seat status\n");
            return UNKNOWN;
    }
}


void handle_seat_input(request *reqP) {
    if (strcmp("pay", reqP->buf) == 0) {
        if (reqP->booking_info.num_of_chosen_seats == 0) {
            write(reqP->conn_fd, no_seat_msg, strlen(no_seat_msg));
            print_train_info(reqP);
            write(reqP->conn_fd, write_seat_msg, strlen(write_seat_msg));
        } else {
            // 更新選中的座位為已支付狀態
            for (int i = 0; i < SEAT_NUM; i++) {  // SEAT_NUM 應為 40
                if (reqP->booking_info.seat_stat[i] == CHOSEN) {
                    printf("%d change status to PAID\n", i + 1);  // 調試用的輸出
                    reqP->booking_info.seat_stat[i] = PAID;
                    Write_Bit_To_Fd(reqP->booking_info.train_fd, i + 1, PAID);  // 更新文件中的座位狀態
                }
            }
            // 清空選擇狀態，避免再次選擇錯誤
            reqP->booking_info.num_of_chosen_seats = 0;
            print_train_info(reqP);     
            write(reqP->conn_fd, book_succ_msg, strlen(book_succ_msg));    
            write(reqP->conn_fd, write_seat_or_exit_msg, strlen(write_seat_or_exit_msg));
            reqP->status = BOOKED;
        }
    } else {
        int seat_id = atoi(reqP->buf);  // 將輸入的座位號轉換為數字
        if (seat_id < 1 || seat_id > SEAT_NUM) {  // 檢查座位是否在有效範圍內
            write(reqP->conn_fd, write_seat_msg, strlen(write_seat_msg));
            return;
        }

        // 檢查座位狀態
        switch (Read_Bit_From_Fd(reqP->booking_info.train_fd, seat_id)) {
            case UNKNOWN:
                reqP->booking_info.seat_stat[seat_id - 1] = CHOSEN;  // 更新 seat_stat 狀態，seat_id - 1 對應數組索引
                reqP->booking_info.num_of_chosen_seats++;
                print_train_info(reqP);
                Write_Bit_To_Fd(reqP->booking_info.train_fd, seat_id, CHOSEN);  // 更新 train 文件
                break;

            case CHOSEN:
                if (reqP->booking_info.seat_stat[seat_id - 1] == CHOSEN) { //User Has been Chosen
                    reqP->booking_info.seat_stat[seat_id - 1] = UNKNOWN;
                    reqP->booking_info.num_of_chosen_seats--;
                    write(reqP->conn_fd, cancel_msg, strlen(cancel_msg));
                    print_train_info(reqP);
                    Write_Bit_To_Fd(reqP->booking_info.train_fd, seat_id, UNKNOWN);
                } else {
                    write(reqP->conn_fd, lock_msg, strlen(lock_msg));  // 提示座位被鎖定
                }
                break;

            case PAID:
                write(reqP->conn_fd, seat_booked_msg, strlen(seat_booked_msg));  // 座位已支付，無法再次選擇
                break;

            default:
                break;
        }
        write(reqP->conn_fd, write_seat_msg, strlen(write_seat_msg));
    }
}



void handle_booked_input(request *reqP){
    if(strcmp(reqP->buf, "exit") == 0){
        write(reqP->conn_fd, exit_msg, strlen(exit_msg));
        close(reqP->conn_fd);
        free_request(reqP);
    }else if(strcmp(reqP->buf, "seat") == 0){
        reqP->status = SEAT;
        write(reqP->conn_fd, write_seat_msg, strlen(write_seat_msg));
    }
}
#endif

// Function to calculate the remaining time of the connection
int update_remaining_time(request* reqP) {
    struct timeval now;
    gettimeofday(&now, NULL);

    // 計算已過時間
    long elapsed_sec = now.tv_sec - reqP->remaining_time.tv_sec;
    long elapsed_usec = now.tv_usec - reqP->remaining_time.tv_usec;

    if (elapsed_usec < 0) {
        elapsed_usec += 1000000;
        elapsed_sec -= 1;
    }

    // 如果超時，關閉連線
    if (elapsed_sec >= TIMEOUT_SEC) {
        printf("Connection timed out for client %d\n", reqP->client_id);
        //close(reqP->conn_fd);
        //reqP->conn_fd = -1;  // 標記此連線為關閉
        return 1;
    }
    return 0;
}


int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    int conn_fd;  // fd for file that we open for reading
    char filename[FILE_LEN];

    int i,j;

    for (i = TRAIN_ID_START, j = 0; i <= TRAIN_ID_END; i++, j++) {
        getfilepath(filename, i);
#ifdef READ_SERVER
        trains[j].file_fd = open(filename, O_RDONLY);
        if (flock(trains[j].file_fd, LOCK_EX) == -1) {  // LOCK_EX 取得排他鎖
            perror("Error acquiring lock");
        }
#elif defined WRITE_SERVER
        trains[j].file_fd = open(filename, O_RDWR);
        if (flock(trains[j].file_fd, LOCK_EX) == -1) {  // LOCK_EX 取得排他鎖
            perror("Error acquiring lock");
        }
#else
        trains[j].file_fd = -1;
#endif
        if (trains[j].file_fd < 0) {
            ERR_EXIT("open");
        }
    }




    // Initialize server
    init_server((unsigned short)atoi(argv[1]));
    // Loop for handling connections

    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);

    // Create pollfd array
    struct pollfd fds[MAX_CLIENTS + 1];
    memset(fds, 0, sizeof(fds));

    // Add server listen_fd to the poll set
    fds[0].fd = svr.listen_fd;
    fds[0].events = POLLIN;
    int nfds = 1;
    
    while (1) {        
        if (poll(fds, nfds, 100) < 0) { // 100ms 
            ERR_EXIT("poll error");
        }

        // Check for new connections on the listening socket
        if (fds[0].revents & POLLIN) {
            conn_fd = accept_conn();
            if (conn_fd  > -1) {
                // Add new client connection to the poll set
                fds[nfds].fd = conn_fd;
                fds[nfds].events = POLLIN;
                nfds++;
                printf("New connection: fd %d\n", conn_fd);
                write(conn_fd, welcome_banner, strlen(welcome_banner));
#ifdef READ_SERVER
                write(conn_fd, read_shift_msg, strlen(read_shift_msg));
#elif defined WRITE_SERVER
                write(conn_fd, write_shift_msg, strlen(write_shift_msg));
#endif
            }
        }

        // Check for I/O on existing connections
        for (i = 1; i < nfds; i++) {
            
            if (fds[i].revents & POLLIN) { // 如果有資料可讀，則處理讀取操作
                int ret = handle_read(&requestP[fds[i].fd]);
                if (ret < 0) {
                    fprintf(stderr, "bad request from %s\n", requestP[fds[i].fd].host);
                    continue;
                }

                // 確保 write 只寫入到當前處理的客戶端
                int client_fd = fds[i].fd; // 當前客戶端的文件描述符
                if(strcmp(requestP[client_fd].buf, "exit") == 0){
                    write(requestP[conn_fd].conn_fd, exit_msg, strlen(exit_msg));
                    close(fds[i].fd);
                    free_request(&requestP[fds[i].fd]);
                    fds[i] = fds[nfds - 1];  // Move last entry to the current slot
                    nfds--;
                    i--;
                }

#ifdef READ_SERVER
                int train_id = atoi(requestP[client_fd].buf);

                if (train_id < TRAIN_ID_START || train_id > TRAIN_ID_END) {
                    // 如果輸入無效班次，提示訊息
                    write(client_fd, read_shift_msg, strlen(read_shift_msg)); 
                } else {
                    // Update booking_info
                    requestP[client_fd].booking_info.shift_id = train_id;  // 存儲 train_id
                    int train_fd = trains[train_id - TRAIN_ID_START].file_fd;
                    requestP[client_fd].booking_info.train_fd = train_fd;  // 存儲 train_fd

                    // 呼叫 print_train_info 函數來處理座位訊息
                    if (print_train_info(&requestP[client_fd]) == 0) {
                        // 成功生成座位訊息後，將其發送給客戶端
                        write(client_fd, requestP[client_fd].buf, strlen(requestP[client_fd].buf));
                        write(client_fd, read_shift_msg, strlen(read_shift_msg));
                    } else {
                        // 若 print_train_info 失敗，則提示錯誤
                        perror("print_train_info failed");
                    }
                }

#elif defined WRITE_SERVER

                switch (requestP[client_fd].status)
                {
                    case SHIFT:
                        if( strcmp(requestP[client_fd].buf, "seat") == 0 || strcmp(requestP[client_fd].buf, "pay") == 0){
                            write(requestP[client_fd].conn_fd, invalid_op_msg, strlen(invalid_op_msg));
                        }else{
                            handle_shift_input(&requestP[client_fd]);
                        }
                        break;
                    case SEAT :
                        if( strcmp(requestP[client_fd].buf, "seat") == 0){
                            write(requestP[client_fd].conn_fd, invalid_op_msg, strlen(invalid_op_msg));
                        }else{
                            handle_seat_input(&requestP[client_fd]);
                        }                        
                        break;
                    case BOOKED:
                        if(strcmp(requestP[client_fd].buf, "seat") == 0 || strcmp(requestP[client_fd].buf, "exit") == 0){
                            handle_booked_input(&requestP[client_fd]);
                        }else{
                            write(requestP[client_fd].conn_fd, invalid_op_msg, strlen(invalid_op_msg));
                        }
                        
                        break;
                    default:
                        break;
                }
        
#endif
            } // POLLIN end 

            if(update_remaining_time(&requestP[fds[i].fd]) == 1){
                close(fds[i].fd);
                free_request(&requestP[fds[i].fd]);
                fds[i] = fds[nfds - 1];  // Move last entry to the current slot
                nfds--;
                i--;
            }            
            

        }// For loop end
    }

    close(svr.listen_fd);
    for (i = 0; i < TRAIN_NUM; i++)
#ifdef READ_SERVER
        if (flock(trains[i].file_fd, LOCK_UN) == -1) {
            perror("Error releasing lock");
        }
#else   
        if (flock(trains[i].file_fd, LOCK_UN) == -1) {
            perror("Error releasing lock");
        } 
#endif         
        close(trains[i].file_fd);

    return 0;
}

int accept_conn(void) {

    struct sockaddr_in cliaddr;
    size_t clilen;
    int conn_fd;  // fd for a new connection with client

    clilen = sizeof(cliaddr);
    conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
    if (conn_fd < 0) {
        if (errno == EINTR || errno == EAGAIN) return -1;  // try again
        if (errno == ENFILE) {
            (void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                return -1;
        }
        ERR_EXIT("accept");
    }
    
    requestP[conn_fd].conn_fd = conn_fd;
    strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
    fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);
    requestP[conn_fd].client_id = (svr.port * 1000) + num_conn;    // This should be unique for the same machine.
    num_conn++;
    requestP[conn_fd].status = SHIFT;
    gettimeofday(&requestP[conn_fd].remaining_time, NULL);  // 紀錄當前時間
    requestP[conn_fd].remaining_time.tv_sec += TIMEOUT_SEC;  // 加上 5 秒
    return conn_fd;
}

static void getfilepath(char* filepath, int extension) {
    char fp[FILE_LEN*2];
    
    memset(filepath, 0, FILE_LEN);
    sprintf(fp, "%s%d", file_prefix, extension);
    strcpy(filepath, fp);
}

// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->client_id = -1;
    reqP->buf_len = 0;
    reqP->status = INVALID;
    reqP->remaining_time.tv_sec = 5;
    reqP->remaining_time.tv_usec = 0;

    reqP->booking_info.num_of_chosen_seats = 0;
    reqP->booking_info.train_fd = -1;
    for (int i = 0; i < SEAT_NUM; i++)
        reqP->booking_info.seat_stat[i] = UNKNOWN;
}

static void free_request(request* reqP) {
    memset(reqP, 0, sizeof(request));
    init_request(reqP);
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    // Get file descripter table size and initialize request table
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    return;
}