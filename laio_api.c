/* $Id: laio_api.c,v 1.14 2004/06/23 19:07:46 kdiaa Exp $ */

#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include "upcall_handler.h"
#include "laio_api.h"

void *foo = NULL;

#if 0
int
laio_syscall(int number, ...)
{
	if (!_setjmp(sched_data.env)) {
		assert(current_thread);
		sched_data.mb.km_curthread = current_thread;
		thread_desc = current_thread;
		//rv = syscall(number, arg_list);
		//the asm code here
		{asm (
			" \
				pop %ecx; \
				pop %eax; \
				push %ecx; \
				int $0x80; \
				push %ecx; \
				jb 1f; \
				jmp 2f; \
			1: \
				pushl %ebx; \
				call 1f; \
			1: \
				popl %ebx; \
				addl $_GLOBAL_OFFSET_TABLE_+[.-1b],%ebx; \
				jmp .cerror@PLT; \
			2: \
			");
		}
		sched_data.mb.km_curthread = NULL;
		thread_desc = (void *) 0;
		asm("ret");
		assert(0);
	}
	else {
		laio_desc_bg = thread_desc;
		errno = EINPROGRESS;
		return (-1);
	}

	/* does not come here */
	assert(0);
	return (0);
}
#endif

void *
laio_gethandle(void)
{
	return (laio_desc_bg);
}

int
laio_poll(struct laio_completion completions[], int ncompletions, const struct timespec *timeout)
{
	struct kse_thr_mailbox *ktm = NULL;
	int i;
	if (completed_threads == NULL && ncompletions > 0) {
#if 0
		/* we have to wait */
		if (timeout == NULL) {
			struct timespec ts = { 60, 0 };
			/* enable upcalls */
			if (nblocked_threads > 0) {
				sched_data.mb.km_curthread = current_thread;
				thr_in_laio_poll = current_thread;
			}
			/* infinite timeout */
			for (;;) {
				nanosleep(&ts, NULL);
				if (completed_threads != NULL)
					break;
			}
			/* disable upcalls */
			sched_data.mb.km_curthread = NULL;
			thr_in_laio_poll = NULL;
		}
		else {
			long tv_time = timeout->tv_sec * 1000000000 + timeout->tv_nsec;
			if (tv_time < 0) {
				errno = EINVAL;
				return (-1);
			}
			/* enable upcalls */
			if (nblocked_threads > 0) {
				sched_data.mb.km_curthread = current_thread;
				thr_in_laio_poll = current_thread;
			}
			nanosleep(timeout, NULL);
			/* disable upcalls */
			sched_data.mb.km_curthread = NULL;
			thr_in_laio_poll = NULL;
		}
#endif
		thr_in_laio_poll = current_thread;
		if (!_setjmp(poll_data.env)) {
			kse_release((struct timespec *) timeout);
			/* cannot come here */
			assert(0);
		}
		/* 
		 * will come here when something unblocks or
		 * timer expires
		 */
		thr_in_laio_poll = NULL;
	}
	for (i = 0; i < ncompletions && completed_threads != NULL; i++) {
		int ret;
		ktm = completed_threads;
		completed_threads = completed_threads->tm_next;
		if (completed_threads == NULL)
			completed_threads_tail = &completed_threads;
		*free_threads_tail = ktm;
		free_threads_tail = &ktm->tm_next;
		ktm->tm_next = NULL;
		completions[i].laio_h = ktm;
		ret = ktm->tm_context.uc_mcontext.mc_eax;
		if (ktm->tm_context.uc_mcontext.mc_eflags & CARRY_FLAG) {
			completions[i].laio_rv = -1;
			completions[i].laio_errno = ret;
		}
		else {
			completions[i].laio_rv = ret;
			completions[i].laio_errno = 0;
		}
	}
	return (i);
}

