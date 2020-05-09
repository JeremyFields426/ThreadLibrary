#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <valgrind/valgrind.h>


typedef struct tcb
{
        int         thread_id;
        int         thread_priority;
        ucontext_t *thread_context;
        struct tcb *next;
} tcb;


// The val_ret structure is used for storing
// information about the stacks that are
// created for each context in the thread 
// control blocks.
typedef struct val_ret
{
	int             ret;
	struct val_ret *next;
} val_ret;


void push(tcb *thread);

void t_init();

int t_create(void (*func)(int), int id, int pri);

void t_yield();

void t_terminate();

void t_shutdown();
