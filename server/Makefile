CC = $(CROSS_COMPILE)gcc
CFLAGS = -g -Wall -Werror
SRCS := aesdsocket.c 
TARGET = aesdsocket
OBJS := $(SRC:.c=.o)

all: $(TARGET)

writer: 
	$(CC) $(CFLAGS) -c $(SRCS) -o $(TARGET).o 
	$(CC) $(CFLAGS) $(TARGET).o -o $(TARGET)


clean:
	-rm -f *.o $(TARGET)