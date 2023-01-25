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

#include "hammer_util.h"

static void check_volume(volume_info_t volume);
static void get_buffer_readahead(buffer_info_t base);
static __inline int readhammervol(volume_info_t volume);
static __inline int readhammerbuf(buffer_info_t buffer);
static __inline int writehammervol(volume_info_t volume);
static __inline int writehammerbuf(buffer_info_t buffer);

hammer_uuid_t Hammer_FSType;
hammer_uuid_t Hammer_FSId;
int UseReadBehind = -4;
int UseReadAhead = 4;
int DebugOpt;
uint32_t HammerVersion = -1;
const char *LH1VersionString = LH1_VERSION_STRING;

TAILQ_HEAD(volume_list, volume_info);
static struct volume_list VolList = TAILQ_HEAD_INITIALIZER(VolList);
static int valid_hammer_volumes;

static __inline
int
buffer_hash(hammer_off_t zone2_offset)
{
	int hi;

	hi = (int)(zone2_offset / HAMMER_BUFSIZE) & HAMMER_BUFLISTMASK;
	return(hi);
}

static
buffer_info_t
find_buffer(hammer_off_t zone2_offset)
{
	volume_info_t volume;
	buffer_info_t buffer;
	int hi;

	volume = get_volume(HAMMER_VOL_DECODE(zone2_offset));
	assert(volume);

	hi = buffer_hash(zone2_offset);
	TAILQ_FOREACH(buffer, &volume->buffer_lists[hi], entry) {
		if (buffer->zone2_offset == zone2_offset)
			return(buffer);
	}
	return(NULL);
}

static
volume_info_t
__alloc_volume(const char *volname, int oflags)
{
	volume_info_t volume;
	int i;

	volume = calloc(1, sizeof(*volume));
	volume->vol_no = -1;
	volume->rdonly = (oflags == O_RDONLY);
	volume->name = strdup(volname);
	volume->fd = open(volume->name, oflags);
	if (volume->fd < 0) {
		err(1, "alloc_volume: Failed to open %s", volume->name);
		/* not reached */
	}
	check_volume(volume);

	volume->ondisk = calloc(1, HAMMER_BUFSIZE);

	for (i = 0; i < HAMMER_BUFLISTS; ++i)
		TAILQ_INIT(&volume->buffer_lists[i]);

	return(volume);
}

static
void
__add_volume(const volume_info_t volume)
{
	volume_info_t scan;
	struct stat st1, st2;

	if (fstat(volume->fd, &st1) != 0) {
		errx(1, "add_volume: %s: Failed to stat", volume->name);
		/* not reached */
	}

	TAILQ_FOREACH(scan, &VolList, entry) {
		if (scan->vol_no == volume->vol_no) {
			errx(1, "add_volume: %s: Duplicate volume number %d "
				"against %s",
				volume->name, volume->vol_no, scan->name);
			/* not reached */
		}
		if (fstat(scan->fd, &st2) != 0) {
			errx(1, "add_volume: %s: Failed to stat %s",
				volume->name, scan->name);
			/* not reached */
		}
		if ((st1.st_ino == st2.st_ino) && (st1.st_dev == st2.st_dev)) {
			errx(1, "add_volume: %s: Specified more than once",
				volume->name);
			/* not reached */
		}
	}

	TAILQ_INSERT_TAIL(&VolList, volume, entry);
}

static
void
__verify_volume(const volume_info_t volume)
{
	hammer_volume_ondisk_t ondisk = volume->ondisk;
	char *fstype;

	if (ondisk->vol_signature != HAMMER_FSBUF_VOLUME) {
		errx(1, "verify_volume: Invalid volume signature %016jx",
			ondisk->vol_signature);
		/* not reached */
	}
	if (ondisk->vol_rootvol != HAMMER_ROOT_VOLNO) {
		errx(1, "verify_volume: Invalid root volume# %d",
			ondisk->vol_rootvol);
		/* not reached */
	}
	hammer_uuid_to_string(&ondisk->vol_fstype, &fstype);
	if (hammer_uuid_compare(&Hammer_FSType, &ondisk->vol_fstype)) {
		errx(1, "verify_volume: %s: fstype %s does not indicate "
			"this is a HAMMER volume", volume->name, fstype);
		/* not reached */
	}
	free(fstype);
	if (hammer_uuid_compare(&Hammer_FSId, &ondisk->vol_fsid)) {
		errx(1, "verify_volume: %s: fsid does not match other volumes!",
			volume->name);
		/* not reached */
	}
	if (ondisk->vol_version < HAMMER_VOL_VERSION_MIN ||
	    ondisk->vol_version >= HAMMER_VOL_VERSION_WIP) {
		errx(1, "verify_volume: %s: Invalid volume version %u",
			volume->name, ondisk->vol_version);
		/* not reached */
	}
}

