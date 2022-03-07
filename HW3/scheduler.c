#include "threadtools.h"
/*
1) You should state the signal you received by: printf('TSTP signal caught!\n') or printf('ALRM signal caught!\n')
2) If you receive SIGALRM, you should reset alarm() by timeslice argument passed in ./main
3) You should longjmp(SCHEDULER,1) once you're done.
*/
void sighandler(int signo) {
    /* Please fill this code section. */
    // fprintf(stderr, "sig %d caught!!\n", signo);
    // fprintf(stderr, "note0\n");
    sigprocmask(SIG_SETMASK, &base_mask, NULL);
    // fprintf(stderr, "note1\n");
    if (switchmode == 0) return;  // ignore the signal
    if (signo == SIGTSTP) {       // TSTP
        printf("TSTP signal caught!\n");
    } else if (signo == SIGALRM) {  // ALRM
        printf("ALRM signal caught!\n");
        alarm(timeslice);
    }
    longjmp(SCHEDULER, 1);
}

/*
1) You are strongly adviced to make 
	setjmp(SCHEDULER) = 1 for ThreadYield() case
	setjmp(SCHEDULER) = 2 for ThreadExit() case
2) Please point the Current TCB_ptr to correct TCB_NODE
3) Please maintain the circular linked-list here
*/

void scheduler() {
    /* Please fill this code section. */
    int jmpno = setjmp(SCHEDULER);
    // fprintf(stderr, "scheduler %d\n", jmpno);
    if (jmpno == 0) // for set the longjmp 
        Current = Head;
    else if (jmpno == 1)  // for ThreadYield, context switch
        Current = Current->Next;
    else if (jmpno == 2) {  // for ThreadExit, delete one TCB
        if (Current->Next == Current) {
            // only one TCB left
            free(Current);
            Current = Head = NULL;
            longjmp(MAIN, 1);  // end of execution
        }
        Current->Next->Prev = Current->Prev;
        Current->Prev->Next = Current->Next;
        Work = Current->Next;
        free(Current);
        Current = Work;
    }
    longjmp(Current->Environment, 1);
}
