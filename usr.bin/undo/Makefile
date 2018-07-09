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
install:
	install -m 755 ./${PROG} /usr/local/bin/ || exit 1
	cat ./${PROG}.1 | gzip -9 -n > ./${PROG}.1.gz || exit 1
	install -m 644 ./${PROG}.1.gz /usr/local/share/man/man1/ || exit 1
	rm ./${PROG}.1.gz || exit 1
uninstall:
	rm /usr/local/bin/${PROG} || exit 1
	rm /usr/local/share/man/man1/${PROG}.1.gz || exit 1
