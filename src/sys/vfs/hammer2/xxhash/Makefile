SRCS1=	xxhash.o

OBJS1 := $(SRCS1:.c=.o)

CC=	gcc
CFLAGS+= -Wall -g

.PHONY: all clean

all: $(OBJS1)
.c.o:
	$(CC) $(CFLAGS) -c $<
clean:
	rm -f ./*.o
