PROG=	newfs_hammer

SRCS=	$(PROG).c

OBJS := $(SRCS:.c=.o)

CC=	gcc
CFLAGS+= -I../../sys -I../hammer -Wall -g

.PHONY: all clean

all: $(PROG)
$(PROG): $(OBJS) ../hammer/ ../../lib/libc/gen/ ../../sys/libkern/
	$(CC) $(CFLAGS) -o $@ $(OBJS) ../hammer/ondisk.o ../hammer/cache.o ../hammer/blockmap.o ../hammer/misc.o ../hammer/uuid.o ../../lib/libc/gen/sysctlbyname.o ../../sys/libkern/crc32.o ../../sys/libkern/icrc32.o -luuid
.c.o:
	$(CC) $(CFLAGS) -c $<
clean:
	rm -f ./*.o ./$(PROG)
