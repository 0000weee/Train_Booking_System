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
     *   TODO: handle incomplete input
     */ 

    int r;
    char buf[MAX_MSG_LEN];
    size_t len;

    memset(buf, 0, sizeof(buf));

    // Read in request from client
    r = read(reqP->conn_fd, buf, sizeof(buf));

    //512 nonblock IO, need read until "\n"
    if (r < 0) return -1; 
    if (r == 0) return 0;

    char* p1 = strstr(buf, "\015\012"); // \r\n
    if (p1 == NULL) {
        p1 = strstr(buf, "\012");   // \n
        if (p1 == NULL) {
            if (!strncmp(buf, IAC_IP, 2)) {
                // Client presses ctrl+C, regard as disconnection
                fprintf(stderr, "Client presses ctrl+C....\n");
                return 0;
            }
        }
    }

    len = p1 - buf + 1;
    memmove(reqP->buf, buf, len);
    reqP->buf[len - 1] = '\0';
    reqP->buf_len = len-1;
    return 1;
}

#ifdef READ_SERVER
int print_train_info(request *reqP) {
    int train_fd = reqP->booking_info.train_fd;  // 獲取 train_fd

    if (flock(train_fd, LOCK_EX) == -1) {  // LOCK_EX 取得排他鎖
        perror("Error acquiring lock");
        return;
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

int fully_booked(int fd){ // HAV BUG
    char Fully_seat[FILE_LEN*2] = "1 1 1 1\n1 1 1 1\n1 1 1 1\n1 1 1 1\n1 1 1 1\n1 1 1 1\n1 1 1 1\n1 1 1 1\n1 1 1 1\n1 1 1 1\n";
    char Current_seat[FILE_LEN*2];
    read(fd, Current_seat, FILE_LEN*2);
    return (strcmp( Current_seat, Fully_seat) == 0)? 1 : 0 ;
}

void handle_shift_input(request *reqP){
    int train_id = atoi(reqP->buf);
    if(train_id<TRAIN_ID_START || train_id > TRAIN_ID_END){
        write(reqP->conn_fd, write_shift_msg, strlen(write_shift_msg));
    }else{
        if(fully_booked(reqP->booking_info.train_fd)){
            write(reqP->conn_fd, full_msg, strlen(full_msg));
            write(reqP->conn_fd, write_shift_msg, strlen(write_shift_msg));
        }else{
            // update booking_info
            reqP->booking_info.shift_id = train_id;
            reqP->booking_info.train_fd = trains[train_id - TRAIN_ID_START].file_fd;    
            print_train_info(reqP);

            reqP->status = SEAT;
            write(reqP->conn_fd, write_seat_msg, strlen(write_seat_msg));
        }
        
    }
}

void Write_Back_To_Fd(int fd, int seat_id, enum SEAT seat_status) {
    if (seat_id < 1 || seat_id > SEAT_NUM) {
        printf("Invalid seat_id\n");
        return;
    }

    // 計算 seat_id 對應的文件偏移位置
    int row = (seat_id - 1) / 4;  // 計算座位在哪一行
    int col = (seat_id - 1) % 4;  // 計算座位在哪一列
    off_t offset = row * ROW_SIZE + col * 2; // 每列2個字元, ROW_SIZE表示每行字元數 (數字 + 空格)

    // 根據座位狀態設置要寫入的字符
    char seat_char;
    switch (seat_status) {
        case UNKNOWN:
            seat_char = '0';
            break;
        case CHOSEN:
            seat_char = '1';
            break;
        case PAID:
            seat_char = '2';
            break;
        default:
            printf("Invalid seat status\n");
            return;
    }

    // 定位到檔案中的指定偏移量
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("lseek failed");
        return;
    }

    // 寫入座位狀態字符
    if (write(fd, &seat_char, 1) == -1) {
        perror("write failed");
        return;
    }

    printf("Seat %d updated successfully with status %d.\n", seat_id, seat_status);
}

int Check_Seat_Stat(int fd, int seat_id) {
    if (seat_id < 1 || seat_id > SEAT_NUM) {
        printf("Invalid seat_id\n");
        return -1; // 返回錯誤
    }

    // 計算 seat_id 對應的文件偏移位置
    int row = (seat_id - 1) / 4;  // 計算座位在哪一行
    int col = (seat_id - 1) % 4;  // 計算座位在哪一列
    off_t offset = row * ROW_SIZE + col * 2; // 每列2個字元 (數字 + 空格)

    // 定位到檔案中的指定偏移量
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("lseek failed");
        return -1;
    }

    // 讀取座位狀態字符
    char seat_char;
    if (read(fd, &seat_char, 1) == -1) {
        perror("read failed");
        return -1;
    }

    // 返回對應的 enum SEAT 狀態
    switch (seat_char) {
        case '0':
            return UNKNOWN;
        case '1':
            return CHOSEN;
        case '2':
            return PAID;
        default:
            printf("Unknown seat status: %c\n", seat_char);
            return -1; // 不認識的狀態
    }
}