/*
 * Initialize a volume structure and ondisk vol_no field.
 */
volume_info_t
init_volume(const char *filename, int oflags, int32_t vol_no)
{
	volume_info_t volume;

	volume = __alloc_volume(filename, oflags);
	volume->vol_no = volume->ondisk->vol_no = vol_no;

	__add_volume(volume);

	return(volume);
}

/*
 * Initialize a volume structure and read ondisk volume header.
 */
volume_info_t
load_volume(const char *filename, int oflags, int verify_volume)
{
	volume_info_t volume;
	int n;

	volume = __alloc_volume(filename, oflags);

	n = readhammervol(volume);
	if (n == -1) {
		err(1, "load_volume: %s: Read failed at offset 0",
		    volume->name);
		/* not reached */
	}
	volume->vol_no = volume->ondisk->vol_no;
	if (volume->vol_no == HAMMER_ROOT_VOLNO)
		HammerVersion = volume->ondisk->vol_version;

	if (valid_hammer_volumes++ == 0)
		Hammer_FSId = volume->ondisk->vol_fsid;
	if (verify_volume)
		__verify_volume(volume);

	__add_volume(volume);

	return(volume);
}

/*
 * Check basic volume characteristics.
 */
static
void
check_volume(volume_info_t volume)
{
	struct stat st;
	off_t size;

	/*
	 * Allow the formatting of block devices or regular files
	 */
	if (ioctl(volume->fd, BLKGETSIZE, &size) < 0) {
		if (fstat(volume->fd, &st) < 0) {
			err(1, "Unable to stat %s", volume->name);
			/* not reached */
		}
		if (S_ISREG(st.st_mode)) {
			volume->size = st.st_size;
			volume->type = "REGFILE";
		} else {
			errx(1, "Unsupported file type for %s", volume->name);
			/* not reached */
		}
	} else {
		int sector_size;
		if (ioctl(volume->fd, BLKSSZGET, &sector_size) < 0) {
			errx(1, "Failed to get media sector size");
			/* not reached */
		}
		if (sector_size > HAMMER_BUFSIZE ||
		    HAMMER_BUFSIZE % sector_size) {
			errx(1, "A media sector size of %d is not supported",
			     sector_size);
			/* not reached */
		}

		volume->size = size << 9;
		volume->device_offset = 0; // XXX
		volume->type = "DEVICE";
	}
}

int
is_regfile(const volume_info_t volume)
{
	return(strcmp(volume->type, "REGFILE") ? 0 : 1);
}

void
assert_volume_offset(const volume_info_t volume)
{
	assert(hammer_is_zone_raw_buffer(volume->vol_free_off));
	assert(hammer_is_zone_raw_buffer(volume->vol_free_end));
	if (volume->vol_free_off >= volume->vol_free_end) {
		errx(1, "Ran out of room, filesystem too small");
		/* not reached */
	}
}

volume_info_t
get_volume(int32_t vol_no)
{
	volume_info_t volume;

	TAILQ_FOREACH(volume, &VolList, entry) {
		if (volume->vol_no == vol_no)
			break;
	}

	return(volume);
}

volume_info_t
get_root_volume(void)
{
	return(get_volume(HAMMER_ROOT_VOLNO));
}

static
hammer_off_t
__blockmap_xlate_to_zone2(hammer_off_t buf_offset)
{
	hammer_off_t zone2_offset;
	int error = 0;

	if (hammer_is_zone_raw_buffer(buf_offset))
		zone2_offset = buf_offset;
	else
		zone2_offset = blockmap_lookup(buf_offset, &error);

	if (error)
		return(HAMMER_OFF_BAD);
	assert(hammer_is_zone_raw_buffer(zone2_offset));

	return(zone2_offset);
}

