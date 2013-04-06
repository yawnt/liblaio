##
## $Id: Makefile,v 1.4 2004/01/13 08:21:54 anupamc Exp $
##
CFLAGS += -Wall -g -O -fpic -fPIC

OBJS = 	kse_asm.o \
		laio_api.o \
		laio_syscall.o \
		upcall_handler.o

SRCS =	kse_asm.S \
		laio_syscall.S \
		laio_api.c \
		upcall_handler.c

#liblaio.a:	${OBJS}
#	${AR} ${ARFLAGS} ${.TARGET} ${.OODATE}
#	${RANLIB} ${.TARGET}

liblaio.so: ${OBJS}
	${CC} -shared -o ${.TARGET} ${.OODATE}

clean:
	rm -f ${OBJS} liblaio.so

depend:
	mkdep ${CFLAGS} ${SRCS}
