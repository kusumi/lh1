/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.org> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 * $FreeBSD: head/lib/libc/gen/sysctlbyname.c 244153 2012-12-12 15:27:33Z pjd $
 *
 */
#include <sys/types.h>
#include <errno.h>

// XXX This isn't usable unless fs exists, so just return -1.
int
sysctlbyname(const char *name, void *oldp, size_t *oldlenp,
    const void *newp, size_t newlen)
{
	errno = ENOTSUP;
	return(-1);
}
