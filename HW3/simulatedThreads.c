#include "threadtools.h"

// Please complete this three functions. You may refer to the macro function defined in "threadtools.h"
// Reduce Integer
// You are required to solve this function via iterative method, instead of recursion.

void ReduceInteger(int thread_id, int number) {
    /* Please fill this code section. */
    ThreadInit(thread_id, number);
    Current->w = 0;
    if (Current->N <= 1) {
        sleep(1);
        printf("Reduce Integer: %d\n", Current->w);
        ThreadYield();
    }
    while (Current->N > 1) {
        sleep(1);
        if (Current->N % 2 == 0)
            Current->N /= 2;
        else if (Current->N % 4 == 1 || Current->N == 3)
            Current->N--;
        else
            Current->N++;
        Current->w++;
        printf("Reduce Integer: %d\n", Current->w);
        ThreadYield();
    }
    // return Current->w;
    ThreadExit();
}
/*
calculation:
int reduce_int(int number) {
    int N = number, w = 0;
    while (N > 1) {
        if (N % 4 == 3) N++;
        else if (N % 4 == 1) N--;
        else N /= 2;
        w++;
    }
    return w;
}
*/

// Mountain Climbing
// You are required to solve this function via iterative method, instead of recursion.
void MountainClimbing(int thread_id, int number) {
    /* Please fill this code section. */
    ThreadInit(thread_id, number);
    Current->x = 0, Current->y = 1, Current->w = 1, Current->i = 0;
    do {
        sleep(1);
        Current->w = Current->x + Current->y;
        Current->x = Current->y;
        Current->y = Current->w;
        printf("Mountain Climbing: %d\n", Current->w);
        Current->i++;
        ThreadYield();
    } while (Current->i < Current->N);
    ThreadExit();
}
/*
calculation:
int mnt_climb(int number) {
	int x = 0, y = 1, w = 1;
	for(int i = 0; i < number; i++) {
		w = x + y;
		x = y;
		y = w;
	}
	return w;
}
*/

// Operation Count
// You are required to solve this function via iterative method, instead of recursion.
void OperationCount(int thread_id, int number) {
    /* Please fill this code section. */
    ThreadInit(thread_id, number);
    sleep(1);
    Current->w = Current->N * Current->N / 4;
    printf("Operation Count: %d\n", Current->w);
    Current->i = 1;
    ThreadYield();
    ThreadExit();
    // return n^2 / 4
}
/*
int func(int num) {
	return n * n / 4;
}
*/
