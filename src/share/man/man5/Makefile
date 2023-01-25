PROG=	hammer

install:
	cat ./${PROG}.5 | gzip -9 -n > ./${PROG}.5.gz || exit 1
	install -m 644 ./${PROG}.5.gz /usr/local/share/man/man5/ || exit 1
	rm ./${PROG}.5.gz || exit 1
uninstall:
	rm /usr/local/share/man/man5/${PROG}.5.gz || exit 1
