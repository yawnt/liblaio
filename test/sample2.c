#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <laio_api.h>

#define MAX 4

int
main(int argc, char *argv[])
{
	struct laio_completion c[MAX];
	struct timespec rqt;
	void *handle[MAX];
	int i, j, x, rv, laio_completed;
	int timo[MAX];

	srandom(getpid());

	for (j = 0; j < 2; j++) {
		for (i = 0; i < MAX; i++) {
			x = random()%10 + 1;
			timo[i] = x;
			rqt.tv_sec = x;
			rqt.tv_nsec = 0;
			rv = laio_syscall(SYS_nanosleep, &rqt, NULL);
			assert(rv == -1 && errno == EINPROGRESS);
			handle[i] = laio_gethandle();
			printf("Timeout %d: %d seconds handle = %p\n", i, x, handle[i]);
		}

		laio_completed = 0;
		do {
			rv = laio_poll(c + laio_completed, MAX - laio_completed, NULL);
			laio_completed += rv;
		} while (laio_completed < MAX);

		for (i = 0; i < MAX; i++) {
			printf("laio %d: handle = %p ret = %d errno = %d\n",
					i, c[i].laio_h, c[i].laio_rv, c[i].laio_errno);
		}
	}

	return (0);
}