static
buffer_info_t
__alloc_buffer(hammer_off_t zone2_offset, int isnew)
{
	volume_info_t volume;
	buffer_info_t buffer;
	int hi;

	volume = get_volume(HAMMER_VOL_DECODE(zone2_offset));
	assert(volume != NULL);

	buffer = calloc(1, sizeof(*buffer));
	buffer->zone2_offset = zone2_offset;
	buffer->raw_offset = hammer_xlate_to_phys(volume->ondisk, zone2_offset);
	buffer->volume = volume;
	buffer->ondisk = calloc(1, HAMMER_BUFSIZE);

	if (isnew <= 0) {
		if (readhammerbuf(buffer) == -1) {
			err(1, "Failed to read %s:%016jx at %016jx",
			    volume->name,
			    (intmax_t)buffer->zone2_offset,
			    (intmax_t)buffer->raw_offset);
			/* not reached */
		}
	}

	hi = buffer_hash(zone2_offset);
	TAILQ_INSERT_TAIL(&volume->buffer_lists[hi], buffer, entry);
	hammer_cache_add(&buffer->cache);

	return(buffer);
}

/*
 * Acquire the 16KB buffer for specified zone offset.
 */
static
buffer_info_t
get_buffer(hammer_off_t buf_offset, int isnew)
{
	buffer_info_t buffer;
	hammer_off_t zone2_offset;
	int dora = 0;

	zone2_offset = __blockmap_xlate_to_zone2(buf_offset);
	if (zone2_offset == HAMMER_OFF_BAD)
		return(NULL);

	zone2_offset &= ~HAMMER_BUFMASK64;
	buffer = find_buffer(zone2_offset);

	if (buffer == NULL) {
		buffer = __alloc_buffer(zone2_offset, isnew);
		dora = (isnew == 0);
	} else {
		assert(isnew != -1);
		hammer_cache_used(&buffer->cache);
	}
	assert(buffer->ondisk != NULL);

	++buffer->cache.refs;
	hammer_cache_flush();

	if (isnew > 0) {
		assert(buffer->cache.modified == 0);
		bzero(buffer->ondisk, HAMMER_BUFSIZE);
		buffer->cache.modified = 1;
	}
	if (dora)
		get_buffer_readahead(buffer);
	return(buffer);
}

static
void
get_buffer_readahead(const buffer_info_t base)
{
	buffer_info_t buffer;
	volume_info_t volume;
	hammer_off_t zone2_offset;
	int64_t raw_offset;
	int ri = UseReadBehind;
	int re = UseReadAhead;

	raw_offset = base->raw_offset + ri * HAMMER_BUFSIZE;
	volume = base->volume;

	while (ri < re) {
		if (raw_offset >= volume->ondisk->vol_buf_end)
			break;
		if (raw_offset < volume->ondisk->vol_buf_beg || ri == 0) {
			++ri;
			raw_offset += HAMMER_BUFSIZE;
			continue;
		}
		zone2_offset = HAMMER_ENCODE_RAW_BUFFER(volume->vol_no,
			raw_offset - volume->ondisk->vol_buf_beg);
		buffer = find_buffer(zone2_offset);
		if (buffer == NULL) {
			/* call with -1 to prevent another readahead */
			buffer = get_buffer(zone2_offset, -1);
			rel_buffer(buffer);
		}
		++ri;
		raw_offset += HAMMER_BUFSIZE;
	}
}

void
rel_buffer(buffer_info_t buffer)
{
	volume_info_t volume;
	int hi;

	if (buffer == NULL)
		return;
	assert(buffer->cache.refs > 0);
	if (--buffer->cache.refs == 0) {
		if (buffer->cache.delete) {
			hi = buffer_hash(buffer->zone2_offset);
			volume = buffer->volume;
			if (buffer->cache.modified)
				flush_buffer(buffer);
			TAILQ_REMOVE(&volume->buffer_lists[hi], buffer, entry);
			hammer_cache_del(&buffer->cache);
			free(buffer->ondisk);
			free(buffer);
		}
	}
}

/*
 * Retrieve a pointer to a buffer data given a zone-X buffer offset.
 * The underlying bufferp is freed if isnew or the corresponding zone-2
 * offset is out of range of the cached data.  If bufferp is freed,
 * a referenced buffer is loaded into it.
 */
