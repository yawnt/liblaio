/* $Id: upcall_handler.h,v 1.11 2004/02/01 01:00:29 anupamc Exp $ */

#ifndef _UPCALL_HANDLER_H_
#define _UPCALL_HANDLER_H_

#include <sys/kse.h>
#include <sys/queue.h>
#include <setjmp.h>
#include "laio_api.h"

/* carry flag is at the least significant bit of the eflags register. */
#define CARRY_FLAG 0x1

struct uts_data {
    struct kse_mailbox      mb;  
    jmp_buf                 env; /* store state to be able to jump back
								  * to the stack from the scheduler
								  */
};

extern void *laio_desc_bg;
extern struct uts_data sched_data;
extern struct uts_data poll_data;
extern struct kse_thr_mailbox *current_thread;

extern int nblocked_threads;
extern struct kse_thr_mailbox *thr_in_laio_poll;

extern struct kse_thr_mailbox *free_threads;
extern struct kse_thr_mailbox **free_threads_tail;
extern struct kse_thr_mailbox *completed_threads;
extern struct kse_thr_mailbox **completed_threads_tail;

/*
#define NON_BLOCKING(func,ret,args...) { \
    if (!_setjmp(sched_data.env)) { \
		sched_data.mb.km_curthread = current_thread; \
		thread_desc = (int) current_thread; \
		ret = func(args); \
		sched_data.mb.km_curthread = NULL; \
		thread_desc = 0; \
    } \
    else { \
		ret = -1 ; \
		errno = EAGAIN; \
    } \
}
*/

void kse_init(struct uts_data *data);

#endif /* !_UPCALL_HANDLER_H_ */

