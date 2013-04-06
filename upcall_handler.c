/* $Id: upcall_handler.c,v 1.16 2004/02/01 01:00:29 anupamc Exp $ */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/kse.h>   
#include <sys/sysctl.h>
#include <assert.h>
#include <err.h>
#include <unistd.h>
#include "upcall_handler.h"

#define MAIN_STACK_SIZE                 (1024 * 1024)
#define THREAD_STACK_SIZE               (32 * 1024)

#define COUNT_UPCALLS

/* Functions implemented in assembly */                      
extern int uts_to_thread(struct kse_thr_mailbox *tdp,
                         struct kse_thr_mailbox **curthreadp);  
extern int thread_to_uts(struct kse_thr_mailbox *tm,
		                  struct kse_mailbox *km);

static void laio_init(void) __attribute__ ((constructor));

void *laio_desc_bg = NULL;
struct uts_data sched_data;
struct uts_data poll_data;
struct kse_thr_mailbox *current_thread = NULL;
struct kse_thr_mailbox *thr_in_laio_poll = NULL;

struct kse_thr_mailbox *free_threads = NULL; 
struct kse_thr_mailbox **free_threads_tail = &free_threads;
struct kse_thr_mailbox *completed_threads = NULL; 
struct kse_thr_mailbox **completed_threads_tail = &completed_threads;

int nblocked_threads = 0;
int max_blocked = 0;

int upcall_count = 0;
int completed_events = 0;

#ifdef COUNT_UPCALLS

#define TEMPHASHTABLESIZE 37
long long nall_upcalls = 0;
long long nblocking_upcalls = 0;
long long nunblocking_upcalls = 0;
long long nblocking_unblocking_upcalls = 0;
int functions[TEMPHASHTABLESIZE][2];
int release_flag = 0;

void func_block(void *p)
{
    int i;
    i = ((int) p%TEMPHASHTABLESIZE);
    if (!functions[i][0])
		functions[i][0] = (int) p;
    assert(functions[i][0] == (int) p);
    functions[i][1]++;
}

#endif /* COUNT_UPCALLS */

static struct kse_thr_mailbox *
get_new_thread(void)
{
	struct kse_thr_mailbox *km;
	if (free_threads) {
		km = free_threads;
		free_threads = free_threads->tm_next;
		if (free_threads == NULL)
			free_threads_tail = &free_threads;
	}
	else {
		km = (struct kse_thr_mailbox *) malloc(sizeof(struct kse_thr_mailbox));
	}
	memset(km, 0, sizeof(struct kse_thr_mailbox));
	return (km);
}

/*                     
 * Upcall handler.   
 */                                        
