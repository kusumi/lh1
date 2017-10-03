/*
 * Copyright (c) 2007-2016 The DragonFly Project.  All rights reserved.
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

#include "hammer.h"

static int hammer_test_offset(const char *msg, hammer_off_t offset);
static void hammer_strip_bigblock(int zone, hammer_off_t offset);
static void hammer_ask_yn(void);

void
hammer_cmd_strip(void)
{
	volume_info_t volume;
	hammer_blockmap_t rootmap;
	hammer_blockmap_layer1_t layer1;
	hammer_blockmap_layer2_t layer2;
	buffer_info_t buffer1 = NULL;
	buffer_info_t buffer2 = NULL;
	hammer_off_t layer1_offset;
	hammer_off_t layer2_offset;
	hammer_off_t phys_offset;
	hammer_off_t block_offset;
	hammer_off_t offset;
	int i, zone = HAMMER_ZONE_FREEMAP_INDEX;

	hammer_ask_yn();

	volume = get_root_volume();
	if (volume == NULL) {
		printf("No root volume found\n");
		goto strip_header;
	}

	rootmap = &volume->ondisk->vol0_blockmap[zone];
	if (!hammer_test_offset("layer1 physical", rootmap->phys_offset))
		goto strip_header;

	for (phys_offset = HAMMER_ZONE_ENCODE(zone, 0);
	     phys_offset < HAMMER_ZONE_ENCODE(zone, HAMMER_OFF_LONG_MASK);
	     phys_offset += HAMMER_BLOCKMAP_LAYER2) {
		/*
		 * Dive layer 1.
		 */
		layer1_offset = rootmap->phys_offset +
				HAMMER_BLOCKMAP_LAYER1_OFFSET(phys_offset);
		if (!hammer_test_offset("layer1", layer1_offset))
			break;
		layer1 = get_buffer_data(layer1_offset, &buffer1, 0);

		if (layer1->phys_offset == HAMMER_BLOCKMAP_UNAVAIL)
			continue;

		for (block_offset = 0;
		     block_offset < HAMMER_BLOCKMAP_LAYER2;
		     block_offset += HAMMER_BIGBLOCK_SIZE) {
			offset = phys_offset + block_offset;
			/*
			 * Dive layer 2, each entry represents a big-block.
			 */
			layer2_offset = layer1->phys_offset +
					HAMMER_BLOCKMAP_LAYER2_OFFSET(block_offset);
			if (!hammer_test_offset("layer2", layer2_offset))
				break;
			layer2 = get_buffer_data(layer2_offset, &buffer2, 0);

			if (layer2->zone == HAMMER_ZONE_BTREE_INDEX ||
			    layer2->zone == HAMMER_ZONE_META_INDEX) {
				hammer_strip_bigblock(layer2->zone, offset);
				layer2->zone = HAMMER_ZONE_UNAVAIL_INDEX;
				layer2->append_off = HAMMER_BIGBLOCK_SIZE;
				layer2->bytes_free = 0;
				hammer_crc_set_layer2(HammerVersion, layer2);
				buffer2->cache.modified = 1;
			} else if (layer2->zone == HAMMER_ZONE_UNAVAIL_INDEX) {
				break;
			}
		}
	}
	rel_buffer(buffer1);
	rel_buffer(buffer2);

strip_header:
	for (i = 0; i < HAMMER_MAX_VOLUMES; i++) {
		volume = get_volume(i);
		if (volume) {
			bzero(volume->ondisk, sizeof(*volume->ondisk));
			memcpy(&volume->ondisk->vol_signature, "STRIPPED", 8);
			printf("Stripped volume header %s\n", volume->name);
		}
	}

	flush_all_volumes();
}

static
int
hammer_test_offset(const char *msg, hammer_off_t offset)
{
	if (get_volume(HAMMER_VOL_DECODE(offset)) == NULL) {
		printf("Invalid volume# %d\n", HAMMER_VOL_DECODE(offset));
		return(0);
	}
	if (!hammer_is_zone_raw_buffer(offset)) {
		printf("Invalid %s offset 0x%jx\n", msg, offset);
		return(0);
	}

	return(1);
}

static
void
hammer_strip_bigblock(int zone, hammer_off_t offset)
{
	buffer_info_t buffer = NULL;
	int i;

	assert(hammer_is_index_record(zone));
	assert((offset & HAMMER_BIGBLOCK_MASK64) == 0);
	assert((offset & HAMMER_BUFMASK) == 0);
	offset = hammer_xlate_to_zoneX(zone, offset);

	/*
	 * This format is taken from hammer blockmap.
	 */
	printf("Stripped big-block ");
	if (VerboseOpt) {
		printf("%016jx zone=%-2d vol=%-3d L1#=%-6d L2#=%-6d L1=%-7lu L2=%-7lu\n",
			offset,
			zone,
			HAMMER_VOL_DECODE(offset),
			HAMMER_BLOCKMAP_LAYER1_INDEX(offset),
			HAMMER_BLOCKMAP_LAYER2_INDEX(offset),
			HAMMER_BLOCKMAP_LAYER1_OFFSET(offset),
			HAMMER_BLOCKMAP_LAYER2_OFFSET(offset));
	} else {
		printf("%016jx\n", offset);
	}

	for (i = 0; i < HAMMER_BIGBLOCK_SIZE; i += HAMMER_BUFSIZE) {
		get_buffer_data(offset + i, &buffer, 1);
		assert(buffer);
	}
}

static
void
hammer_ask_yn(void)
{
	volume_info_t volume;
#define _HAMMER "HAMMER filesystem"
	char type[64];
	char label[64 + 4];
	int i;

	volume = get_root_volume();

	if (volume && volume->ondisk->vol_signature == HAMMER_FSBUF_VOLUME)
		strcpy(type, _HAMMER);
	else
		strcpy(type, "devices");

	if (volume && volume->ondisk->vol_label[0]) {
		snprintf(label, sizeof(label), " (%s)",
			volume->ondisk->vol_label);
	} else {
		strcpy(label, "");
	}

	/*
	 * This format is taken from hammer pfs-destroy.
	 */
	printf("You have requested that %s%s be stripped", type, label);
	if (strcmp(type, _HAMMER))
		printf(", but %s may not be %s volumes\n", type, _HAMMER);
	else
		printf("\n");

	printf("Do you really want to do this? [y/n] ");
	fflush(stdout);
	if (getyn() == 0) {
		errx(1, "No action taken");
		/* not reached */
	}

	if (strcmp(type, _HAMMER)) {
		printf("Are you absolutely sure you want to do this? [y/n] ");
		fflush(stdout);
		if (getyn() == 0) {
			errx(1, "No action taken");
			/* not reached */
		}
	}

	printf("Stripping %s%s", type, label);

	if (DebugOpt) {
		printf("\n");
	} else {
		printf(" in");
		for (i = 5; i; --i) {
			printf(" %d", i);
			fflush(stdout);
			sleep(1);
		}
		printf(".. starting destruction pass\n");
	}
}
