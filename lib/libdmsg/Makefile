SRCS1=	icrc.c debug.c subs.c crypto.c msg.c msg_lnk.c service.c uuid.c

OBJS1 := $(SRCS1:.c=.o)

CC=	gcc
CFLAGS+= -I. -I../../include -I../libutil -I../../sys -Wall -g

.PHONY: all clean

all: $(OBJS1)
.c.o:
	$(CC) $(CFLAGS) -c $<
clean:
	rm -f ./*.o