static void                
upcall_handler(struct kse_mailbox *km)  
{
    struct kse_thr_mailbox *tm, *p, *thr_completed, *new_thread;  
    struct uts_data *data;      

    assert(!km->km_curthread);

	upcall_count++;

    new_thread = NULL;                              

    /*
     * Insert any processes back from being blocked
     * in the kernel into the run queue. 
     */ 
    data = km->km_udata;
    p = km->km_completed;
    thr_completed = km->km_completed;
    km->km_completed = NULL; 

    while ((tm = p) != NULL) {         

		p = tm->tm_next;               

		/*  
		 * If callback due to preemption of the current thread, 
		 * just continue executing the curent thread, as we don't want threads preempting one another.
		 */ 
		if (tm == current_thread) {
			assert(!new_thread);
			new_thread = tm;
#ifdef COUNT_UPCALLS
			/* count kevent upcalls, can only count after thread unblocks 
			 * to be able to see the pc */
			if (release_flag)
				func_block((void *) current_thread->tm_context.uc_mcontext.mc_eip);
#endif
		}
		else {
			/* schedule thread's event handler and reclaim current thread */
			//int ret;

#ifdef COUNT_UPCALLS
			/* upcalls breakdown, can only count after thread unblocks to be able to see the pc */
			func_block((void *) tm->tm_context.uc_mcontext.mc_eip);
#endif
			if (thr_in_laio_poll == tm) {
				continue;
			}
			completed_events++;
			nblocked_threads--;
			assert(nblocked_threads >= 0);
			*completed_threads_tail = tm;
			completed_threads_tail = &tm->tm_next;
			tm->tm_next = NULL;
		}
    } 
#ifdef COUNT_UPCALLS

    if (new_thread == NULL && !release_flag)
		nblocking_upcalls++;
    if (thr_completed)
		nunblocking_upcalls++;
    if (new_thread == NULL && thr_completed && !release_flag)
		nblocking_unblocking_upcalls++;
    if (new_thread)
		release_flag = 0;
    nall_upcalls++;      

#endif


    if (!thr_in_laio_poll && new_thread == NULL) {
		/*  
		 * Callback not due to preemption, if due to blocking at kevent, 
		 * and there are ready unblocked threads, interrupt kevent, 
		 * so that we return to the event loop and run the ready threads.
		 */ 
		if (0 && thr_in_laio_poll) {

			if (thr_completed) {
				assert(current_thread == thr_in_laio_poll);
				kse_thr_interrupt(thr_in_laio_poll, KSE_INTR_INTERRUPT, 0);
			}
			/* 
			 * We are here meaning we blocked on kevent, 
			 * so release the kse, and wait for another upcall.
			 */
#ifdef COUNT_UPCALLS
			release_flag = 1;
#endif
			kse_release(NULL);
			/* 
			 * shouldn't come here 
			 */
			printf("kse_release failed\n");
			
		}
		/* we are here, meaning we blocked on some blocking syscall,
		 * that the application has registered interest to implement as non-blocking.
		 * jump back to the stack (steal it), and add the current thread to the
		 * hashtable, to wait for its completion. 
		 */ 
		new_thread = get_new_thread();
		assert(new_thread);
		nblocked_threads++;

#ifdef COUNT_UPCALLS
		if (nblocked_threads > max_blocked) {
			max_blocked = nblocked_threads;
		}
#endif
		laio_desc_bg = current_thread;
		current_thread = new_thread;
		/* printf("data->env = %p\n", data->env); */
		_longjmp(data->env, 1);
    }
	if (thr_in_laio_poll) {
		assert(!new_thread);
		current_thread = thr_in_laio_poll;
		_longjmp(poll_data.env, 1);
	}
    assert(new_thread); 
    uts_to_thread(new_thread, &km->km_curthread);  
    printf("\n-- uts_to_thread() failed --\n");
    exit(0); 
}


void                 
kse_init(struct uts_data *data) 
{
    struct kse_thr_mailbox *tm;   
    int mib[2];                
    char *p;  
    size_t len;  

    /*  
     * Create initial thread.  
     */                               
    tm = calloc(1, sizeof(struct kse_thr_mailbox));

    /* Find our stack. */     
    mib[0] = CTL_KERN;  
    mib[1] = KERN_USRSTACK;
    len = sizeof(p);     
    if (sysctl(mib, 2, &p, &len, NULL, 0) == -1)    
		printf("sysctl(CTL_KER.KERN_USRSTACK) failed.\n");
    tm->tm_context.uc_stack.ss_sp = p - MAIN_STACK_SIZE; 
    tm->tm_context.uc_stack.ss_size = MAIN_STACK_SIZE; 
    tm->tm_udata = tm->tm_context.uc_stack.ss_sp;

    /*    
     * Inititalize KSE mailbox. 
	 */
    p = malloc(THREAD_STACK_SIZE); 
    bzero(&data->mb, sizeof(struct kse_mailbox));  
    data->mb.km_stack.ss_sp = p;            
    data->mb.km_stack.ss_size = THREAD_STACK_SIZE;   
    data->mb.km_func = (void *) upcall_handler;
    data->mb.km_quantum = 8000000;
    data->mb.km_udata = data;                    
    data->mb.km_curthread = tm;  

    /*    
     * Create KSE mailbox. 
     */                                                                        
    if (kse_create(&data->mb, 0) != 0)
		err(1, "kse_create");
    else {
		data->mb.km_curthread = NULL;
		data->mb.km_curthread = tm;
    }
}

static void
laio_init(void)
{
	kse_init(&sched_data);
	current_thread = sched_data.mb.km_curthread;
	sched_data.mb.km_curthread = NULL;
}

