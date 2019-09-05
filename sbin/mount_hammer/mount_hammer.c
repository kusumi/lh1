/*
 * Copyright (c) 2007 The DragonFly Project.  All rights reserved.
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
 */

#include <sys/types.h>
#include <sys/stat.h>
#include "./mount.h"
#include <libutil.h> // before <vfs/hammer/hammer_mount.h>
#include <vfs/hammer/hammer_mount.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <err.h>
#include <mntopts.h>

static void test_master_id(int master_id);
static void extract_volumes(struct hammer_mount_info *info, char **av, int ac);
static void free_volumes(struct hammer_mount_info *info);
static void usage(void);

#define hwarnx(format, args...)	warnx("WARNING: "format,## args)

#define MOPT_HAMMEROPTS		\
	{ "history", 1, HMNT_NOHISTORY, 1 },	\
	{ "master=", 0, HMNT_MASTERID, 1 },	\
	{ "mirror", 1, HMNT_NOMIRROR, 1 }

static struct mntopt mopts[] = { MOPT_STDOPTS, MOPT_HAMMEROPTS,
				 MOPT_UPDATE, MOPT_NULL };

int
main(int ac, char **av)
{
	struct hammer_mount_info info;
	//struct vfsconf vfc;
	int mount_flags = 0;
	int init_flags = 0;
	//int error;
	int ch;
	//char *mountpt;
	char *ptr;

	bzero(&info, sizeof(info));

	while ((ch = getopt(ac, av, "o:T:u")) != -1) {
		switch(ch) {
		case 'T':
			info.asof = strtoull(optarg, NULL, 0);
			break;
		case 'o':
			getmntopts(optarg, mopts, &mount_flags, &info.hflags);

			/*
			 * Handle extended flags with parameters.
			 */
			if (info.hflags & HMNT_MASTERID) {
				ptr = strstr(optarg, "master=");
				if (ptr) {
					info.master_id = strtol(ptr + 7, NULL, 0);
					test_master_id(info.master_id);
				}
			}
			if (info.hflags & HMNT_NOMIRROR) {
				ptr = strstr(optarg, "nomirror");
				if (ptr)
					info.master_id = -1;
			}
			break;
		case 'u':
			init_flags |= MNT_UPDATE;
			break;
		default:
			usage();
			/* not reached */
			break;
		}
	}
	ac -= optind;
	av += optind;
	mount_flags |= init_flags;

	/*
	 * Only the mount point need be specified in update mode.
	 */
	if (init_flags & MNT_UPDATE) {
		if (ac != 1) {
			usage();
			/* not reached */
		}
#if 0
		mountpt = av[0];
		if (mount(vfc.vfc_name, mountpt, mount_flags, &info)) {
			err(1, "mountpoint %s", mountpt);
			/* not reached */
		}
#endif
		exit(0);
	}

	if (ac < 2) {
		usage();
		/* not reached */
	}

	/*
	 * Mount arguments: vol [vol...] mountpt
	 */
	extract_volumes(&info, av, ac - 1);
#if 0
	mountpt = av[ac - 1];
	/*
	 * Load the hammer module if necessary (this bit stolen from
	 * mount_null).
	 */
	error = getvfsbyname("hammer", &vfc);
	if (error && vfsisloadable("hammer")) {
		if (vfsload("hammer") != 0) {
			err(1, "vfsload(hammer)");
			/* not reached */
		}
		endvfsent();
		error = getvfsbyname("hammer", &vfc);
	}
	if (error) {
		errx(1, "hammer filesystem is not available");
		/* not reached */
	}

	if (mount(vfc.vfc_name, mountpt, mount_flags, &info)) {
		perror("mount");
		exit(1);
	}
#endif
	free_volumes(&info);

	return(0);
}

static
void
test_master_id(int master_id)
{
	switch (master_id) {
	case 0:
		hwarnx("A master id of 0 is the default, "
			"explicit settings should use 1-15");
		break;
	case -1:
		hwarnx("A master id of -1 is nomirror mode, "
			"equivalent to -o nomirror option");
		break;
	case 1 ... 15: /* gcc */
		/* Expected values via -o master= option */
		break;
	default:
		/* This will eventually fail in hammer_vfs_mount() */
		hwarnx("A master id of %d is not supported", master_id);
		break;
	}
}

/*
 * Extract a volume list
 */
static
void
extract_volumes(struct hammer_mount_info *info, char **av, int ac)
{
	int idx = 0;
	int arymax = 32;
	char **ary, *ptr, *next, *orig;

#define _extend_ary(ary, arymax) ({	\
		(arymax) += 32;		\
		realloc(ary, (arymax) * sizeof(char *)); \
	})
	ary = calloc(arymax, sizeof(char *));

	while (ac) {
		if (idx == arymax)
			ary = _extend_ary(ary, arymax);
		if (strchr(*av, ':') == NULL) {
			ary[idx++] = strdup(*av);
		} else {
			orig = next = strdup(*av);
			while ((ptr = next) != NULL) {
				if (idx == arymax)
					ary = _extend_ary(ary, arymax);
				if ((next = strchr(ptr, ':')) != NULL)
					*next++ = 0;
				ary[idx++] = strdup(ptr);
			}
			free(orig);
		}
		--ac;
		++av;
	}
	info->nvolumes = idx;
	info->volumes = ary;
}

static
void
free_volumes(struct hammer_mount_info *info)
{
	int i;

	for (i = 0; i < info->nvolumes; i++)
		free(info->volumes[i]);
	free(info->volumes);
}

static
void
usage(void)
{
	fprintf(stderr, "usage: mount_hammer [-o options] [-T transaction-id] "
			"special ... node\n");
	fprintf(stderr, "       mount_hammer [-o options] [-T transaction-id] "
			"special[:special]* node\n");
	fprintf(stderr, "       mount_hammer -u [-o options] node\n");
	exit(1);
}
