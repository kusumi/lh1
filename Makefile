SUBDIRS = src

.PHONY: all clean $(SUBDIRS)

all: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@
clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $@; \
	done
install:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $@; \
	done
uninstall:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $@; \
	done
