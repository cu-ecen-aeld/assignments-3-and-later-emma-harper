
ifeq ($(CC),)
	CC= $(CROSS_COMPILE)gcc
endif

ifeq ($(CFLAGS),)
	CFLAGS= -g -Wall -Werror -pthread
endif

ifeq ($(LDFLAGS),)
	LDFLAGS = -lpthread -lrt	
endif

TARGET=aesdsocket

CFILES= aesdsocket.c

SRCS= ${CFILES}
OBJS= ${CFILES:.c=.o}


default: ${TARGET}

all: ${TARGET}


$(TARGET): $(SRCS) 
	$(CC) $(CFLAGS) $^ -o $@ $(INCLUDES) $(LDFLAGS)

clean:
	-rm -f *.o
	-rm -f ${TARGET} 
