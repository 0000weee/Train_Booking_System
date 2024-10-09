#include "server.h"

#include <errno.h>
#include <limits.h>

const unsigned char IAC_IP[3] = "\xff\xf4";
const char *file_prefix = "./csie_trains/train_";
const char *accept_read_header = "ACCEPT_FROM_READ";
const char *accept_write_header = "ACCEPT_FROM_WRITE";
const char *welcome_banner =
    "======================================\n"
    " Welcome to CSIE Train Booking System \n"
    "======================================\n";

const char *lock_msg = ">>> Locked.\n";
const char *exit_msg = ">>> Client exit.\n";
const char *cancel_msg = ">>> You cancel the seat.\n";
const char *full_msg = ">>> The shift is fully booked.\n";
const char *seat_booked_msg = ">>> The seat is booked.\n";
const char *no_seat_msg = ">>> No seat to pay.\n";
const char *book_succ_msg = ">>> Your train booking is successful.\n";
const char *invalid_op_msg = ">>> Invalid operation.\n";

#ifdef READ_SERVER
char *read_shift_msg = "Please select the shift you want to check [902001-902005]: ";
#elif defined WRITE_SERVER
char *write_shift_msg = "Please select the shift you want to book [902001-902005]: ";
char *write_seat_msg = "Select the seat [1-40] or type \"pay\" to confirm: ";
char *write_seat_or_exit_msg = "Type \"seat\" to continue or \"exit\" to quit [seat/exit]: ";
#endif

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request *reqP);
// initailize a request instance

static void free_request(request *reqP);
// free resources used by a request instance

static void init_pollfd(struct pollfd *pollfdInfoP);
// initialize a pollfd instance

static void free_pollfd(struct pollfd *pollfdInfoP);
// free resources used by a pollfd instance

int accept_conn(void);
// accept connection

int str_to_long(char *text, long *result);
// convert string to long

static void getfilepath(char *filepath, int extension);
// get record filepath

// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void init_request(request *reqP) {
    reqP->conn_fd = -1;
    reqP->client_id = -1;
    reqP->buf_len = 0;
    reqP->status = STATE_INVALID;
    reqP->remaining_time.tv_sec = 5;
    reqP->remaining_time.tv_usec = 0;

    reqP->booking_info.num_of_chosen_seats = 0;
    reqP->booking_info.train_fd = -1;
    for (int i = 0; i < SEAT_NUM; i++)
        reqP->booking_info.seat_stat[i] = SEAT_UNKNOWN;
}

static void free_request(request *reqP) {
    // TODO: 把保留的座位釋出
    memset(reqP, 0, sizeof(request));
    init_request(reqP);
}

static void init_pollfd(struct pollfd *pollfdInfoP) {
    pollfdInfoP->fd = -1;
    pollfdInfoP->events = POLLIN;
    pollfdInfoP->revents = 0;
}