void *
get_buffer_data(hammer_off_t buf_offset, buffer_info_t *bufferp, int isnew)
{
	hammer_off_t xor = 0;
	hammer_volume_ondisk_t ondisk;

	if (*bufferp != NULL) {
		if (hammer_is_zone_undo(buf_offset)) {
			ondisk = (*bufferp)->volume->ondisk;
			xor = hammer_xlate_to_undo(ondisk, buf_offset) ^
				(*bufferp)->zone2_offset;
		} else if (hammer_is_zone_direct_xlated(buf_offset)) {
			xor = HAMMER_OFF_LONG_ENCODE(buf_offset) ^
			      HAMMER_OFF_LONG_ENCODE((*bufferp)->zone2_offset);
		} else {
			assert(0);
		}
		if (isnew > 0 || (xor & ~HAMMER_BUFMASK64)) {
			rel_buffer(*bufferp);
			*bufferp = NULL;
		} else {
			hammer_cache_used(&(*bufferp)->cache);
		}
	}

	if (*bufferp == NULL) {
		*bufferp = get_buffer(buf_offset, isnew);
		if (*bufferp == NULL)
			return(NULL);
	}

	return((char *)(*bufferp)->ondisk +
		((int32_t)buf_offset & HAMMER_BUFMASK));
}

/*
 * Allocate HAMMER elements - B-Tree nodes
 */
hammer_node_ondisk_t
alloc_btree_node(hammer_off_t *offp, buffer_info_t *data_bufferp)
{
	hammer_node_ondisk_t node;

	node = alloc_blockmap(HAMMER_ZONE_BTREE_INDEX, sizeof(*node),
			      offp, data_bufferp);
	bzero(node, sizeof(*node));
	return(node);
}

/*
 * Allocate HAMMER elements - meta data (inode, direntry, PFS, etc)
 */
void *
alloc_meta_element(hammer_off_t *offp, int32_t data_len,
		   buffer_info_t *data_bufferp)
{
	void *data;

	data = alloc_blockmap(HAMMER_ZONE_META_INDEX, data_len,
			      offp, data_bufferp);
	bzero(data, data_len);
	return(data);
}

/*
 * Format a new blockmap.  This is mostly a degenerate case because
 * all allocations are now actually done from the freemap.
 */
void
format_blockmap(volume_info_t root_vol, int zone, hammer_off_t offset)
{
	hammer_blockmap_t blockmap;
	hammer_off_t zone_base;

	/* Only root volume needs formatting */
	assert(root_vol->vol_no == HAMMER_ROOT_VOLNO);

	assert(hammer_is_index_record(zone));

	blockmap = &root_vol->ondisk->vol0_blockmap[zone];
	zone_base = HAMMER_ZONE_ENCODE(zone, offset);

	bzero(blockmap, sizeof(*blockmap));
	blockmap->phys_offset = 0;
	blockmap->first_offset = zone_base;
	blockmap->next_offset = zone_base;
	blockmap->alloc_offset = HAMMER_ENCODE(zone, 255, -1);
	hammer_crc_set_blockmap(HammerVersion, blockmap);
}

/*
 * Format a new freemap.  Set all layer1 entries to UNAVAIL.  The initialize
 * code will load each volume's freemap.
 */
void
format_freemap(volume_info_t root_vol)
{
	buffer_info_t buffer = NULL;
	hammer_off_t layer1_offset;
	hammer_blockmap_t blockmap;
	hammer_blockmap_layer1_t layer1;
	int i, isnew;

	/* Only root volume needs formatting */
	assert(root_vol->vol_no == HAMMER_ROOT_VOLNO);

	layer1_offset = bootstrap_bigblock(root_vol);
	for (i = 0; i < HAMMER_BIGBLOCK_SIZE; i += sizeof(*layer1)) {
		isnew = ((i % HAMMER_BUFSIZE) == 0);
		layer1 = get_buffer_data(layer1_offset + i, &buffer, isnew);
		bzero(layer1, sizeof(*layer1));
		layer1->phys_offset = HAMMER_BLOCKMAP_UNAVAIL;
		layer1->blocks_free = 0;
		hammer_crc_set_layer1(HammerVersion, layer1);
	}
	assert(i == HAMMER_BIGBLOCK_SIZE);
	rel_buffer(buffer);

	blockmap = &root_vol->ondisk->vol0_blockmap[HAMMER_ZONE_FREEMAP_INDEX];
	bzero(blockmap, sizeof(*blockmap));
	blockmap->phys_offset = layer1_offset;
	blockmap->first_offset = 0;
	blockmap->next_offset = HAMMER_ENCODE_RAW_BUFFER(0, 0);
	blockmap->alloc_offset = HAMMER_ENCODE_RAW_BUFFER(255, -1);
	hammer_crc_set_blockmap(HammerVersion, blockmap);
}

