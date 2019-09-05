SRCS1=	getdevpath.c sysctlbyname.c setproctitle.c

OBJS1 := $(SRCS1:.c=.o)

CC=	gcc
CFLAGS+= -I. -Wall -g

.PHONY: all clean

all: $(OBJS1)
.c.o:
	$(CC) $(CFLAGS) -c $<
clean:
	rm -f ./*.o
