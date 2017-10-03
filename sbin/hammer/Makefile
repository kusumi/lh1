# Dropped files: cmd_info.c

PROG1=	hammer
PROG2=	test_dupkey

SRCS1=	$(PROG1).c ondisk.c cache.c blockmap.c misc.c uuid.c cycle.c cmd_show.c cmd_softprune.c cmd_history.c cmd_blockmap.c cmd_reblock.c cmd_rebalance.c cmd_synctid.c cmd_stats.c cmd_remote.c cmd_pfs.c cmd_snapshot.c cmd_mirror.c cmd_cleanup.c cmd_version.c cmd_volume.c cmd_config.c cmd_recover.c cmd_dedup.c cmd_abort.c cmd_strip.c
SRCS2=	$(PROG2).c

OBJS1 := $(SRCS1:.c=.o)
OBJS2 := $(SRCS2:.c=.o)

CC=	gcc
CFLAGS+= -I../../sys -I../../lib/libutil -Wall -g

.PHONY: all clean

all: $(PROG1) $(PROG2)
$(PROG1): $(OBJS1) ../../lib/libc/gen/ ../../lib/libutil/ ../../sys/libkern/ ../../sys/crypto/sha2/
	$(CC) $(CFLAGS) -o $@ $(OBJS1) ../../lib/libc/gen/getdevpath.o ../../lib/libc/gen/sysctlbyname.o ../../lib/libutil/hexdump.o ../../lib/libutil/pidfile.o ../../lib/libutil/flopen.o ../../lib/libutil/humanize_unsigned.o ../../sys/libkern/crc32.o ../../sys/libkern/icrc32.o ../../sys/crypto/sha2/sha2.o -lm -luuid
$(PROG2): $(OBJS2) ../../sys/libkern/
	$(CC) $(CFLAGS) -o $@ $(OBJS2) ../../sys/libkern/crc32.o ../../sys/libkern/icrc32.o
.c.o:
	$(CC) $(CFLAGS) -c $<
clean:
	rm -f ./*.o ./$(PROG1) ./$(PROG2)
