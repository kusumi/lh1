SRCS1=	hexdump.c pidfile.c flopen.c humanize_unsigned.c realhostname.c trimdomain.c getmntopts.c

OBJS1 := $(SRCS1:.c=.o)

CC=	gcc
CFLAGS+= -I. -I../../include -Wall -g

.PHONY: all clean

all: $(OBJS1)
.c.o:
	$(CC) $(CFLAGS) -c $<
clean:
	rm -f ./*.o
