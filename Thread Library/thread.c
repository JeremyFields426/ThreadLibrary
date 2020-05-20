#include "thread.h"


// val_rets is a linked list that contains integers
// used for the valgrind return info. Using this
// structure, I am able to avoid a majority of the
// errors associated with using valgrind in tandem
// with additional stacks.

// Although running could technically be used as a
// linked list, it only ever has one thread control
// block in the chain at any given time.

tcb     *running = NULL;
tcb     *ready = NULL;
val_ret *val_rets = NULL;


int sem_init(sem_t **sp, int sem_count)
{
	*sp = malloc(sizeof(sem_t));
	(*sp)->count = sem_count;
	(*sp)->q = NULL;
}

void sem_wait(sem_t *sp)
{
	// This function moves the current thread
	// to the end of the semaphore's blocking
	// queue and then switches to the first
	// available thread in the ready queue.

	sp->count--;
	if (sp->count < 0)
	{
		tcb *tmp = running;
		push(&(sp->q), running);
		running = pull(&ready);
		
		swapcontext(tmp->thread_context, running->thread_context);
	}
}

void sem_signal(sem_t *sp)

	// This function takes the first thread in the
	// semaphore's blocking queue and moves it to the
	// end of the ready queue.

	sp->count++;
	push(&ready, pull(&(sp->q)));
}

void sem_destroy(sem_t **sp)
{
	tcb *step = (*sp)->q;

	while (step != NULL)
        {
                tcb *tmp = step->next;
                free(step->thread_context->uc_stack.ss_sp);
                free(step->thread_context);
                free(step);
                step = tmp;
        }

	free(*sp);
	*sp = NULL;
}


tcb *pull(tcb **head)
{
	// This function is used simply to put the thread at
        // the beginning of the given queue.

        if (*head == NULL) { return NULL; }

        tcb* tmp = *head;
        *head = (*head)->next;
        tmp->next = NULL;

        return tmp;
}

void push(tcb **head, tcb *thread)
{
	// This function is used simply to put the thread at
        // the end of the given queue.

	if (thread == NULL) { return; }

	if (*head == NULL)
	{
		*head = thread;
		return;
	}

	tcb *step = *head;

	while (step->next != NULL) { step = step->next; }

	step->next = thread;
}

void display(struct tcb *queue)
{
	// This is just a debug function used to print out
	// all of the tcb structs in the linked list.

        tcb *step = queue;

        while (step != NULL)
        {
                printf("Id: %d\t", step->thread_id);
                step = step->next;
        }

        printf("\n");
}

void t_init()
{
	// This function sets up the initial context of the main function
	// and assigns it to the running queue.

        ucontext_t *uc = (ucontext_t *) malloc(sizeof(ucontext_t));
        getcontext(uc);

        tcb *main = (tcb *) malloc(sizeof(tcb));
        main->thread_context = uc;
        main->next = NULL;

        running = main;
}

void t_yield()
{
	// This function pushes the currently running thread to the
	// end of the ready queue, pulls the first available thread
	// from the ready queue, and swaps to the new running thread.

	tcb *tmp = running;
	push(&ready, running);
	running = pull(&ready);

  	swapcontext(tmp->thread_context, running->thread_context);
}

int t_create(void (*fct)(int), int id, int pri)
{
	// This function sets up the context for the given function,
	// creates the associated thread control block, and adds the
	// new thread to the end of the ready queue.

	size_t size = 0x10000;
	
	// For ease of use, I made the valgrind return info
	// a linked list (the val_ret struct is defined in thread.h)
	// so that I would not have to worry about having a set
	// number of threads running at any given time (i.e. the
	// array could have a dynamic size).
	void *stack = malloc(size);
	val_ret *ret = (val_ret *) malloc(sizeof(val_ret));
	ret->ret = VALGRIND_STACK_REGISTER(stack, stack + size);
	ret->next = val_rets;
	val_rets = ret;

  	ucontext_t *uc = (ucontext_t *) malloc(sizeof(ucontext_t));
  	getcontext(uc);
  	uc->uc_stack.ss_sp = stack;  
	uc->uc_stack.ss_size = size;
  	uc->uc_stack.ss_flags = 0;
  	uc->uc_link = running->thread_context; 
  	makecontext(uc, (void (*)(void)) fct, 1, id);
  	
	tcb *thread = (tcb *) malloc(sizeof(tcb));
	thread->thread_id = id;
	thread->thread_priority = pri;
	thread->thread_context = uc;
	thread->next = NULL;

	push(&ready, thread);
}

void t_terminate()
{
	// This function frees the currently running thread
	// and switches contexts to the first available thread
	// in the ready queue.

	free(running->thread_context->uc_stack.ss_sp);
	free(running->thread_context);
	free(running);

	running = pull(&ready);
	setcontext(running->thread_context);
}

void t_shutdown()
{
	// This function frees the currently running thread,
	// all of the valgrind return info, and all of the
	// threads in the ready queue (it also frees all
	// associated information of each structure).

	free(running->thread_context);
	free(running);

	while (val_rets != NULL)
	{
		val_ret *tmp_ret = val_rets->next;
		VALGRIND_STACK_DEREGISTER(val_rets->ret);
		free(val_rets);
		val_rets = tmp_ret;
	}

	while (ready != NULL)
	{
		tcb *tmp_tcb = ready->next;
		free(ready->thread_context->uc_stack.ss_sp);
		free(ready->thread_context);
		free(ready);
		ready = tmp_tcb;
	}
}