/*
 * Load the volume's remaining free space into the freemap.
 *
 * Returns the number of big-blocks available.
 */
int64_t
initialize_freemap(volume_info_t volume)
{
	volume_info_t root_vol;
	buffer_info_t buffer1 = NULL;
	buffer_info_t buffer2 = NULL;
	hammer_blockmap_layer1_t layer1;
	hammer_blockmap_layer2_t layer2;
	hammer_off_t layer1_offset;
	hammer_off_t layer2_offset;
	hammer_off_t phys_offset;
	hammer_off_t block_offset;
	hammer_off_t aligned_vol_free_end;
	hammer_blockmap_t freemap;
	int64_t count = 0;
	int64_t layer1_count = 0;

	root_vol = get_root_volume();

	assert_volume_offset(volume);
	aligned_vol_free_end = HAMMER_BLOCKMAP_LAYER2_DOALIGN(volume->vol_free_end);

	printf("initialize freemap volume %d\n", volume->vol_no);

	/*
	 * Initialize the freemap.  First preallocate the big-blocks required
	 * to implement layer2.   This preallocation is a bootstrap allocation
	 * using blocks from the target volume.
	 */
	freemap = &root_vol->ondisk->vol0_blockmap[HAMMER_ZONE_FREEMAP_INDEX];

	for (phys_offset = HAMMER_ENCODE_RAW_BUFFER(volume->vol_no, 0);
	     phys_offset < aligned_vol_free_end;
	     phys_offset += HAMMER_BLOCKMAP_LAYER2) {
		layer1_offset = freemap->phys_offset +
				HAMMER_BLOCKMAP_LAYER1_OFFSET(phys_offset);
		layer1 = get_buffer_data(layer1_offset, &buffer1, 0);
		if (layer1->phys_offset == HAMMER_BLOCKMAP_UNAVAIL) {
			layer1->phys_offset = bootstrap_bigblock(volume);
			layer1->blocks_free = 0;
			buffer1->cache.modified = 1;
			hammer_crc_set_layer1(HammerVersion, layer1);
		}
	}

	/*
	 * Now fill everything in.
	 */
	for (phys_offset = HAMMER_ENCODE_RAW_BUFFER(volume->vol_no, 0);
	     phys_offset < aligned_vol_free_end;
	     phys_offset += HAMMER_BLOCKMAP_LAYER2) {
		layer1_count = 0;
		layer1_offset = freemap->phys_offset +
				HAMMER_BLOCKMAP_LAYER1_OFFSET(phys_offset);
		layer1 = get_buffer_data(layer1_offset, &buffer1, 0);
		assert(layer1->phys_offset != HAMMER_BLOCKMAP_UNAVAIL);

		for (block_offset = 0;
		     block_offset < HAMMER_BLOCKMAP_LAYER2;
		     block_offset += HAMMER_BIGBLOCK_SIZE) {
			layer2_offset = layer1->phys_offset +
				        HAMMER_BLOCKMAP_LAYER2_OFFSET(block_offset);
			layer2 = get_buffer_data(layer2_offset, &buffer2, 0);
			bzero(layer2, sizeof(*layer2));

			if (phys_offset + block_offset < volume->vol_free_off) {
				/*
				 * Big-blocks already allocated as part
				 * of the freemap bootstrap.
				 */
				layer2->zone = HAMMER_ZONE_FREEMAP_INDEX;
				layer2->append_off = HAMMER_BIGBLOCK_SIZE;
				layer2->bytes_free = 0;
			} else if (phys_offset + block_offset < volume->vol_free_end) {
				layer2->zone = 0;
				layer2->append_off = 0;
				layer2->bytes_free = HAMMER_BIGBLOCK_SIZE;
				++count;
				++layer1_count;
			} else {
				layer2->zone = HAMMER_ZONE_UNAVAIL_INDEX;
				layer2->append_off = HAMMER_BIGBLOCK_SIZE;
				layer2->bytes_free = 0;
			}
			hammer_crc_set_layer2(HammerVersion, layer2);
			buffer2->cache.modified = 1;
		}

		layer1->blocks_free += layer1_count;
		hammer_crc_set_layer1(HammerVersion, layer1);
		buffer1->cache.modified = 1;
	}

	rel_buffer(buffer1);
	rel_buffer(buffer2);
	return(count);
}

