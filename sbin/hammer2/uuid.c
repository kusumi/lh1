/*
 * Copyright (c) 2017 The DragonFly Project.  All rights reserved.
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <byteswap.h>
#include <uuid/uuid.h>

#include "hammer2_subs.h"

/*
 * See also:
 *      http://www.opengroup.org/dce/info/draft-leach-uuids-guids-01.txt
 *      http://www.opengroup.org/onlinepubs/009629399/apdxa.htm
 *
 * A DCE 1.1 compatible source representation of UUIDs.
 */
struct __uuid {
	uint32_t	time_low;
	uint16_t	time_mid;
	uint16_t	time_hi_and_version;
	uint8_t		clock_seq_hi_and_reserved;
	uint8_t		clock_seq_low;
	uint8_t		node[6];
};

static
void __uuid_swap(struct __uuid *p)
{
	p->time_low = bswap_32(p->time_low);
	p->time_mid = bswap_16(p->time_mid);
	p->time_hi_and_version = bswap_16(p->time_hi_and_version);
}

void hammer2_uuid_create(hammer2_uuid_t *uuid)
{
	uuid_generate(uuid->uuid);
}

int hammer2_uuid_from_string(const char *str, hammer2_uuid_t *uuid)
{
	if (uuid_parse(str, uuid->uuid))
		return(-1);

	__uuid_swap((struct __uuid*)&uuid->uuid); /* big to little */

	return(0);
}

int hammer2_uuid_to_string(const hammer2_uuid_t *uuid, char **str)
{
	struct __uuid u;

	memcpy(&u, uuid->uuid, sizeof(u));
	__uuid_swap(&u); /* little to big */

	*str = calloc(64, sizeof(char*));
	uuid_unparse((void*)&u, *str);

	return(0);
}

int hammer2_uuid_name_lookup(hammer2_uuid_t *uuid, const char *str)
{
	if (strcmp(str, "DragonFly HAMMER2") ||
	    hammer2_uuid_from_string(HAMMER2_UUID_STRING, uuid)) {
		memset(uuid, 0, sizeof(*uuid));
		return(-1);
	}

	return(0);
}

int hammer2_uuid_addr_lookup(const hammer2_uuid_t *uuid, char **str)
{
	hammer2_uuid_t tmp;

	if (hammer2_uuid_from_string(HAMMER2_UUID_STRING, &tmp) ||
	    memcmp(uuid, &tmp, sizeof(tmp))) {
		*str = NULL;
		return(-1);
	}
	*str = strdup("DragonFly HAMMER2");

	return(0);
}

int hammer2_uuid_compare(const hammer2_uuid_t *uuid1, const hammer2_uuid_t *uuid2)
{
	return(uuid_compare(uuid1->uuid, uuid2->uuid));
}