void handle_seat_input(request *reqP){
    if(strcmp("pay", reqP->buf) == 0){
        if(reqP->booking_info.num_of_chosen_seats == 0){
            write(reqP->conn_fd, no_seat_msg, strlen(no_seat_msg));
            print_train_info(reqP);
        }
        else{
            for(int i=0; i< TRAIN_NUM; i++){
                    if(reqP->booking_info.seat_stat[i] == CHOSEN){
                        printf("%d chang status\n", i);
                        reqP->booking_info.seat_stat[i] = PAID;
                        Write_Back_To_Fd(reqP->booking_info.train_fd, i, PAID);  
                    }    
                }
                print_train_info(reqP);     
                write(reqP->conn_fd, book_succ_msg, strlen(book_succ_msg));    
                reqP->status = BOOKED;
                write(reqP->conn_fd, write_seat_or_exit_msg, strlen(write_seat_or_exit_msg));
        }
        

    }else{
        int seat_id = atoi(reqP->buf);
        switch( Check_Seat_Stat(reqP->booking_info.train_fd, seat_id)){ // Public data
            case UNKNOWN:
                reqP->booking_info.seat_stat[seat_id -1] = CHOSEN;//update seat_stat[SEAT_NUM]
                reqP->booking_info.num_of_chosen_seats ++;

                print_train_info(reqP);
                Write_Back_To_Fd(reqP->booking_info.train_fd, seat_id, CHOSEN); // Update train_90200X
                break;
            case CHOSEN:
                //write(reqP->conn_fd, lock_msg, strlen(lock_msg));
                if(reqP->booking_info.seat_stat[seat_id -1] == CHOSEN){
                    reqP->booking_info.seat_stat[seat_id -1] = UNKNOWN;//update seat_stat[SEAT_NUM]
                    reqP->booking_info.num_of_chosen_seats --;
                    print_train_info(reqP);
                    Write_Back_To_Fd(reqP->booking_info.train_fd, seat_id, UNKNOWN);
                }else{
                    write(reqP->conn_fd, lock_msg, strlen(lock_msg));
                }            
                break;

            case BOOKED:
                write(reqP->conn_fd, seat_booked_msg, strlen(seat_booked_msg));
                break;
            default:
                break;                
        }
    }    
    write(reqP->conn_fd, write_seat_msg, strlen(write_seat_msg));
}
void handle_booked_input(request *reqP){

}
#endif



int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    int conn_fd;  // fd for file that we open for reading
    char buf[MAX_MSG_LEN*2], filename[FILE_LEN];

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
        if (poll(fds, nfds, -1) < 0) {
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
                        handle_shift_input(&requestP[client_fd]);
                        break;
                    case SEAT :
                        handle_seat_input(&requestP[client_fd]);
                        break;
                    case BOOKED:
                        handle_booked_input(&requestP[client_fd]);
                        break;
                    default:
                        break;
                }
        
#endif
            // Close and remove the connection：timeout、user input exit
            /*close(fds[i].fd);
            free_request(&requestP[fds[i].fd]);
            fds[i] = fds[nfds - 1];  // Move last entry to the current slot
            nfds--;
            i--;  // Ensure we don't skip the next fd   
            */ 
            }            
        }
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