/*
 * Returns the number of big-blocks available for filesystem data and undos
 * without formatting.
 */
int64_t
count_freemap(const volume_info_t volume)
{
	hammer_off_t phys_offset;
	hammer_off_t vol_free_off;
	hammer_off_t aligned_vol_free_end;
	int64_t count = 0;

	vol_free_off = HAMMER_ENCODE_RAW_BUFFER(volume->vol_no, 0);

	assert_volume_offset(volume);
	aligned_vol_free_end = HAMMER_BLOCKMAP_LAYER2_DOALIGN(volume->vol_free_end);

	if (volume->vol_no == HAMMER_ROOT_VOLNO)
		vol_free_off += HAMMER_BIGBLOCK_SIZE;

	for (phys_offset = HAMMER_ENCODE_RAW_BUFFER(volume->vol_no, 0);
	     phys_offset < aligned_vol_free_end;
	     phys_offset += HAMMER_BLOCKMAP_LAYER2) {
		vol_free_off += HAMMER_BIGBLOCK_SIZE;
	}

	for (phys_offset = HAMMER_ENCODE_RAW_BUFFER(volume->vol_no, 0);
	     phys_offset < aligned_vol_free_end;
	     phys_offset += HAMMER_BIGBLOCK_SIZE) {
		if (phys_offset < vol_free_off)
			;
		else if (phys_offset < volume->vol_free_end)
			++count;
	}

	return(count);
}

/*
 * Format the undomap for the root volume.
 */
void
format_undomap(volume_info_t root_vol, int64_t *undo_buffer_size)
{
	hammer_off_t undo_limit;
	hammer_blockmap_t blockmap;
	hammer_volume_ondisk_t ondisk;
	buffer_info_t buffer = NULL;
	hammer_off_t scan;
	int n;
	int limit_index;
	uint32_t seqno;

	/* Only root volume needs formatting */
	assert(root_vol->vol_no == HAMMER_ROOT_VOLNO);
	ondisk = root_vol->ondisk;

	/*
	 * Size the undo buffer in multiples of HAMMER_BIGBLOCK_SIZE,
	 * up to HAMMER_MAX_UNDO_BIGBLOCKS big-blocks.
	 * Size to approximately 0.1% of the disk.
	 *
	 * The minimum UNDO fifo size is 512MB, or approximately 1% of
	 * the recommended 50G disk.
	 *
	 * Changing this minimum is rather dangerous as complex filesystem
	 * operations can cause the UNDO FIFO to fill up otherwise.
	 */
	undo_limit = *undo_buffer_size;
	if (undo_limit == 0) {
		undo_limit = HAMMER_VOL_BUF_SIZE(ondisk) / 1000;
		if (undo_limit < HAMMER_BIGBLOCK_SIZE * HAMMER_MIN_UNDO_BIGBLOCKS)
			undo_limit = HAMMER_BIGBLOCK_SIZE * HAMMER_MIN_UNDO_BIGBLOCKS;
	}
	undo_limit = HAMMER_BIGBLOCK_DOALIGN(undo_limit);
	if (undo_limit < HAMMER_BIGBLOCK_SIZE)
		undo_limit = HAMMER_BIGBLOCK_SIZE;
	if (undo_limit > HAMMER_BIGBLOCK_SIZE * HAMMER_MAX_UNDO_BIGBLOCKS)
		undo_limit = HAMMER_BIGBLOCK_SIZE * HAMMER_MAX_UNDO_BIGBLOCKS;
	*undo_buffer_size = undo_limit;

	blockmap = &ondisk->vol0_blockmap[HAMMER_ZONE_UNDO_INDEX];
	bzero(blockmap, sizeof(*blockmap));
	blockmap->phys_offset = HAMMER_BLOCKMAP_UNAVAIL;
	blockmap->first_offset = HAMMER_ENCODE_UNDO(0);
	blockmap->next_offset = blockmap->first_offset;
	blockmap->alloc_offset = HAMMER_ENCODE_UNDO(undo_limit);
	hammer_crc_set_blockmap(HammerVersion, blockmap);

	limit_index = undo_limit / HAMMER_BIGBLOCK_SIZE;
	assert(limit_index <= HAMMER_MAX_UNDO_BIGBLOCKS);

	for (n = 0; n < limit_index; ++n)
		ondisk->vol0_undo_array[n] = alloc_undo_bigblock(root_vol);
	while (n < HAMMER_MAX_UNDO_BIGBLOCKS)
		ondisk->vol0_undo_array[n++] = HAMMER_BLOCKMAP_UNAVAIL;

	/*
	 * Pre-initialize the UNDO blocks (HAMMER version 4+)
	 */
	printf("initializing the undo map (%jd MB)\n",
		(intmax_t)HAMMER_OFF_LONG_ENCODE(blockmap->alloc_offset) /
		(1024 * 1024));

	scan = blockmap->first_offset;
	seqno = 0;

	while (scan < blockmap->alloc_offset) {
		hammer_fifo_head_t head;
		hammer_fifo_tail_t tail;
		int bytes = HAMMER_UNDO_ALIGN;
		int isnew = ((scan & HAMMER_BUFMASK64) == 0);

		head = get_buffer_data(scan, &buffer, isnew);
		buffer->cache.modified = 1;
		tail = (void *)((char *)head + bytes - sizeof(*tail));

		bzero(head, bytes);
		head->hdr_signature = HAMMER_HEAD_SIGNATURE;
		head->hdr_type = HAMMER_HEAD_TYPE_DUMMY;
		head->hdr_size = bytes;
		head->hdr_seq = seqno++;

		tail->tail_signature = HAMMER_TAIL_SIGNATURE;
		tail->tail_type = HAMMER_HEAD_TYPE_DUMMY;
		tail->tail_size = bytes;

		hammer_crc_set_fifo_head(HammerVersion, head, bytes);

		scan += bytes;
	}
	rel_buffer(buffer);
}

