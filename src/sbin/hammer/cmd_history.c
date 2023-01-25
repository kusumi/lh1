/*
 * Copyright (c) 2008 The DragonFly Project.  All rights reserved.
 *
 * This code is derived from software contributed to The DragonFly Project
 * by Matthew Dillon <dillon@backplane.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of The DragonFly Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific, prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $DragonFly: src/sbin/hammer/cmd_history.c,v 1.4 2008/06/24 17:40:21 dillon Exp $
 */

#include "hammer.h"

typedef struct cmd_attr {
	char *path;
	int64_t offset;
	long length;
} cmd_attr_t;

static void hammer_do_history(const char *path, off_t off, long len);
static int parse_attr(const char *s, cmd_attr_t *ca);
static int parse_attr_path(const char *s, cmd_attr_t *ca);
static void dumpat(const char *path, off_t off, long len);
static const char *timestr32(uint32_t time32);
static __inline int test_strtol(int res, long val);
static __inline int test_strtoll(int res, long long val);

/*
 * history <file1> ... <fileN>
 */
void
hammer_cmd_history(const char *offset_str, char **av, int ac)
{
	int i;
	int old_behavior = 0;
	cmd_attr_t ca;

	bzero(&ca, sizeof(ca));
	if (parse_attr(offset_str, &ca) == 0)
		old_behavior = 1;

	for (i = 0; i < ac; ++i) {
		if (!old_behavior)
			parse_attr_path(av[i], &ca);
		if (ca.path == NULL)
			ca.path = strdup(av[i]);
		hammer_do_history(ca.path, ca.offset, ca.length);
		free(ca.path);
		ca.path = NULL;
	}
}

static
void
hammer_do_history(const char *path, off_t off, long len)
{
	struct hammer_ioc_history hist;
	const char *status;
	int fd;
	int i;

	printf("%s\t", path);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf("%s\n", strerror(errno));
		return;
	}
	bzero(&hist, sizeof(hist));
	hist.beg_tid = HAMMER_MIN_TID;
	hist.end_tid = HAMMER_MAX_TID;

	if (off >= 0) {
		hist.head.flags |= HAMMER_IOC_HISTORY_ATKEY;
		hist.key = off;
		hist.nxt_key = off + 1;
	}


	if (ioctl(fd, HAMMERIOC_GETHISTORY, &hist) < 0) {
		printf("%s\n", strerror(errno));
		close(fd);
		return;
	}
	status = ((hist.head.flags & HAMMER_IOC_HISTORY_UNSYNCED) ?
		 "dirty" : "clean");
	printf("%016jx %s {\n", (uintmax_t)hist.obj_id, status);
	for (;;) {
		for (i = 0; i < hist.count; ++i) {
			char *hist_path = NULL;

			asprintf(&hist_path, "%s@@0x%016jx",
				 path, (uintmax_t)hist.hist_ary[i].tid);
			printf("    %016jx %s",
			       (uintmax_t)hist.hist_ary[i].tid,
			       timestr32(hist.hist_ary[i].time32));
			if (off >= 0) {
				if (VerboseOpt) {
					printf(" '");
					dumpat(hist_path, off, len);
					printf("'");
				}
			}
			printf("\n");
			free(hist_path);
		}
		if (hist.head.flags & HAMMER_IOC_HISTORY_EOF)
			break;
		if (hist.head.flags & HAMMER_IOC_HISTORY_NEXT_KEY)
			break;
		if ((hist.head.flags & HAMMER_IOC_HISTORY_NEXT_TID) == 0)
			break;
		hist.beg_tid = hist.nxt_tid;
		if (ioctl(fd, HAMMERIOC_GETHISTORY, &hist) < 0) {
			printf("    error: %s\n", strerror(errno));
			break;
		}
	}
	printf("}\n");
	close(fd);
}

static
int
parse_attr(const char *s, cmd_attr_t *ca)
{
	long long offset;
	long length;
	char *rptr;

	ca->offset = -1;  /* Don't use offset as a key */
	ca->length = 32;  /* Default dump size */

	if (*s != '@')
		return(-1);  /* not parsed */

	errno = 0;  /* clear */
	offset = strtoll(s + 1, &rptr, 0);
	if (test_strtoll(errno, offset)) {
		*rptr = '\0';  /* side effect */
		err(1, "%s", s);
		/* not reached */
	}
	ca->offset = offset;

	if (*rptr == ',') {
		errno = 0;  /* clear */
		length = strtol(rptr + 1, NULL, 0);
		if (test_strtol(errno, length)) {
			err(1, "%s", rptr);
			/* not reached */
		}
		if (length >= 0)
			ca->length = length;
	}

	return(0);
}

static
int
parse_attr_path(const char *s, cmd_attr_t *ca)
{
	int length, ret;
	char *p;
	struct stat st;

	ca->path = NULL;

	if (stat(s, &st) == 0)
		return(-1);  /* real path */

	p = strstr(s, "@");
	if (p == NULL || p == s)
		return(-1);  /* no attr specified */

	ret = parse_attr(p, ca);

	length = p - s + 1;
	ca->path = calloc(1, length);
	strncpy(ca->path, s, length - 1);

	return(ret);
}

static
void
dumpat(const char *path, off_t off, long len)
{
	char buf[1024];
	int fd;
	int n;
	int r;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return;
	lseek(fd, off, 0);
	while (len) {
		n = (len > (int)sizeof(buf)) ? (int)sizeof(buf) : len;
		r = read(fd, buf, n);
		if (r <= 0)
			break;
		len -= r;
		for (n = 0; n < r; ++n) {
			if (isprint(buf[n]))
				putc(buf[n], stdout);
			else
				putc('.', stdout);
		}
	}
	close(fd);
}

/*
 * Return a human-readable timestamp
 */
static
const char *
timestr32(uint32_t time32)
{
	static char timebuf[64];
	time_t t = (time_t)time32;
	struct tm *tp;

	tp = localtime(&t);
	strftime(timebuf, sizeof(timebuf), "%d-%b-%Y %H:%M:%S", tp);
	return(timebuf);
}

/*
 * Return non-zero on either overflow or underflow
 */
static __inline
int
test_strtol(int res, long val)
{
	return(res == ERANGE && (val == LONG_MIN || val == LONG_MAX));
}

static __inline
int
test_strtoll(int res, long long val)
{
	return(res == ERANGE && (val == LLONG_MIN || val == LLONG_MAX));
}
