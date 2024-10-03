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
    while(0){
        //一個一個讀
    }

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
    
    int i;
    char buf[MAX_MSG_LEN];

    memset(buf, 0, sizeof(buf));
    for (i = 0; i < SEAT_NUM / 4; i++) {
        sprintf(buf + (i * 4 * 2), "%d %d %d %d\n", 0, 0, 0, 0);
    }
    return 0;
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
    char chosen_seat[MAX_MSG_LEN] = "1,2";
    char paid[MAX_MSG_LEN] = "3,4";

    memset(buf, 0, sizeof(buf));
    sprintf(buf, "\nBooking info\n"
                 "|- Shift ID: %d\n"
                 "|- Chose seat(s): %s\n"
                 "|- Paid: %s\n\n"
                 ,902001, chosen_seat, paid);
    return 0;
}
#endif

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    int conn_fd, maxfd = 0;
    char buf[MAX_MSG_LEN * 2], filename[FILE_LEN];
    int i, j;

    for (i = 0, j = 0; i < TRAIN_NUM; i++, j++) {
        getfilepath(filename, i + 1);
#ifdef READ_SERVER
            trains[j].file_fd = open(filename, O_RDONLY);
#elif defined WRITE_SERVER
            trains[j].file_fd = open(filename, O_RDWR);
#else
            trains[j].file_fd = -1;
#endif      
        if (trains[j].file_fd < 0) {
            ERR_EXIT("open");
        }
    }

    // Initialize server
    init_server((unsigned short)atoi(argv[1]));

    // Create pollfd array
    struct pollfd fds[MAX_CLIENTS + 1];
    memset(fds, 0, sizeof(fds));

    // Add server listen_fd to the poll set
    fds[0].fd = svr.listen_fd;
    fds[0].events = POLLIN;
    int nfds = 1;

    fprintf(stderr, "starting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, (int)(svr.port), svr.listen_fd, maxfd);

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