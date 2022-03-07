#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern int timeslice, switchmode;

//typedef struct TCB_NODE *TCB_ptr;
typedef struct TCB_NODE* TCB_ptr;
typedef struct TCB_NODE {
    jmp_buf Environment;
    int Thread_id;
    TCB_ptr Next;
    TCB_ptr Prev;
    int i, N;
    int w, x, y, z;
} TCB;

extern jmp_buf MAIN, SCHEDULER;
extern TCB_ptr Head;
extern TCB_ptr Current;
extern TCB_ptr Work;
extern sigset_t base_mask, waiting_mask, tstp_mask, alrm_mask;

void sighandler(int signo);
void scheduler();

// Call function in the argument that is passed in
#define ThreadCreate(function, thread_id, number) \
    {                                             \
        /* Please fill this code section. */      \
        if (setjmp(MAIN) == 0)                    \
            (function)(thread_id, number);        \
    }

// Build up TCB_NODE for each function, insert it into circular linked-list
#define ThreadInit(thread_id, number)        \
    {                                        \
        /* Please fill this code section. */ \
        Work = (TCB_ptr)malloc(sizeof(TCB)); \
        Work->Thread_id = thread_id;         \
        if (Head == NULL)                    \
            Current = Head = Work;           \
        else                                 \
            Current->Next = Work;            \
        Head->Prev = Work;                \
        Work->Prev = Current;                \
        Work->Next = Head;                   \
        Current = Work;                      \
        Current->N = number;                 \
        if (setjmp(Work->Environment) == 0)  \
            longjmp(MAIN, 1);                \
    }

// Call this while a thread is terminated
/*
    1. Release CPU voluntarily
*/
#define ThreadExit()                         \
    {                                        \
        /* Please fill this code section. */ \
        longjmp(SCHEDULER, 2);               \
    }

// Decided whether to "context switch" based on the switchmode argument passed in main.c
#define ThreadYield()                                                   \
    {                                                                   \
        /* Please fill this code section. */                            \
        if (setjmp(Current->Environment) == 0) {                        \
            if (switchmode == 1) {                                      \
                /* IIf there are signals, make sure they are accepted*/ \
                sigpending(&waiting_mask);                              \
                if (sigismember(&waiting_mask, SIGTSTP)) {              \
                    /* TSTP is in the pending signal*/                  \
                    sigprocmask(SIG_SETMASK, &alrm_mask, NULL);         \
                } else if (sigismember(&waiting_mask, SIGALRM)) {       \
                    /* ALRM is in the pending signal*/                  \
                    sigprocmask(SIG_SETMASK, &tstp_mask, NULL);         \
                }                                                       \
            } else if (switchmode == 0) {                               \
                longjmp(SCHEDULER, 1);                                  \
            }                                                           \
        }                                                               \
    }
