CC =	gcc
CFLAGS +=	-g -Wall -I../
LDFLAGS +=	-L../ -llaio

.c.o:
	@rm -f $@
	$(CC) $(CFLAGS) -c $*.c

SRC =	laio_syscall_test.c \
		sample.c \
		sample2.c

OBJ = 	$(SRC:.c=.o)

ALL =	laio_syscall_test sample sample2

CLEANFILES =	$(ALL) $(OBJ)

all: $(ALL)

laio_syscall_test: laio_syscall_test.o
		@rm -f $@
		$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $>

sample: sample.o
		@rm -f $@
		$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $>

sample2: sample2.o
		@rm -f $@
		$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $>

clean:
	rm -f $(CLEANFILES)
