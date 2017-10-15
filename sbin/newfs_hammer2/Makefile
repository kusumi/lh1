PROG=	newfs_hammer2

SRCS=	$(PROG).c uuid.c

OBJS := $(SRCS:.c=.o)

CC=	gcc
CFLAGS+= -I../../sys -Wall -g

.PHONY: all clean

all: $(PROG)
$(PROG): $(OBJS) ../../lib/libc/gen/ ../../sys/libkern/ ../../sys/vfs/hammer2/xxhash
	$(CC) $(CFLAGS) -o $@ $(OBJS) ../../lib/libc/gen/sysctlbyname.o ../../sys/libkern/icrc32.o ../../sys/vfs/hammer2/xxhash/xxhash.o -luuid
.c.o:
	$(CC) $(CFLAGS) -c $<
clean:
	rm -f ./*.o ./$(PROG)
