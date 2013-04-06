/* $Id: laio_api.h,v 1.12 2004/06/23 19:07:46 kdiaa Exp $ */

#ifndef _LAIO_API_H_
#define _LAIO_API_H_

#include <time.h>

extern int upcall_count;
extern int completed_events;

struct laio_completion {
	void *laio_h; /* laio descriptor */
	int laio_rv; /* laio return value */
	int laio_errno; /* laio error code */
};

/* LAIO API */
void	*laio_gethandle(void);
int	 laio_poll(struct laio_completion completions[], int ncompletions,
	     const struct timespec *);
int	 laio_syscall(int number, ...);

#endif /* !_LAIO_API_H_ */
