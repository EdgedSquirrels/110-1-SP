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
  ./player -n [player_id]
*/
int main(int argc, char **argv) {
    int player_id, opt = 0;
    while ((opt = getopt(argc, argv, "n:")) != -1) {
        // read from command line input
        switch (opt) {
            case 'n':
                player_id = atoi(optarg);
                break;
        }
    }

    if (player_id == -1) return 0;
    fclose(stdin);
    for (int round = 1; round <= 10; round++) {
        // iterate 10 rounds
        int guess;
        /* initialize random seed: */
        srand((player_id + round) * 323);
        /* generate guess between 1 and 1000: */
        guess = rand() % 1001;
        // send
        // draw: smaller id wins
        printf("%d %d\n", player_id, guess);
        fflush(stdout);
    }
    fclose(stdout);
}
