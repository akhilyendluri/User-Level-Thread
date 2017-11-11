/* Pet Thread Library
 *  (c) 2017, Jack Lange <jacklange@cs.pitt.edu>
 *  Akhil Yendluri
 *  Krithika Ganesh
 */
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <inttypes.h>

#include "pet_thread.h"
#include "pet_hashtable.h"
#include "pet_list.h"
#include "pet_log.h"

#define STACK_SIZE (4096 * 32)

typedef enum {PET_THREAD_STOPPED,
	      PET_THREAD_RUNNING,
 	      PET_THREAD_READY,
	      PET_THREAD_BLOCKED} thread_run_state_t;

struct exec_ctx {
    uint64_t rbp;
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rip;
} __attribute__((packed));


struct pet_thread {
    struct exec_ctx context;
    pet_thread_id_t thread_id;
    thread_run_state_t state;
    char * stackPtr;
    char *stackPtrTop;
    pet_thread_fn_t function;
    void * args;
    struct list_head node;
    pet_thread_id_t joinfrom;
    void* return_val;

};

static pet_thread_id_t current     = PET_MASTER_THREAD_ID;
struct pet_thread      master_dummy_thread;

static LIST_HEAD(thread_list);
struct pet_thread readyQueue; 
static int thread_ids = 1;
struct pet_thread * master_thread;
static int pet_thread_ready_count  = -1;

extern void __switch_to_stack(void            * tgt_stack,
			      void            * saved_stack,
			      pet_thread_id_t   current,
			      pet_thread_id_t   tgt);

extern void __abort_to_stack(void * tgt_stack);

static struct pet_thread *
get_thread(pet_thread_id_t thread_id)
{
    if (thread_id == PET_MASTER_THREAD_ID) {
	return master_thread;
    }
    struct pet_thread *tmp, *next;
    struct list_head *pos;
    list_for_each_safe(pos,next, &readyQueue.node) {
		tmp=list_entry(pos,struct pet_thread, node);
    if(tmp->thread_id == thread_id) {
            return tmp;
        }

    }
    return NULL;
}

static pet_thread_id_t
get_thread_id(struct pet_thread * thread)
{
    if (thread == &(master_dummy_thread)) {
	return PET_MASTER_THREAD_ID;
    }
    return thread->thread_id;
}

/*
 * Initializes main thread (master_dummy_thread)
 */
int
pet_thread_init(void)
{
    INIT_LIST_HEAD(&readyQueue.node);
    master_thread = &master_dummy_thread;
    master_thread->state= PET_THREAD_READY;
    master_thread->thread_id = PET_MASTER_THREAD_ID;
    char * size = (master_thread->stackPtr + STACK_SIZE);
    pet_thread_ready_count  = 0;
    return 0;
}


static void
__dump_stack(struct pet_thread * thread)
{
    DEBUG("Dump thread> Thread ID: %d \n",thread->thread_id);
    DEBUG("Dump thread> Thread Ptr: %p \n",thread);
    DEBUG("Dump thread> Stack Pointer %p \n", thread->stackPtr);
    DEBUG("Dump thread> FunctionPtr: %p \n",thread->function);
    DEBUG("Dump thread> State:%d \n",thread->state);
    DEBUG("Dump thread> Join From %d \n",thread->joinfrom);
    DEBUG("Dump thread> Arguments %d \n",thread->args);
    DEBUG("\n");

    return;
}


/*
 * joins the current thread with the thread ID specified
 * changes the current thread state to BLOCKED
 * changes the target thread state to RUNNING
 * tracks the current joining thread using joinfrom of target thread
 * saves the address of the target thread's return value in ret_val
 */
int
pet_thread_join(pet_thread_id_t    thread_id,
		void            ** ret_val)
{

    struct pet_thread * tgt_thread;
    struct pet_thread * cur_thread;
    cur_thread = get_thread(current);
    tgt_thread = get_thread(thread_id);
    if (tgt_thread == NULL || tgt_thread->state==PET_THREAD_STOPPED) {return (-1);}
    tgt_thread->joinfrom = cur_thread->thread_id;
    cur_thread->state = PET_THREAD_BLOCKED;
    tgt_thread->state = PET_THREAD_RUNNING;
    current=tgt_thread->thread_id;
    pet_thread_ready_count--;
    if(ret_val!=NULL)
    {
        *ret_val = &tgt_thread->return_val;

    }
    __switch_to_stack(&tgt_thread->stackPtr,&cur_thread->stackPtr,cur_thread->thread_id,tgt_thread->thread_id);

    return 0;

 }

/*
 * Captures the return value of the exiting thread in ret_val
 * Changes the exiting thread's state to STOPPED *
 * Check if there are any joining threads on the exiting threads and UNBLOCKS them *
 */
