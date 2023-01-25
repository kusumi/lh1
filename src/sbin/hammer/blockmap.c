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
 * $DragonFly: src/sbin/hammer/blockmap.c,v 1.2 2008/06/17 04:03:38 dillon Exp $
 */

#include "hammer_util.h"

/*
 * Allocate big-blocks using our poor-man's volume->vol_free_off.
 * We are bootstrapping the freemap itself and cannot update it yet.
 */
hammer_off_t
bootstrap_bigblock(volume_info_t volume)
{
	hammer_off_t result_offset;

	assert_volume_offset(volume);
	result_offset = volume->vol_free_off;

	volume->vol_free_off += HAMMER_BIGBLOCK_SIZE;

	return(result_offset);
}

/*
 * Allocate a big-block for zone-3 for UNDO/REDO FIFO.
 */
hammer_off_t
alloc_undo_bigblock(volume_info_t volume)
{
	hammer_blockmap_t freemap;
	buffer_info_t buffer1 = NULL;
	buffer_info_t buffer2 = NULL;
	hammer_blockmap_layer1_t layer1;
	hammer_blockmap_layer2_t layer2;
	hammer_off_t layer1_offset;
	hammer_off_t layer2_offset;
	hammer_off_t result_offset;

	/* Only root volume needs formatting */
	assert(volume->vol_no == HAMMER_ROOT_VOLNO);

	result_offset = bootstrap_bigblock(volume);
	freemap = &volume->ondisk->vol0_blockmap[HAMMER_ZONE_FREEMAP_INDEX];

	/*
	 * Dive layer 1.
	 */
	layer1_offset = freemap->phys_offset +
			HAMMER_BLOCKMAP_LAYER1_OFFSET(result_offset);
	layer1 = get_buffer_data(layer1_offset, &buffer1, 0);
	assert(layer1->phys_offset != HAMMER_BLOCKMAP_UNAVAIL);
	--layer1->blocks_free;
	hammer_crc_set_layer1(HammerVersion, layer1);
	buffer1->cache.modified = 1;

	/*
	 * Dive layer 2, each entry represents a big-block.
	 */
	layer2_offset = layer1->phys_offset +
			HAMMER_BLOCKMAP_LAYER2_OFFSET(result_offset);
	layer2 = get_buffer_data(layer2_offset, &buffer2, 0);
	assert(layer2->zone == 0);
	layer2->zone = HAMMER_ZONE_UNDO_INDEX;
	layer2->append_off = HAMMER_BIGBLOCK_SIZE;
	layer2->bytes_free = 0;
	hammer_crc_set_layer2(HammerVersion, layer2);
	buffer2->cache.modified = 1;

	--volume->ondisk->vol0_stat_freebigblocks;

	rel_buffer(buffer1);
	rel_buffer(buffer2);

	return(result_offset);
}

/*
 * Allocate a chunk of data out of a blockmap.  This is a simplified
 * version which uses next_offset as a simple allocation iterator.
 */
void *
alloc_blockmap(int zone, int bytes, hammer_off_t *result_offp,
	       buffer_info_t *bufferp)
{
	volume_info_t volume;
	hammer_blockmap_t blockmap;
	hammer_blockmap_t freemap;
	buffer_info_t buffer1 = NULL;
	buffer_info_t buffer2 = NULL;
	hammer_blockmap_layer1_t layer1;
	hammer_blockmap_layer2_t layer2;
	hammer_off_t tmp_offset;
	hammer_off_t layer1_offset;
	hammer_off_t layer2_offset;
	void *ptr;

	volume = get_root_volume();

	blockmap = &volume->ondisk->vol0_blockmap[zone];
	freemap = &volume->ondisk->vol0_blockmap[HAMMER_ZONE_FREEMAP_INDEX];
	assert(HAMMER_ZONE_DECODE(blockmap->next_offset) == zone);

	/*
	 * Alignment and buffer-boundary issues.  If the allocation would
	 * cross a buffer boundary we have to skip to the next buffer.
	 */
	bytes = HAMMER_DATA_DOALIGN(bytes);
	assert(bytes > 0 && bytes <= HAMMER_BUFSIZE);  /* not HAMMER_XBUFSIZE */
	assert(hammer_is_index_record(zone));

again:
	assert(blockmap->next_offset != HAMMER_ZONE_ENCODE(zone + 1, 0));

	tmp_offset = blockmap->next_offset + bytes - 1;
	if ((blockmap->next_offset ^ tmp_offset) & ~HAMMER_BUFMASK64)
		blockmap->next_offset = tmp_offset & ~HAMMER_BUFMASK64;

	/*
	 * Dive layer 1.
	 */
	layer1_offset = freemap->phys_offset +
			HAMMER_BLOCKMAP_LAYER1_OFFSET(blockmap->next_offset);
	layer1 = get_buffer_data(layer1_offset, &buffer1, 0);
	assert(layer1->phys_offset != HAMMER_BLOCKMAP_UNAVAIL);
	assert(!((blockmap->next_offset & HAMMER_BIGBLOCK_MASK) == 0 &&
		layer1->blocks_free == 0));

	/*
	 * Dive layer 2, each entry represents a big-block.
	 */
	layer2_offset = layer1->phys_offset +
			HAMMER_BLOCKMAP_LAYER2_OFFSET(blockmap->next_offset);
	layer2 = get_buffer_data(layer2_offset, &buffer2, 0);

	if (layer2->zone == HAMMER_ZONE_UNAVAIL_INDEX) {
		errx(1, "alloc_blockmap: layer2 ran out of space!");
		/* not reached */
	}

	/*
	 * If we are entering a new big-block assign ownership to our
	 * zone.  If the big-block is owned by another zone skip it.
	 */
	if (layer2->zone == 0) {
		--layer1->blocks_free;
		hammer_crc_set_layer1(HammerVersion, layer1);
		layer2->zone = zone;
		--volume->ondisk->vol0_stat_freebigblocks;
		assert(layer2->bytes_free == HAMMER_BIGBLOCK_SIZE);
		assert(layer2->append_off == 0);
	}
	if (layer2->zone != zone) {
		blockmap->next_offset =
			HAMMER_ZONE_LAYER2_NEXT_OFFSET(blockmap->next_offset);
		goto again;
	}

	assert(layer2->append_off ==
		(blockmap->next_offset & HAMMER_BIGBLOCK_MASK));
	layer2->bytes_free -= bytes;
	*result_offp = blockmap->next_offset;
	blockmap->next_offset += bytes;
	layer2->append_off = (int)blockmap->next_offset & HAMMER_BIGBLOCK_MASK;
	hammer_crc_set_layer2(HammerVersion, layer2);

	ptr = get_buffer_data(*result_offp, bufferp, 0);
	(*bufferp)->cache.modified = 1;

	buffer1->cache.modified = 1;
	buffer2->cache.modified = 1;

	rel_buffer(buffer1);
	rel_buffer(buffer2);
	return(ptr);
}