static void free_pollfd(struct pollfd *pollfdInfoP) {
    memset(pollfdInfoP, 0, sizeof(struct pollfd));
    init_pollfd(pollfdInfoP);
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0)
        ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&tmp,
                   sizeof(tmp))
        < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    // Get file descripter table size and initialize request table and pollfd table
    maxfd = getdtablesize();
    requestP = (request *)malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    pollfdP = (struct pollfd *)malloc(sizeof(struct pollfd) * maxfd);
    if (pollfdP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
        init_pollfd(&pollfdP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    pollfdP[svr.listen_fd].fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    return;
}
// ======================================================================================================

int handle_read(request *reqP) {
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
    if (r < 0)
        return -1;
    if (r == 0)
        return 0;
    char *p1 = strstr(buf, "\015\012");  // \r\n
    if (p1 == NULL) {
        p1 = strstr(buf, "\012");  // \n
        if (p1 == NULL) {
            if (!strncmp(buf, (const char *)IAC_IP, 2)) {
                // Client presses ctrl+C, regard as disconnection
                fprintf(stderr, "Client presses ctrl+C....\n");
                return 0;
            }
        }
    }

    len = p1 - buf + 1;
    memmove(reqP->buf, buf, len);
    reqP->buf[len - 1] = '\0';
    reqP->buf_len = len - 1;
    return 1;
}

void read_train_data(int train_fd, char *buf) {
    lseek(train_fd, 0, SEEK_SET);
    read(train_fd, buf, MAX_MSG_LEN);
}

void convert_train_data_to_array(char *train_data, int *array) {
    char *token = strtok(train_data, " \n");
    int i = 0;
    while (token != NULL) {
        array[i] = atoi(token);
        i++;
        token = strtok(NULL, " \n");
    }
}

void read_train_data_array(int train_fd, int *array) {
    char buf[MAX_MSG_LEN];
    read_train_data(train_fd, buf);
    convert_train_data_to_array(buf, array);
}

void write_train_data_by_array(int train_fd, int *array) {
    char buf[MAX_MSG_LEN];
    memset(buf, 0, sizeof(buf));
    for (int i = 0; i < SEAT_NUM; i++) {
        sprintf(buf + strlen(buf), "%d ", array[i]);
        if ((i % 4) == 3) {
            sprintf(buf + strlen(buf), "\n");
        }
    }
    lseek(train_fd, 0, SEEK_SET);
    write(train_fd, buf, strlen(buf));
}

#ifdef READ_SERVER

int print_train_info(request *reqP, long train_id) {
    train_info *trainsP = &trains[train_id - TRAIN_ID_START];
    char buf[MAX_MSG_LEN];
    read_train_data(trainsP->file_fd, buf);
    write(reqP->conn_fd, buf, strlen(buf));
    return 0;
}

void handle_client_input(request *reqP, struct pollfd *pollfdInfoP) {
    long train_id;
    int result = str_to_long(reqP->buf, &train_id);
    if (result == 0 && TRAIN_ID_START <= train_id && train_id <= TRAIN_ID_END) {
        print_train_info(reqP, train_id);
    }
    write(reqP->conn_fd, read_shift_msg, strlen(read_shift_msg));
}

#else

void create_seat_string(request *reqP, char *chosen_seat, enum SEAT seat_status) {
    for (int i = 0; i < SEAT_NUM; i++)
        if (reqP->booking_info.seat_stat[i] == seat_status) {
            sprintf(chosen_seat + strlen(chosen_seat), "%d,", i + 1);
        }
    if (chosen_seat[strlen(chosen_seat) - 1] == ',')
        chosen_seat[strlen(chosen_seat) - 1] = '\0';
}

int print_train_info(request *reqP) {
    /*
     * Booking info
     * |- Shift ID: 902001
     * |- Chose seat(s): 1,2
     * |- Paid: 3,4
     */
    char buf[MAX_MSG_LEN * 3];
    char chosen_seat[MAX_MSG_LEN] = "";
    char paid_seat[MAX_MSG_LEN] = "";

    // Construct chosen seat string
    create_seat_string(reqP, chosen_seat, SEAT_CHOSEN);
    // Construct paid seat string
    create_seat_string(reqP, paid_seat, SEAT_PAID);

    memset(buf, 0, sizeof(buf));
    sprintf(buf,
            "\nBooking info\n"
            "|- Shift ID: %d\n"
            "|- Chose seat(s): %s\n"
            "|- Paid: %s\n\n",
            reqP->booking_info.shift_id, chosen_seat, paid_seat);

    write(reqP->conn_fd, buf, strlen(buf));
    return 0;
}

void handle_input_shift(request *reqP) {
    long train_id;
    int result = str_to_long(reqP->buf, &train_id);
    if (result != 0 || train_id < TRAIN_ID_START || TRAIN_ID_END < train_id) {
        write(reqP->conn_fd, write_shift_msg, strlen(write_shift_msg));
        return;
    }

    reqP->booking_info.shift_id = train_id;
    reqP->booking_info.train_fd = trains[train_id - TRAIN_ID_START].file_fd;
    reqP->status = STATE_SEAT;

    print_train_info(reqP);
    write(reqP->conn_fd, write_seat_msg, strlen(write_seat_msg));
}

int lock_seat(request *reqP, long seat_id) {
    // TODO: lock file

    int result = 0;
    int train_data_array[SEAT_NUM];
    read_train_data_array(reqP->booking_info.train_fd, train_data_array);

    if (train_data_array[seat_id - 1] == SEAT_CHOSEN) {
        // Seat is chosen
        result = SEAT_CHOSEN;
    } else if (train_data_array[seat_id - 1] == SEAT_PAID) {
        // Seat is paid
        result = SEAT_PAID;
    }

    if (result == 0) {
        reqP->booking_info.seat_stat[seat_id - 1] = SEAT_CHOSEN;
        reqP->booking_info.num_of_chosen_seats++;
        train_data_array[seat_id - 1] = SEAT_CHOSEN;
        write_train_data_by_array(reqP->booking_info.train_fd, train_data_array);
    } else if (result == SEAT_CHOSEN && reqP->booking_info.seat_stat[seat_id - 1] == SEAT_CHOSEN) {
        reqP->booking_info.seat_stat[seat_id - 1] = SEAT_UNKNOWN;
        reqP->booking_info.num_of_chosen_seats--;
        train_data_array[seat_id - 1] = SEAT_UNKNOWN;
        write_train_data_by_array(reqP->booking_info.train_fd, train_data_array);
        write(reqP->conn_fd, cancel_msg, strlen(cancel_msg));
    }

    // TODO: unlock file

    return result;
}

void pay_seat(request *reqP) {
    if (reqP->booking_info.num_of_chosen_seats == 0) {
        write(reqP->conn_fd, no_seat_msg, strlen(no_seat_msg));

        print_train_info(reqP);
        write(reqP->conn_fd, write_seat_msg, strlen(write_seat_msg));
        return;
    }

    // TODO: lock file

    int train_data_array[SEAT_NUM];
    read_train_data_array(reqP->booking_info.train_fd, train_data_array);

    for (int i = 0; i < SEAT_NUM; i++) {
        if (reqP->booking_info.seat_stat[i] == SEAT_CHOSEN) {
            reqP->booking_info.seat_stat[i] = SEAT_PAID;
            train_data_array[i] = SEAT_PAID;
        }
    }
    write_train_data_by_array(reqP->booking_info.train_fd, train_data_array);

    // TODO: unlock file

    reqP->booking_info.num_of_chosen_seats = 0;
    reqP->status = STATE_BOOKED;

    write(reqP->conn_fd, book_succ_msg, strlen(book_succ_msg));
    print_train_info(reqP);
    write(reqP->conn_fd, write_seat_or_exit_msg, strlen(write_seat_or_exit_msg));
}

void handle_input_seat(request *reqP) {
    if (!strncmp(reqP->buf, "pay", 3)) {
        pay_seat(reqP);
        return;
    }

    long seat_id;
    int result = str_to_long(reqP->buf, &seat_id);
    if (result != 0 || seat_id < 1 || SEAT_NUM < seat_id) {
        write(reqP->conn_fd, write_seat_msg, strlen(write_seat_msg));
        return;
    }

    result = lock_seat(reqP, seat_id);
    if (result == SEAT_CHOSEN) {
        write(reqP->conn_fd, lock_msg, strlen(lock_msg));
    } else if (result == SEAT_PAID) {
        write(reqP->conn_fd, seat_booked_msg, strlen(seat_booked_msg));
    }

    print_train_info(reqP);
    write(reqP->conn_fd, write_seat_msg, strlen(write_seat_msg));
}

void handle_input_booked(request *reqP, struct pollfd *pollfdInfoP) {
    if (!strncmp(reqP->buf, "exit", 4)) {
        write(reqP->conn_fd, exit_msg, strlen(exit_msg));
        close(reqP->conn_fd);
        free_request(reqP);
        free_pollfd(pollfdInfoP);
        return;
    } else if (!strncmp(reqP->buf, "seat", 4)) {
        reqP->status = STATE_SHIFT;
        memset(reqP->booking_info.seat_stat, 0, sizeof(reqP->booking_info.seat_stat));
        reqP->booking_info.num_of_chosen_seats = 0;
        write(reqP->conn_fd, write_shift_msg, strlen(write_shift_msg));
    }
}

void handle_client_input(request *reqP, struct pollfd *pollfdInfoP) {
    switch (reqP->status) {
        case STATE_SHIFT:
            handle_input_shift(reqP);
            break;
        case STATE_SEAT:
            handle_input_seat(reqP);
            break;
        case STATE_BOOKED:
            handle_input_booked(reqP, pollfdInfoP);
            break;
        default:
            break;
    }
}

#endif

int accept_conn(void) {
    struct sockaddr_in client_address;
    size_t client_size;
    int conn_fd;  // fd for a new connection with client

    client_size = sizeof(client_address);
    conn_fd = accept(svr.listen_fd, (struct sockaddr *)&client_address, (socklen_t *)&client_size);
    if (conn_fd < 0) {
        if (errno == EINTR || errno == EAGAIN) {
            return -1;  // try again
        } else if (errno == ENFILE) {
            (void)fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
            return -1;
        }
        ERR_EXIT("accept");
    }

    request *currentReqP = &requestP[conn_fd];
    currentReqP->conn_fd = conn_fd;
    currentReqP->status = STATE_SHIFT;
    strcpy(currentReqP->host, inet_ntoa(client_address.sin_addr));
    fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, currentReqP->host);
    currentReqP->client_id = (svr.port * 1000) + num_conn;  // This should be unique for the same machine.
    pollfdP[conn_fd].fd = conn_fd;
    num_conn++;

    return conn_fd;
}