void
pet_thread_exit(void * ret_val)
{
    struct pet_thread *current_exit=get_thread(current);
    current_exit->return_val=ret_val;
    current_exit->state = PET_THREAD_STOPPED;

    if(current_exit->joinfrom != -1)
    {
        struct pet_thread *joinThread = get_thread(current_exit->joinfrom);
        joinThread->state= PET_THREAD_READY;
        pet_thread_ready_count++;
    }
    __switch_to_stack(&master_thread->stackPtr,&current_exit->stackPtr,current_exit->thread_id,master_thread->thread_id);
}

/*
 * Entry point for a thread execution
 * Executes the threads' function with the arguments
 * Captures the return value of the exiting thread in return_val
 * After execution, changes the state of the thread to STOPPED
 * Check if there are any joining threads on the exiting threads and UNBLOCKS them
 */
static int
__thread_invoker(struct pet_thread * thread)
{

    void* retVal = thread->function(thread->args);
    thread->return_val=retVal;
    thread->state = PET_THREAD_STOPPED;
    if(thread->joinfrom != -1)
    {
        struct pet_thread *joinThread = get_thread(thread->joinfrom);
        joinThread->state= PET_THREAD_READY;
        pet_thread_ready_count++;
    }
        __switch_to_stack(&master_thread->stackPtr,&thread->stackPtr,thread->thread_id,master_thread->thread_id);
}

/*
 * Allocates thread control block
 * Allocates thread stack
 * Positions the stack pointer to the correct location in the stack
 * Creating thread context by assigning values to the registers
 *  Adding TCB to the ready queue
 */
int
pet_thread_create(pet_thread_id_t * thread_id,
		  pet_thread_fn_t   func,
		  void            * arg)
{
    struct pet_thread * new_thread;
    new_thread = (struct pet_thread *)malloc(sizeof(struct pet_thread));
    new_thread->thread_id = (pet_thread_id_t)thread_ids++;
    new_thread->state = PET_THREAD_READY;
    new_thread->joinfrom = -1;
    new_thread->stackPtrTop = calloc(1,STACK_SIZE);
    char * sptr = (new_thread->stackPtrTop + STACK_SIZE - sizeof(struct exec_ctx));
    new_thread->stackPtr = sptr;
    struct exec_ctx *cont = sptr ;
    cont->rip = (uint64_t)__thread_invoker;
    new_thread->function = func;
    new_thread->args = arg;
    cont->rdi = (uint64_t)new_thread;
    *thread_id = new_thread->thread_id;
    list_add_tail(&(new_thread->node),&(readyQueue.node));
    pet_thread_ready_count++;
    DEBUG("Created new Pet Thread (%p):\n", new_thread);
    DEBUG("--Add thread state here--\n");
    __dump_stack(new_thread);
    return 0;
}

/*
 * free the exited thread's stack and TCB
 */
void
pet_thread_cleanup(pet_thread_id_t prev_id,
		   pet_thread_id_t my_id)
{
    if (prev_id == PET_MASTER_THREAD_ID) return;
    if(get_thread(prev_id)->state==PET_THREAD_STOPPED)
    {
        free(get_thread(prev_id)->stackPtrTop);
	    free(get_thread(prev_id));
        
    }
}

/*
 * Changes current thread's state to READY
 * Changes target thread's state to RUNNING
 * Switches context from current to target thread
 */
static void
__yield_to(struct pet_thread * tgt_thread)
{

    if (tgt_thread == NULL ) {
        DEBUG ("NOT YIELDING TO NULL TARGET\n");
        return;
    }
    struct pet_thread *runningThread = get_thread(current);
    runningThread->state = PET_THREAD_READY;
    tgt_thread->state = PET_THREAD_RUNNING;
    current = tgt_thread->thread_id;
    __switch_to_stack(&tgt_thread->stackPtr,&runningThread->stackPtr,runningThread->thread_id,tgt_thread->thread_id);

}

/*
 * calls __yeild_to
 */
int
pet_thread_yield_to(pet_thread_id_t thread_id)
{
    struct pet_thread *tgt;
    tgt  = get_thread(thread_id);
    if (tgt == NULL) return -1;
    __yield_to(tgt);
    return 0;
}

/*
 * Iterates through the queue in Round Robin fashion
 * Checks the state of the thread, if READY changes to RUNNING
 * Switches context from current to target thread
 */
int
pet_thread_schedule()

{
    struct list_head *pos;    
    struct pet_thread *tmp, *next;
    do {
        list_for_each_safe(pos,next, &readyQueue.node) {
		tmp=list_entry(pos,struct pet_thread, node);
            if(tmp->state == PET_THREAD_READY)
            {
                current = tmp->thread_id;
                tmp->state = PET_THREAD_RUNNING;
                pet_thread_ready_count--;
                __switch_to_stack(&tmp->stackPtr,&master_thread->stackPtr,master_thread->thread_id,tmp->thread_id);
            }

        }

    }while(pet_thread_ready_count>0);

    return 0;
}

/*
 * Calls the scheduler
 * Does not exit until all threads are completed
 */
int
pet_thread_run()
{  
    int pet_id;
    pet_id = pet_thread_schedule();
    DEBUG("Pet Thread execution has finished\n");
    return 0;
}
	     
