#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

// define macro
#define ERR_EXIT(a) \
    do {            \
        perror(a);  \
        exit(1);    \
    } while (0)
// broadcast the error message

typedef struct {
    char hostname[512];   // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;        // fd to wait for a new connection
} server;

typedef struct {
    char host[512];  // client's host
    int conn_fd;     // fd to talk with client
    char buf[512];   // data sent by/to client
    size_t buf_len;  // bytes used by buf
    // you don't need to change this.
    int id;     // indicate the connected id
    int stage;  // 0: ask id 1: ask the order
} request;

typedef struct {
    int id;  //902001-902020
    int AZ;
    int BNT;
    int Moderna;
} registerRecord;

int lock(int fd, int id, short lock_type) {
    struct flock lock;
    lock.l_type = lock_type;
    lock.l_start = sizeof(registerRecord) * (id - 902001);
    lock.l_whence = SEEK_SET;
    lock.l_len = sizeof(registerRecord);
    return (fcntl(fd, F_SETLK, &lock));
}

int unlock(int fd, int id) {
    struct flock lock;
    lock.l_type = F_UNLCK;
    lock.l_start = sizeof(registerRecord) * (id - 902001);
    lock.l_whence = SEEK_SET;
    lock.l_len = sizeof(registerRecord);
    return (fcntl(fd, F_SETLKW, &lock));
}

server svr;                // server
request *requestP = NULL;  // pointer to a list of requests
int maxfd;                 // size of open file descriptor table, size of request list

const char *accept_read_header = "ACCEPT_FROM_READ";    // a header string
const char *accept_write_header = "ACCEPT_FROM_WRITE";  // a header string

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request *reqP);
// initailize a request instance

static void free_request(request *reqP);
// free resources used by a request instance

ssize_t read_record(int rfd, registerRecord *reg, int id) {
    lseek(rfd, sizeof(registerRecord) * (id - 902001), SEEK_SET);
    return read(rfd, reg, sizeof(registerRecord));
}

ssize_t write_record(int wfd, registerRecord *reg, int id) {
    // check and lock
    lseek(wfd, sizeof(registerRecord) * (id - 902001), SEEK_SET);
    return write(wfd, reg, sizeof(registerRecord));
    // lseek(fd, sizeof())
    // unlock
}

int handle_read(request *reqP) {
    // read buf from client and write it on reqP
    // return 0: no input
    // return -1: fail
    // return 1: success
    int r;
    char buf[512];

    // Read in request from client
    r = read(reqP->conn_fd, buf, sizeof(buf));
    if (r < 0)
        return -1;
    if (r == 0)
        return 0;
    char *p1 = strstr(buf, "\015\012");  // \r\n
    // int newline_len = 2;                 // unknown usage
    if (p1 == NULL) {
        p1 = strstr(buf, "\012");  // \n
        if (p1 == NULL) {
            ERR_EXIT("this really should not happen...");
        }
    }
    // store the buf to req
    size_t len = p1 - buf + 1;
    memmove(reqP->buf, buf, len);
    reqP->buf[len - 1] = '\0';
    reqP->buf_len = len - 1;
    return 1;
}

struct pollfd fds[100];
int fdsn = 0, ids_avail[25];

char msg[][100] = {
    "[Error] Operation failed. Please try again.\n",
    "Locked.\n",
    "Please enter your id (to check your preference order):\n",
    "Your preference order is %s > %s > %s.\n",
    "Please input your preference order respectively(AZ,BNT,Moderna):\n",
    "Preference order for %d modified successed, new preference order is %s > %s > %s.\n"};

int msglen[6];

void conn_cls(int i, int conn_fd) {
    // write(conn_fd, "debug", strlen("debug"));
    close(requestP[conn_fd].conn_fd);
    free_request(&requestP[conn_fd]);
    fds[i] = fds[fdsn];
    fdsn--;
    // fds[i] = fds[fds[fdsn].fd];
}

void handle_err(int i, int fd) {
    write(fd, msg[0], msglen[0]);
    conn_cls(i, fd);
}

void handle_lcked(int i, int fd) {
    write(fd, msg[1], msglen[1]);
    conn_cls(i, fd);
}