hammer_off_t
blockmap_lookup(hammer_off_t zone_offset, int *errorp)
{
	return(blockmap_lookup_save(zone_offset, NULL, NULL, errorp));
}

hammer_off_t
blockmap_lookup_save(hammer_off_t zone_offset,
		hammer_blockmap_layer1_t save_layer1,
		hammer_blockmap_layer2_t save_layer2,
		int *errorp)
{
	volume_info_t root_volume = NULL;
	hammer_volume_ondisk_t ondisk;
	hammer_blockmap_t blockmap;
	hammer_blockmap_t freemap;
	hammer_blockmap_layer1_t layer1;
	hammer_blockmap_layer2_t layer2;
	buffer_info_t buffer1 = NULL;
	buffer_info_t buffer2 = NULL;
	hammer_off_t layer1_offset;
	hammer_off_t layer2_offset;
	hammer_off_t result_offset = HAMMER_OFF_BAD;;
	int zone;
	int error = 0;

	if (save_layer1)
		bzero(save_layer1, sizeof(*save_layer1));
	if (save_layer2)
		bzero(save_layer2, sizeof(*save_layer2));

	zone = HAMMER_ZONE_DECODE(zone_offset);

	if (zone <= HAMMER_ZONE_RAW_VOLUME_INDEX) {
		error = -1;
		goto done;
	}
	if (zone >= HAMMER_MAX_ZONES) {
		error = -2;
		goto done;
	}

	root_volume = get_root_volume();
	ondisk = root_volume->ondisk;
	blockmap = &ondisk->vol0_blockmap[zone];

	/*
	 * Handle blockmap offset translations.
	 */
	if (hammer_is_index_record(zone)) {
		result_offset = hammer_xlate_to_zone2(zone_offset);
	} else if (zone == HAMMER_ZONE_UNDO_INDEX) {
		if (zone_offset >= blockmap->alloc_offset) {
			error = -3;
			goto done;
		}
		result_offset = hammer_xlate_to_undo(ondisk, zone_offset);
	} else {
		/* assert(zone == HAMMER_ZONE_RAW_BUFFER_INDEX); */
		result_offset = zone_offset;
	}

	/*
	 * The blockmap should match the requested zone (else the volume
	 * header is mashed).
	 */
	if (hammer_is_index_record(zone) &&
	    HAMMER_ZONE_DECODE(blockmap->alloc_offset) != zone) {
		error = -4;
		goto done;
	}

	/*
	 * Validate that the big-block is assigned to the zone.  Also
	 * assign save_layer{1,2} if not NULL.
	 */
	freemap = &ondisk->vol0_blockmap[HAMMER_ZONE_FREEMAP_INDEX];

	/*
	 * Dive layer 1.
	 */
	layer1_offset = freemap->phys_offset +
			HAMMER_BLOCKMAP_LAYER1_OFFSET(result_offset);
	layer1 = get_buffer_data(layer1_offset, &buffer1, 0);

	if (layer1 == NULL) {
		error = -5;
		goto done;
	}
	if (layer1->phys_offset == HAMMER_BLOCKMAP_UNAVAIL) {
		error = -6;
		goto done;
	}
	if (save_layer1)
		*save_layer1 = *layer1;

	/*
	 * Dive layer 2, each entry represents a big-block.
	 */
	layer2_offset = layer1->phys_offset +
			HAMMER_BLOCKMAP_LAYER2_OFFSET(result_offset);
	layer2 = get_buffer_data(layer2_offset, &buffer2, 0);

	if (layer2 == NULL) {
		error = -7;
		goto done;
	}
	if (layer2->zone != zone) {
		error = -8;
		goto done;
	}
	if (save_layer2)
		*save_layer2 = *layer2;

done:
	rel_buffer(buffer1);
	rel_buffer(buffer2);

	if (errorp)
		*errorp = error;

	return(result_offset);
}

