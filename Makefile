SUBDIRS = sbin/hammer sbin/newfs_hammer sbin/newfs_hammer2 usr.bin/undo usr.sbin/fstyp lib/libc/gen lib/libutil sys/libkern sys/crypto/sha2 sys/vfs/hammer2/xxhash

.PHONY: all clean $(SUBDIRS)

all: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@
sbin/newfs_hammer: sbin/hammer
sbin/hammer: lib/libc/gen lib/libutil sys/libkern sys/crypto/sha2
sbin/newfs_hammer2: lib/libc/gen sys/libkern sys/vfs/hammer2/xxhash
clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $@; \
	done