const char *zone_labels[] = {
	"",		/* 0 */
	"raw_volume",	/* 1 */
	"raw_buffer",	/* 2 */
	"undo",		/* 3 */
	"freemap",	/* 4 */
	"",		/* 5 */
	"",		/* 6 */
	"",		/* 7 */
	"btree",	/* 8 */
	"meta",		/* 9 */
	"large_data",	/* 10 */
	"small_data",	/* 11 */
	"",		/* 12 */
	"",		/* 13 */
	"",		/* 14 */
	"unavail",	/* 15 */
};

void
print_blockmap(const volume_info_t volume)
{
	hammer_blockmap_t blockmap;
	hammer_volume_ondisk_t ondisk = volume->ondisk;
	int64_t size, used;
	int i;
	char *fstype, *fsid;
#define INDENT ""

	printf(INDENT"vol_label\t%s\n", ondisk->vol_label);
	printf(INDENT"vol_count\t%d\n", ondisk->vol_count);

	hammer_uuid_to_string(&ondisk->vol_fstype, &fstype);
	hammer_uuid_to_string(&ondisk->vol_fsid, &fsid);
	printf(INDENT"vol_fstype\t%s", fstype);
	if (strcmp(fstype, "61dc63ac-6e38-11dc-8513-01301bb8a9f5") == 0)
		printf(" \"%s\"\n", HAMMER_FSTYPE_STRING);
	else
		printf("\n"); /* invalid UUID */
	printf(INDENT"vol_fsid\t%s\n", fsid);
	free(fstype);
	free(fsid);

	printf(INDENT"vol_bot_beg\t%s\n", sizetostr(ondisk->vol_bot_beg));
	printf(INDENT"vol_mem_beg\t%s\n", sizetostr(ondisk->vol_mem_beg));
	printf(INDENT"vol_buf_beg\t%s\n", sizetostr(ondisk->vol_buf_beg));
	printf(INDENT"vol_buf_end\t%s\n", sizetostr(ondisk->vol_buf_end));
	printf(INDENT"vol0_next_tid\t%016jx\n",
	       (uintmax_t)ondisk->vol0_next_tid);

	blockmap = &ondisk->vol0_blockmap[HAMMER_ZONE_UNDO_INDEX];
	size = HAMMER_OFF_LONG_ENCODE(blockmap->alloc_offset);
	if (blockmap->first_offset <= blockmap->next_offset)
		used = blockmap->next_offset - blockmap->first_offset;
	else
		used = blockmap->alloc_offset - blockmap->first_offset +
			HAMMER_OFF_LONG_ENCODE(blockmap->next_offset);
	printf(INDENT"undo_size\t%s\n", sizetostr(size));
	printf(INDENT"undo_used\t%s\n", sizetostr(used));

	printf(INDENT"zone #             "
	       "phys             first            next             alloc\n");
	for (i = 0; i < HAMMER_MAX_ZONES; i++) {
		blockmap = &ondisk->vol0_blockmap[i];
		printf(INDENT"zone %-2d %-10s %016jx %016jx %016jx %016jx\n",
			i, zone_labels[i],
			(uintmax_t)blockmap->phys_offset,
			(uintmax_t)blockmap->first_offset,
			(uintmax_t)blockmap->next_offset,
			(uintmax_t)blockmap->alloc_offset);
	}
}

