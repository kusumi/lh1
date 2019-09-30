PROG=	fsck_hammer2

SRCS=	$(PROG).c test.c

OBJS := $(SRCS:.c=.o)

CC=	gcc
CFLAGS+= -I../../sys -I../hammer2 -I../../lib/libutil -I../../lib/libdmsg -Wall -g
CFLAGS+= -DXXH_NAMESPACE=h2_

.PHONY: all clean

all: $(PROG)
$(PROG): $(OBJS) ../hammer2/ ../../sys/libkern/ ../../sys/vfs/hammer2/xxhash
	$(CC) $(CFLAGS) -o $@ $(OBJS) ../hammer2/uuid.o ../hammer2/subs.o ../../sys/libkern/icrc32.o ../../sys/vfs/hammer2/xxhash/xxhash.o -luuid -lcrypto
.c.o:
	$(CC) $(CFLAGS) -c $<
clean:
	rm -f ./*.o ./$(PROG)
install:
	install -m 755 ./${PROG} /usr/local/bin/ || exit 1
	cat ./${PROG}.8 | gzip -9 -n > ./${PROG}.8.gz || exit 1
	install -m 644 ./${PROG}.8.gz /usr/local/share/man/man8/ || exit 1
	rm ./${PROG}.8.gz || exit 1
uninstall:
	rm /usr/local/bin/${PROG} || exit 1
	rm /usr/local/share/man/man8/${PROG}.8.gz || exit 1
