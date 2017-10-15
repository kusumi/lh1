/*-
 * Copyright (c) 2016 The DragonFly Project
 * Copyright (c) 2014 The FreeBSD Foundation
 * All rights reserved.
 *
 * This software was developed by Edward Tomasz Napierala under sponsorship
 * from the FreeBSD Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fstyp.h"

#define	LABEL_LEN	256

typedef int (*fstyp_function)(FILE *, char *, size_t);
typedef int (*fsvtyp_function)(const char *, char *, size_t);

static struct {
	const char	*name;
	fstyp_function	function;
	bool		unmountable;
} fstypes[] = {
	{ "hammer", &fstyp_hammer, false },
	{ "hammer2", &fstyp_hammer2, false },
	{ NULL, NULL, NULL }
};

static struct {
	const char	*name;
	fsvtyp_function	function;
	bool		unmountable;
} fsvtypes[] = {
	{ "hammer", &fsvtyp_hammer, false }, /* Must be before partial */
	{ "hammer(partial)", &fsvtyp_hammer_partial, true },
	// XXX hammer2 not supported yet
	{ NULL, NULL, NULL }
};

void *
read_buf(FILE *fp, off_t off, size_t len)
{
	int error;
	size_t nread;
	void *buf;

	error = fseek(fp, off, SEEK_SET);
	if (error != 0) {
		warn("cannot seek to %jd", (uintmax_t)off);
		return (NULL);
	}

	buf = malloc(len);
	if (buf == NULL) {
		warn("cannot malloc %zd bytes of memory", len);
		return (NULL);
	}

	nread = fread(buf, len, 1, fp);
	if (nread != 1) {
		free(buf);
		if (feof(fp) == 0)
			warn("fread");
		return (NULL);
	}

	return (buf);
}

char *
checked_strdup(const char *s)
{
	char *c;

	c = strdup(s);
	if (c == NULL)
		err(1, "strdup");
	return (c);
}

void
rtrim(char *label, size_t size)
{
	ptrdiff_t i;

	for (i = size - 1; i >= 0; i--) {
		if (label[i] == '\0')
			continue;
		else if (label[i] == ' ')
			label[i] = '\0';
		else
			break;
	}
}

static void
usage(void)
{

	fprintf(stderr, "usage: fstyp [-l] [-s] [-u] special\n");
	exit(1);
}

static void
type_check(const char *path, FILE *fp)
{
	int error, fd;
	struct stat sb;
	size_t size;

	fd = fileno(fp);

	error = fstat(fd, &sb);
	if (error != 0)
		err(1, "%s: fstat", path);

	if (S_ISREG(sb.st_mode))
		return;

	error = ioctl(fd, BLKGETSIZE, &size);
	if (error != 0)
		errx(1, "%s: not a disk", path);
}

int
main(int argc, char **argv)
{
	int ch, error, i;
	bool ignore_type = false, show_label = false, show_unmountable = false;
	char label[LABEL_LEN + 1];
	char *path;
	const char *name = NULL;
	FILE *fp;
	fstyp_function fstyp_f;
	fsvtyp_function fsvtyp_f;

	while ((ch = getopt(argc, argv, "lsu")) != -1) {
		switch (ch) {
		case 'l':
			show_label = true;
			break;
		case 's':
			ignore_type = true;
			break;
		case 'u':
			show_unmountable = true;
			break;
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;
	if (argc != 1)
		usage();

	path = argv[0];

	fp = fopen(path, "r");
	if (fp == NULL)
		goto fsvtyp; /* DragonFly */

	if (ignore_type == false)
		type_check(path, fp);

	memset(label, '\0', sizeof(label));

	for (i = 0;; i++) {
		if (show_unmountable == false && fstypes[i].unmountable == true)
			continue;
		fstyp_f = fstypes[i].function;
		if (fstyp_f == NULL)
			break;

		error = fstyp_f(fp, label, sizeof(label));
		if (error == 0) {
			name = fstypes[i].name;
			goto done;
		}
	}
fsvtyp:
	for (i = 0;; i++) {
		if (show_unmountable == false && fsvtypes[i].unmountable == true)
			continue;
		fsvtyp_f = fsvtypes[i].function;
		if (fsvtyp_f == NULL)
			break;

		error = fsvtyp_f(path, label, sizeof(label));
		if (error == 0) {
			name = fsvtypes[i].name;
			goto done;
		}
	}

	warnx("%s: filesystem not recognized", path);
	return (1);
done:
	if (show_label && label[0] != '\0') {
		printf("%s %s\n", name, label);
	} else {
		printf("%s\n", name);
	}

	return (0);
}
