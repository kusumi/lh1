SUBDIRS = sbin/hammer sbin/newfs_hammer sbin/mount_hammer sbin/hammer2 sbin/newfs_hammer2 sbin/mount_hammer2 usr.bin/undo usr.sbin/fstyp lib/libc/gen lib/libc/string lib/libutil lib/libdmsg sys/libkern sys/crypto/sha2 sys/vfs/hammer2/xxhash
BINDIRS = sbin/hammer sbin/newfs_hammer sbin/mount_hammer sbin/hammer2 sbin/newfs_hammer2 sbin/mount_hammer2 usr.bin/undo usr.sbin/fstyp share/man/man5

.PHONY: all clean $(SUBDIRS) ${BINDIRS}

all: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@
sbin/hammer: lib/libc/gen lib/libutil sys/libkern sys/crypto/sha2
sbin/newfs_hammer: sbin/hammer
sbin/mount_hammer: lib/libutil
sbin/hammer2: lib/libc/gen lib/libc/string lib/libutil lib/libdmsg sys/vfs/hammer2/xxhash
sbin/newfs_hammer2: sbin/hammer2 lib/libc/gen sys/libkern sys/vfs/hammer2/xxhash
sbin/mount_hammer2: lib/libutil
clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $@; \
	done
install:
	for dir in $(BINDIRS); do \
		$(MAKE) -C $$dir $@; \
	done
uninstall:
	for dir in $(BINDIRS); do \
		$(MAKE) -C $$dir $@; \
	done