int main(int argc, char **argv) {
    registerRecord reg;
    int db_fd = open("registerRecord", O_RDWR);
    if (db_fd < 0) {
        fprintf(stderr, "Error on opening registerRecord\n");
        return -1;
    }

    for (int i = 0; i < 25; i++)
        ids_avail[i] = 1;
    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    for (int i = 0; i < 6; i++)
        msglen[i] = strlen(msg[i]);

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;
    int conn_fd;  // fd for a new connection with client

    char buf[512];
    int buf_len;

    // Initialize server
    init_server((unsigned short)atoi(argv[1]));

    fds[0].fd = svr.listen_fd;
    fds[0].events = POLLIN;

    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);

    while (true) {
        // TODO: Add IO multiplexing
        int n = poll(fds, fdsn + 1, -1);  // n: the number of fds available
        // fprintf(stderr, "fdsn: %d\n", fdsn);
        // fprintf(stderr, "flag0\n");
        for (int i = fdsn; i >= 0 && n > 0; i--) {
            if (!(fds[i].revents & POLLIN))  // not ready
                continue;
            int fd = fds[i].fd;  // the fd is ready
            if (i == 0) {        // handle new connection
                clilen = sizeof(cliaddr);
                // fprintf(stderr, "\n%d is ready\n", i);
                conn_fd = accept(svr.listen_fd, (struct sockaddr *)&cliaddr, (socklen_t *)&clilen);

                if (conn_fd < 0) {
                    // accept client went wrong
                    if (errno == EINTR || errno == EAGAIN)
                        continue;  // try again
                    if (errno == ENFILE) {
                        (void)fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                        continue;
                    }
                    ERR_EXIT("accept");
                }
                requestP[conn_fd].conn_fd = conn_fd;
                strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
                fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);
                fcntl(conn_fd, F_SETFL, O_SYNC);
                // fprintf(stderr, "to client: %s\n", msg[2]);
                write(conn_fd, msg[2], msglen[2]);
                // "Please enter your id (to check your preference order):\n"
                requestP[conn_fd].stage = 0;

                // add conn_fd to poll list
                fdsn++;
                fds[fdsn].fd = conn_fd;
                fds[fdsn].events = POLLIN;
            }
            // for i != 0
            else {  // handle client input
                // read input
                fd = fds[i].fd;
                // fprintf(stderr, "flag1\n");
                int ret = handle_read(&requestP[fd]);  // parse data from client to requestP[conn_fd].buf
                if (ret < 0) {
                    fprintf(stderr, "bad request from %s\n", requestP[fd].host);
                    continue;
                }

                if (requestP[fd].stage == 0) {  // handle read id
                    // verify read id
                    bool valid = 1;
                    // fprintf(stderr, "flag2\n");
                    if (strlen(requestP[fd].buf) != 6) valid = 0;
                    int id;
                    if (valid) {
                        id = atoi(requestP[fd].buf);
                        if (!(902001 <= id && id <= 902020)) valid = 0;
                    }
                    if (!valid) {
                        handle_err(i, fd);
                        continue;
                    }
                    // fprintf(stderr, "flag3\n");
                    requestP[fd].id = id;
                    // test lock
                    short lock_type = F_RDLCK;
#ifdef WRITE_SERVER
                    lock_type = F_WRLCK;
#endif
                    //char ssss[100] = "fuck\n";
                    // write(fd, ssss, strlen(ssss));
                    if (ids_avail[id - 902001] && lock(db_fd, id, lock_type) == 0) {  // lock it and do some operation
                        ids_avail[id - 902001] = 0;
                        read_record(db_fd, &reg, id);
                        // fprintf(stderr, "flag4\n");
                        char ss[100];
                        char s[4][10];
                        int AZ = reg.AZ, BNT = reg.BNT, Moderna = reg.Moderna;

                        strcpy(s[AZ], "AZ");
                        strcpy(s[BNT], "BNT");
                        strcpy(s[Moderna], "Moderna");
                        snprintf(ss, 100, msg[3], s[1], s[2], s[3]);
                        // fprintf(stderr, "to client: %s\n", ss);
                        write(fd, ss, strlen(ss));
#ifdef WRITE_SERVER
                        write(fd, msg[4], msglen[4]);
                        requestP[fd].stage = 1;
#elif READ_SERVER
                        unlock(db_fd, id);
                        ids_avail[id - 902001] = 1;
                        conn_cls(i, fd);
                        continue;
#endif
                    } else {  // locked
                        handle_lcked(i, fd);
                        continue;
                    }
                }

                else if (requestP[fd].stage == 1) {  // handle update preference
                    // fprintf(stderr, "flag6\n");
                    bool valid = 1;
                    if (requestP[fd].buf_len != 5) valid = 0;
                    int AZ, BNT, Moderna;
                    int id = requestP[fd].id;
                    // fprintf(stderr, "flag7\n");
                    if (valid) {
                        if (requestP[fd].buf[1] != requestP[fd].buf[3]) valid = 0;
                        if (requestP[fd].buf[1] != ' ') valid = 0;
                    }
                    if (valid) {
                        AZ = requestP[fd].buf[0] - '0';
                        BNT = requestP[fd].buf[2] - '0';
                        Moderna = requestP[fd].buf[4] - '0';
                        for (int i = 1; i <= 3; i++)
                            if (AZ != i && BNT != i && Moderna != i) valid = 0;
                    }
                    if (!valid) {  // invalid input
                        unlock(db_fd, id);
                        handle_err(i, fd);
                        ids_avail[id - 902001] = 1;
                        continue;
                    }

                    reg.id = id;
                    reg.AZ = AZ;
                    reg.BNT = BNT;
                    reg.Moderna = Moderna;

                    write_record(db_fd, &reg, id);
                    unlock(db_fd, id);
                    ids_avail[id - 902001] = 1;
                    char ss[100];
                    char s[4][10];
                    // fprintf(stderr, "flag8\n");
                    strcpy(s[AZ], "AZ");
                    strcpy(s[BNT], "BNT");
                    strcpy(s[Moderna], "Moderna");
                    snprintf(ss, 100, msg[5], id, s[1], s[2], s[3]);
                    // fprintf(stderr, "to client: %s\n", ss);
                    write(fd, ss, strlen(ss));

                    conn_cls(i, fd);
                }
            }
            n--;
        }
        // fprintf(stderr, "tick\n");
    }
    // fprintf(stderr, "tick\n");
    free(requestP);
    return 0;
}

// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void init_request(request *reqP) {
    // initailize a request instance
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->id = 0;
}

static void free_request(request *reqP) {
    // free resources used by a request instance
    /*if (reqP->filename != NULL) {
        free(reqP->filename);
        reqP->filename = NULL;
    }*/
    init_request(reqP);
}

static void init_server(unsigned short port) {
    // initailize a server, exit for error

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
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    // Get file descripter table size and initialize request table
    maxfd = getdtablesize();
    requestP = (request *)malloc(sizeof(request) * maxfd);
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
