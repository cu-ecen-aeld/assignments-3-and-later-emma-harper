CC=$(CROSS_COMPILE)gcc
CFLAGS= -g -Wall -Werror -pthread

LDFLAGS= -lpthread -lrt	

TARGET=aesdsocket

HFILES= queue.h
CFILES= aesdsocket.c

SRCS= ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${TARGET}

default: ${TARGET}

clean:
	-rm -f *.o *.NEW *~ *.d
	-rm -f ${TARGET} 

aesdsocket:	aesdsocket.o
	$(CC) $(CFLAGS) -o $@ aesdsocket.o $(LDFLAGS)

depend:

.c.o:
	$(CC) -MD $(CFLAGS) -c $<