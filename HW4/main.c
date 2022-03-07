#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct {
    int left;
    int right;
    // [left, right)
} job_info;

job_info *jobs;

char *board;
int col, row, epoch, epoch_cnt = 0, n_process = 1, n_thread = 1, prefix = 0, fdin, fdout;
// char *inmmap, *outmmap;

#define get_board(i, j, c) \
    board[(c) * (col + 1) * row + i * (col + 1) + j]

#define set_board(val, i, j, c)                                 \
    {                                                           \
        board[(c) * (col + 1) * row + i * (col + 1) + j] = val; \
    }
/*
[
[1, 0, '\n'],
[1, 1, '\n'],
[1, 0,  ?  ]
]
 ** Be aware of output

*/
#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)

/*
void *in_job(void *pidx) {
    int idx = (int)pidx;
    int lb = jobs[idx].left, rb = jobs[idx].right;
    pread(fdin, board, sizeof(char) * (rb - lb), prefix + lb);
}

void *out_job(void *pidx) {
    int idx = (int)pidx;
    int lb = jobs[idx].left, rb = jobs[idx].right;
    pwrite(fdout, board + (epoch % 2) * row * (col+1), sizeof(char) * (rb - lb), lb);
}
*/

void *job(void *pidx) {
    int idx = (int)pidx;
    for (int fidx = jobs[idx].left; fidx < jobs[idx].right; fidx++) {
        int i = fidx / (col + 1), j = fidx % (col + 1);
        if (j == col) continue;
        char num = 0, live = '.';

        int i2l = max(i - 1, 0), i2r = min(i + 2, row), j2l = max(j - 1, 0), j2r = min(j + 2, col);

        for (int i2 = i2l; i2 < i2r; i2++) {
            for (int j2 = j2l; j2 < j2r; j2++) {
                if (get_board(i2, j2, (epoch_cnt % 2)) == 'O') num++;
            }
        }

        if (num == 3 || (get_board(i, j, (epoch_cnt) % 2) == 'O' && num == 4))
            live = 'O';
        set_board(live, i, j, (epoch_cnt + 1) % 2);
    }
}

int main(int argc, char **argv) {
    int opt = 0;
    while ((opt = getopt(argc, argv, "p:t:")) != -1) {
        // update n_process and n_thread
        switch (opt) {
            case 'p':
                n_process = atoi(optarg);
                break;
            case 't':
                n_thread = atoi(optarg);
                break;
        }
    }
    char *input_filename = argv[3], *output_filename = argv[4];
    FILE *fpin = fopen(input_filename, "r"), *fpout = fopen(output_filename, "w");
    fscanf(fpin, "%d%d%d", &row, &col, &epoch);
    prefix = ftell(fpin) + 1;
    fdin = fileno(fpin), fdout = fileno(fpout);

    int worknum = n_process * n_thread, worklen = row * (col + 1) - 1, tmp1 = 0, tmp2 = 0;
    jobs = (job_info *)malloc(sizeof(job_info) * worknum);

    for (int idx = 0; idx < worknum; idx++) {
        tmp2 = tmp1 + worklen / worknum + (idx < worklen % worknum);
        jobs[idx].left = tmp1, jobs[idx].right = tmp2;
        // fprintf(stderr, "job: %d %d\n", tmp1, tmp2);
        tmp1 = tmp2;
    }

    // mmap shared resource board
    if (n_process > 1) {
        board = (char *)mmap(NULL, sizeof(char) * (col + 1) * row * 2 + 10, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

        lseek(fdin, prefix, SEEK_SET);
        read(fdin, board, sizeof(char) * worklen);
        // pread(fdin, board, sizeof(char) * worklen, prefix);

        for (int i = 0; i < row; i++) {
            set_board('\n', i, col, 1);
        }

        for (epoch_cnt = 0; epoch_cnt < epoch; epoch_cnt++) {
            for (int idx = 0; idx < n_process; idx++) {
                if (fork() == 0) {
                    job((void *)idx);
                    _exit(0);
                }
            }
            while (wait(NULL) > 0);
        }

        // pwrite(fdout, board + (epoch%2)*row*(col+1)  , sizeof(char) * worklen, 0);
        write(fdout, board + (epoch % 2) * row * (col + 1), sizeof(char) * worklen);
    } else {
        pthread_t tid[n_thread];
        board = (char *)malloc(sizeof(char) * row * (col + 1) * 2 + 10);

        lseek(fdin, prefix, SEEK_SET);
        read(fdin, board, sizeof(char) * worklen);

        for (int i = 0; i < row; i++) {
            set_board('\n', i, col, 1);
        }

        for (epoch_cnt = 0; epoch_cnt < epoch; epoch_cnt++) {
            for (int idx = 0; idx < n_thread; idx++) {
                pthread_create(&tid[idx], NULL, job, (void *)idx);
            }
            // fprintf(stderr, "epoch_cnt: %d\n", epoch_cnt);
            for (int idx = 0; idx < n_thread; idx++) {
                pthread_join(tid[idx], NULL);
            }
        }

        // pwrite(fdout, board + (epoch%2)*row*(col+1), sizeof(char) * worklen, 0);
        write(fdout, board + (epoch % 2) * row * (col + 1), sizeof(char) * worklen);
    }
    fclose(fpin), fclose(fpout);
}

/*
./main -t 2 ./sample_input/in1.txt ./out1.txt
./main -p 2 ./sample_input/largeCase_in.txt ./largeCase_out.txt
*/