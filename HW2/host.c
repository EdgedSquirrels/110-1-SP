#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
/*
bugs to be fixed:
fflush()
*/

void swap(int *a, int *b) {
    int c = *a;
    *a = *b;
    *b = c;
}

/*
pipe(fd[0][0]);  // write to child1
pipe(fd[0][1]);  // child1 write to host
pipe(fd[1][0]);  // write to child2
pipe(fd[1][1]);  // child2 write to host
*/

#define IO_Redirect_child(idx)              \
    {                                       \
        close(fd[idx][0][1]);               \
        close(fd[idx][1][0]);               \
        close(fd[!idx][0][0]);              \
        close(fd[!idx][0][1]);              \
        close(fd[!idx][1][0]);              \
        close(fd[!idx][1][1]);              \
        dup2(fd[idx][0][0], STDIN_FILENO);  \
        dup2(fd[idx][1][1], STDOUT_FILENO); \
        close(fd[idx][0][0]);               \
        close(fd[idx][1][1]);               \
    }
#define IO_Redirect_parent()                 \
    {                                        \
        pipe(fd[0][0]);                      \
        pipe(fd[0][1]);                      \
        pipe(fd[1][0]);                      \
        pipe(fd[1][1]);                      \
        wr_fd[0] = fdopen(fd[0][0][1], "w"); \
        rd_fd[0] = fdopen(fd[0][1][0], "r"); \
        wr_fd[1] = fdopen(fd[1][0][1], "w"); \
        rd_fd[1] = fdopen(fd[1][1][0], "r"); \
    }
#define IO_Close_parent()   \
    {                       \
        close(fd[0][0][0]); \
        close(fd[0][1][1]); \
        close(fd[1][0][0]); \
        close(fd[1][1][1]); \
    }

// ./host -m [host_id] -d [depth] -l [lucky_number]

