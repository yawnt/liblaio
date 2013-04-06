#include <sys/syscall.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <laio_api.h>

void
panic(const char *s)
{
	printf("terminating: %s\n", s);
	exit(-1);
}

int
main(int argc, char *argv[])
{
	struct laio_completion c[1];
	struct timespec rqt = { 1, 0 };
	void *handle;
	int i, rv;

	if ((rv = laio_syscall(SYS_nanosleep, &rqt, NULL)) == -1 &&
	    errno == EINPROGRESS) {
		handle = laio_gethandle();
		printf("handle = %p\n", handle);
		/*
		 * Block until the laio completes.
		 */
		rv = laio_poll(c, 1, NULL);
		printf("rv = %d\n", rv);
		/*
		 * Verify the completion of the nanosleep.
		 */
		for (i = 0; i < rv; i++)
			if (c[i].laio_h == handle)
				goto ok;
		panic("");
	ok:
		printf("ok\n");
	}
	return (0);
}