int str_to_long(char *text, long *result) {
    char *end;
    long value = strtol(text, &end, 10);

    if (errno == ERANGE && (value == LONG_MAX || value == LONG_MIN)) {
        // Range error occurred
        return -1;
    }
    if (end == text) {
        // No result
        return -1;
    }

    *result = value;
    return 0;
}

static void getfilepath(char *filepath, int extension) {
    char fp[FILE_LEN * 2];

    memset(filepath, 0, FILE_LEN);
    sprintf(fp, "%s%d", file_prefix, extension);
    strcpy(filepath, fp);
}

int main(int argc, char **argv) {
    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    int conn_fd;  // fd for file that we open for reading
    char filename[FILE_LEN];

    int ret;
    int i, j;

    for (i = TRAIN_ID_START, j = 0; i <= TRAIN_ID_END; i++, j++) {
        getfilepath(filename, i);
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

    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);

    while (1) {
        ret = poll(pollfdP, maxfd, 10);
        if (ret == 0) {
            // timeout, sleep 100 microseconds
            usleep(100);
            continue;
        } else if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            printf("errno: %d\n", errno);
            ERR_EXIT("poll fail");
        }

        if (pollfdP[svr.listen_fd].revents & POLLIN) {
            // Check new connection
            conn_fd = accept_conn();
            if (conn_fd < 0)
                continue;
            write(conn_fd, welcome_banner, strlen(welcome_banner));
#if defined READ_SERVER
            write(conn_fd, read_shift_msg, strlen(read_shift_msg));
#elif defined WRITE_SERVER
            write(conn_fd, write_shift_msg, strlen(write_shift_msg));
#endif
        }

        for (i = 0; i < maxfd; i++) {
            if (i == svr.listen_fd || requestP[i].conn_fd < 0) {
                continue;
            }

            if (pollfdP[i].revents & POLLIN) {
                ret = handle_read(&requestP[i]);
                if (ret < 0) {
                    fprintf(stderr, "bad request from %s\n", requestP[i].host);
                    continue;
                } else if (ret == 0) {
                    fprintf(stderr, "client close fd %d\n", requestP[i].conn_fd);
                    close(requestP[i].conn_fd);
                    free_request(&requestP[i]);
                    free_pollfd(&pollfdP[i]);
                    continue;
                }

                handle_client_input(&requestP[i], &pollfdP[i]);
            }
        }
    }

    free(requestP);
    close(svr.listen_fd);
    for (i = 0; i < TRAIN_NUM; i++) {
        close(trains[i].file_fd);
    }

    return 0;
}