/*
 * Flush various tracking structures to disk
 */
void
flush_all_volumes(void)
{
	volume_info_t volume;

	TAILQ_FOREACH(volume, &VolList, entry)
		flush_volume(volume);
}

void
flush_volume(volume_info_t volume)
{
	buffer_info_t buffer;
	int i;

	for (i = 0; i < HAMMER_BUFLISTS; ++i) {
		TAILQ_FOREACH(buffer, &volume->buffer_lists[i], entry)
			flush_buffer(buffer);
	}
	if (writehammervol(volume) == -1) {
		err(1, "Write volume %d (%s)", volume->vol_no, volume->name);
		/* not reached */
	}
}

void
flush_buffer(buffer_info_t buffer)
{
	volume_info_t volume;

	volume = buffer->volume;
	if (writehammerbuf(buffer) == -1) {
		err(1, "Write volume %d (%s)", volume->vol_no, volume->name);
		/* not reached */
	}
	buffer->cache.modified = 0;
}

/*
 * Core I/O operations
 */
static
int
__read(volume_info_t volume, void *data, int64_t offset, int size)
{
	ssize_t n;

	n = pread(volume->fd, data, size, offset);
	if (n != size)
		return(-1);
	return(0);
}

static __inline
int
readhammervol(volume_info_t volume)
{
	return(__read(volume, volume->ondisk, 0, HAMMER_BUFSIZE));
}

static __inline
int
readhammerbuf(buffer_info_t buffer)
{
	return(__read(buffer->volume, buffer->ondisk, buffer->raw_offset,
		HAMMER_BUFSIZE));
}

static
int
__write(volume_info_t volume, const void *data, int64_t offset, int size)
{
	ssize_t n;

	if (volume->rdonly)
		return(0);

	n = pwrite(volume->fd, data, size, offset);
	if (n != size)
		return(-1);
	return(0);
}

static __inline
int
writehammervol(volume_info_t volume)
{
	return(__write(volume, volume->ondisk, 0, HAMMER_BUFSIZE));
}

static __inline
int
writehammerbuf(buffer_info_t buffer)
{
	return(__write(buffer->volume, buffer->ondisk, buffer->raw_offset,
		HAMMER_BUFSIZE));
}

int64_t
init_boot_area_size(int64_t value, off_t avg_vol_size)
{
	if (value == 0) {
		value = HAMMER_BOOT_NOMBYTES;
		while (value > avg_vol_size / HAMMER_MAX_VOLUMES)
			value >>= 1;
	}

	if (value < HAMMER_BOOT_MINBYTES)
		value = HAMMER_BOOT_MINBYTES;
	else if (value > HAMMER_BOOT_MAXBYTES)
		value = HAMMER_BOOT_MAXBYTES;

	return(value);
}

int64_t
init_memory_log_size(int64_t value, off_t avg_vol_size)
{
	if (value == 0) {
		value = HAMMER_MEM_NOMBYTES;
		while (value > avg_vol_size / HAMMER_MAX_VOLUMES)
			value >>= 1;
	}

	if (value < HAMMER_MEM_MINBYTES)
		value = HAMMER_MEM_MINBYTES;
	else if (value > HAMMER_MEM_MAXBYTES)
		value = HAMMER_MEM_MAXBYTES;

	return(value);
}
