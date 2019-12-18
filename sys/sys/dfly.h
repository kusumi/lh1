/*
 * Copyright (c) 2018 Tomohiro Kusumi.  All rights reserved.
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

#ifndef _SYS_DFLY_H_
#define _SYS_DFLY_H_

#include <sys/cdefs.h>

#define LH1_VERSION_STRING	"v0.1.11"

#if __GNUC_PREREQ(2, 7)
/*
 * Note that some Linux headers have a struct field named __unused,
 * which conflicts with __unused directive in DragonFly.
 * e.g. <linux/sysctl.h> included by <sys/sysctl.h>.
 * e.g. <bits/stat.h> included by <sys/stat.h>.
 *
 * These macros used in lh1 won't be renamed to minimize diff from DragonFly.
 * <sys/dfly.h> should be included after such headers that cause conflicts.
 */
#define __unused	__attribute__((unused))
#define __dead2		__attribute__((__noreturn__))
#define __packed	__attribute__((__packed__))
#else
#define __unused
#define __dead2
#define __packed
#endif

#if !__GNUC_PREREQ(2, 7)
#define	__printflike(fmtarg, firstvararg)
#elif __GNUC_PREREQ(3, 0)
#define	__printflike(fmtarg, firstvararg) \
            __attribute__((__nonnull__(fmtarg), \
			  __format__ (__printf__, fmtarg, firstvararg)))
#else
#define	__printflike(fmtarg, firstvararg) \
	    __attribute__((__format__ (__printf__, fmtarg, firstvararg)))
#endif

#endif /* !_SYS_DFLY_H_ */
