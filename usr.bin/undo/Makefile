PROG=	undo

SRCS=	$(PROG).c

OBJS := $(SRCS:.c=.o)

CC=	gcc
CFLAGS+= -I../../sys -Wall -g

.PHONY: all clean

all: $(PROG)
$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)
.c.o:
	$(CC) $(CFLAGS) -c $<
clean:
	rm -f ./*.o ./$(PROG)
