SRCS1=	strlcpy.c

OBJS1 := $(SRCS1:.c=.o)

CC=	gcc
CFLAGS+= -I. -Wall -g

.PHONY: all clean

all: $(OBJS1)
.c.o:
	$(CC) $(CFLAGS) -c $<
clean:
	rm -f ./*.o
