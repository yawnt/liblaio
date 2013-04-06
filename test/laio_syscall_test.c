#include <sys/syscall.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <laio_api.h>

int main()
{
	int rv, rv2;

	rv = syscall(SYS_getpid);
	printf("rv = %d\n", rv);
	rv2 = laio_syscall(SYS_getpid);
	printf("rv2 = %d\n", rv2);

	return (0);
}
