PROG=	hammer2

SRCS=	main.c subs.c icrc.c cmd_remote.c cmd_snapshot.c cmd_pfs.c cmd_service.c cmd_leaf.c cmd_debug.c cmd_rsa.c cmd_stat.c cmd_setcomp.c cmd_setcheck.c cmd_bulkfree.c cmd_cleanup.c cmd_info.c cmd_destroy.c print_inode.c uuid.c

OBJS := $(SRCS:.c=.o)

CC=	gcc
CFLAGS+= -I../../include -I../../sys -I../../lib/libutil -I../../lib/libdmsg -Wall -g
CFLAGS+= -DXXH_NAMESPACE=h2_

.PHONY: all clean

all: $(PROG)
$(PROG): $(OBJS) ../../lib/libc/gen ../../lib/libc/string ../../lib/libutil ../../lib/libdmsg ../../sys/vfs/hammer2/xxhash
	$(CC) $(CFLAGS) -o $@ $(OBJS) ../../lib/libc/gen/sysctlbyname.o ../../lib/libc/string/strlcpy.o ../../lib/libutil/trimdomain.o ../../lib/libutil/realhostname.o ../../lib/libdmsg/crypto.o ../../lib/libdmsg/debug.o ../../lib/libdmsg/icrc.o ../../lib/libdmsg/msg.o ../../lib/libdmsg/msg_lnk.o ../../lib/libdmsg/service.o ../../lib/libdmsg/subs.o ../../lib/libdmsg/uuid.o ../../sys/vfs/hammer2/xxhash/xxhash.o -lm -luuid -lpthread -lssl -lcrypto
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
