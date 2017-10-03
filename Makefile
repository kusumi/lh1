SUBDIRS = sbin/hammer sbin/newfs_hammer usr.bin/undo usr.sbin/fstyp lib/libc/gen lib/libutil sys/libkern sys/crypto/sha2

.PHONY: all clean $(SUBDIRS)

all: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@
sbin/newfs_hammer: sbin/hammer
sbin/hammer: lib/libc/gen lib/libutil sys/libkern sys/crypto/sha2
clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $@; \
	done
