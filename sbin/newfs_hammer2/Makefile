PROG=	newfs_hammer2

SRCS=	$(PROG).c

OBJS := $(SRCS:.c=.o)

CC=	gcc
CFLAGS+= -I../../sys -Wall -g

.PHONY: all clean

all: $(PROG)
$(PROG): $(OBJS) ../hammer2/ ../../lib/libc/gen/ ../../sys/libkern/ ../../sys/vfs/hammer2/xxhash
	$(CC) $(CFLAGS) -o $@ $(OBJS) ../hammer2/uuid.o ../../lib/libc/gen/sysctlbyname.o ../../sys/libkern/icrc32.o ../../sys/vfs/hammer2/xxhash/xxhash.o -luuid
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