int main(int argc, char **argv) {
    // root: depth = 0
    // child: depth = 1
    // leaf: depth = 2

    int host_id, depth, lucky_number;
    int opt = 0;
    // fprintf(stderr, "hihi");
    while ((opt = getopt(argc, argv, "m:d:l:")) != -1) {
        // read from command line input
        switch (opt) {
            case 'm':
                host_id = atoi(optarg);
                break;
            case 'd':
                depth = atoi(optarg);
                break;
            case 'l':
                lucky_number = atoi(optarg);
                break;
        }
    }
    int pid;
    FILE *wr_fifo = NULL, *rd_fifo = NULL;
    FILE *wr_fd[2] = {NULL}, *rd_fd[2] = {NULL};

    int player_ids[8], player_pts[8], len;
    char buffer[100];
    char buffer2[100];
    char buffer3[100];
    /*
    FILE *db_in, *db_out;                         // TODO debug
    snprintf(buffer, 100, "db_%d-%d.in", host_id, depth);   // TODO debug
    db_in = fopen(buffer, "w");                   // TODO debug
    snprintf(buffer, 100, "db_%d-%d.out", host_id, depth);  // TODO debug
    db_out = fopen(buffer, "w");                  // TODO debug
    */
    // build pipes to communicate with children
    int fd[2][2][2];

    // 1. For root host, prepare to read and write FIFO files.
    if (depth == 0) {
        // root
        // prepare read and write FIFO files to chiffon
        char fifo_name[20];
        snprintf(fifo_name, 20, "fifo_%d.tmp", host_id);
        rd_fifo = fopen(fifo_name, "r");  // read fifo
        if (rd_fifo == NULL) fprintf(stderr, "fifo_%d.tmp does not exist\n", host_id);
        wr_fifo = fopen("fifo_0.tmp", "w");  // write fifo
        if (wr_fifo == NULL) fprintf(stderr, "fifo_0.tmp does not exist\n");
    }

    // 2. For hosts except for leaf hosts, fork child hosts
    if (depth != 2) {
        // not leaf host
        IO_Redirect_parent();
        snprintf(buffer, 100, "%d", host_id);
        snprintf(buffer2, 100, "%d", depth + 1);
        snprintf(buffer3, 100, "%d", lucky_number);
        if ((pid = fork()) == 0) {
            IO_Redirect_child(0);
            execlp("./host", "./host", "-m", buffer, "-d", buffer2, "-l", buffer3, (char *)NULL);
            _exit(0);
        } else if ((pid = fork()) == 0) {
            IO_Redirect_child(1);
            execlp("./host", "./host", "-m", buffer, "-d", buffer2, "-l", buffer3, (char *)NULL);
            _exit(0);
        }
        IO_Close_parent();
    }

    // 3. Loop and get the player list from the parent until receiving ending message.
    while (true) {
        // pre-handling
        if (depth == 0) {
            // root host
            for (int i = 0; i < 8; i++) {
                fscanf(rd_fifo, "%d", &player_ids[i]);
                // fprintf(db_in, "%d%c", player_ids[i], " \n"[i==7]);  // TODO debug
                player_pts[i] = 0;
            }
            for (int i = 0; i < 4; i++) {
                fprintf(wr_fd[0], "%d%c", player_ids[i], " \n"[i == 3]);
                // fprintf(db_out, "%d%c", player_ids[i], " \n"[i == 3]);  // TODO debug
                fflush(wr_fd[0]);
            }
            for (int i = 4; i < 8; i++) {
                fprintf(wr_fd[1], "%d%c", player_ids[i], " \n"[i == 7]);
                // fprintf(db_out, "%d%c", player_ids[i], " \n"[i == 7]);  // TODO debug
                fflush(wr_fd[1]);
            }
            if (player_ids[0] == -1) {
                while(wait(NULL) > 0);
                fclose(stdin);
                fclose(stdout);
                fclose(rd_fd[0]);
                fclose(rd_fd[1]);
                break;  // get out of the while loop
            }
        } else if (depth == 1) {
            // child host
            for (int i = 0; i < 4; i++) {
                scanf("%d", &player_ids[i]);
                // fprintf(db_in, "%d%c", player_ids[i], " \n"[i == 3]);  // TODO debug
            }

            for (int i = 0; i < 2; i++) {
                fprintf(wr_fd[0], "%d%c", player_ids[i], " \n"[i == 1]);
                // fprintf(db_out, "%d%c", player_ids[i], " \n"[i == 1]);  // TODO debug
                fflush(wr_fd[0]);
            }
            for (int i = 2; i < 4; i++) {
                fprintf(wr_fd[1], "%d%c", player_ids[i], " \n"[i == 3]);
                // fprintf(db_out, "%d%c", player_ids[i], " \n"[i == 3]);  // TODO debug
                fflush(wr_fd[1]);
            }
            if (player_ids[0] == -1) {
                while( wait(NULL) > 0 );
                fclose(stdin);
                fclose(stdout);
                fclose(rd_fd[0]);
                fclose(rd_fd[1]);
                return 0;  // terminate immediately
            }
        } else if (depth == 2) {
            // leaf host
            for (int i = 0; i < 2; i++) {
                scanf("%d", &player_ids[i]);
                // fprintf(db_out, "%d%c", player_ids[i], " \n"[i == 1]);  // TODO debug
            }
            if (player_ids[0] == -1) return 0;  // terminate immediately
            IO_Redirect_parent();
            if ((pid = fork()) == 0) {
                IO_Redirect_child(0);
                snprintf(buffer, 100, "%d", player_ids[0]);
                execlp("./player", "./player", "-n", buffer, (char *)NULL);  // leaf
                _exit(0);
            } else if ((pid = fork()) == 0) {
                IO_Redirect_child(1);
                snprintf(buffer, 100, "%d", player_ids[1]);
                execlp("./player", "./player", "-n", buffer, (char *)NULL);  // leaf
                _exit(0);
            }
            IO_Close_parent();
        }

        // iterate for 10 rounds
        for (int round = 1; round <= 10; round++) {
            // handle the data from children
            int id[2], guess[2];
            // id and guess_number
            for (int i = 0; i < 2; i++) {
                fscanf(rd_fd[i], "%d%d", &id[i], &guess[i]);
                // fprintf(db_in, "in: %d %d\n", id[i], guess[i]);  // TODO debug
            }
                
            if (abs(lucky_number - guess[0]) > abs(lucky_number - guess[1]) || (abs(lucky_number - guess[0]) == abs(lucky_number - guess[1]) && id[0] > id[1])) {
                swap(&id[0], &id[1]);
                swap(&guess[0], &guess[1]);
            }

            // handle the data from children
            if (depth == 0) {
                // root
                for (int i = 0; i < 8; i++) {
                    // find the player and add score
                    if (id[0] == player_ids[i]) {
                        player_pts[i] += 10;
                        break;
                    }
                }
            } else {
                // not a root
                printf("%d %d\n", id[0], guess[0]);
                // fprintf(db_out, "out: %d %d\n", id[0], guess[0]);  // TODO debug
                fflush(stdout);
            }
            // sum the points up after 10 rounds
            // wait for forked players to exit and close pipes
            //root:
        }
        if (depth == 0) {
            // send final scores to Chiffon Game System
            len = 0;
            len += snprintf(buffer + len, 100 - len, "%d\n", host_id);
            for (int i = 0; i < 8; i++)
                len += snprintf(buffer + len, 100 - len, "%d %d\n", player_ids[i], player_pts[i]);
            /*
                [host_id]\n
                [player1_id] [player1_score]\n
                [player2_id] [player2_score]\n
                ......
                [player8_id] [player8_score]\n
            */
            fprintf(wr_fifo, "%s", buffer);
            // fprintf(db_in, "\nfifo:\n%s\n", buffer);  // TODO debug
            fflush(wr_fifo);
            // fwrite(buffer, sizeof(char), len, wr_fifo);
        } else if (depth == 2) {
            // leaf
            while(wait(NULL) > 0);
            fclose(rd_fd[0]);
            fclose(rd_fd[1]);
        }
    }
    // close FIFO files
    if (depth == 0) {
        fclose(wr_fifo);
        fclose(rd_fifo);
    }
    return 0;
}