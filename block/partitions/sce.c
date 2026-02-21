// SPDX-License-Identifier: GPL-2.0
/*
 *  block/partitions/sce.c
 *  PlayStation Vita SCE partition table support.
 *
 *  The Vita eMMC uses a proprietary partition table with the magic string
 *  "Sony Computer Entertainment Inc." at offset 0 and a standard 0x55AA
 *  boot signature. Up to 16 partition entries are stored at offset 0x50,
 *  each 17 bytes packed.
 *
 *  Reference: https://wiki.henkaku.xyz/vita/Partitions
 */

#include "check.h"
#include <linux/compiler.h>

#define SCE_MBR_MAGIC		"Sony Computer Entertainment Inc."
#define SCE_MBR_MAGIC_LEN	32
#define SCE_MBR_SIGNATURE	0xAA55
#define SCE_PART_OFFSET		0x50
#define SCE_MAX_PARTITIONS	16

struct sce_partition_entry {
	__le32 offset;		/* in 512-byte blocks */
	__le32 size;		/* in 512-byte blocks */
	u8 code;		/* partition code (e.g. 0x03=os0, 0x07=ur0) */
	u8 type;		/* filesystem type (0x06=fat16, 0x07=exfat, 0xDA=raw) */
	u8 active;
	u8 flags;
	u8 reserved[5];
} __packed;

struct sce_mbr {
	u8 magic[SCE_MBR_MAGIC_LEN];	/* 0x00 */
	__le32 version;			/* 0x20 */
	__le32 size_in_blocks;		/* 0x24 */
	u8 unknown[8];			/* 0x28 */
	__le32 header_fields[8];	/* 0x30 */
	struct sce_partition_entry partitions[SCE_MAX_PARTITIONS]; /* 0x50 */
} __packed;

int sce_partition(struct parsed_partitions *state)
{
	Sector sect;
	const struct sce_mbr *mbr;
	const struct sce_partition_entry *p;
	int slot = 1;
	int i;

	mbr = read_part_sector(state, 0, &sect);
	if (!mbr)
		return -1;

	if (memcmp(mbr->magic, SCE_MBR_MAGIC, SCE_MBR_MAGIC_LEN) != 0) {
		put_dev_sector(sect);
		return 0;
	}

	strlcat(state->pp_buf, " [SCE]", PAGE_SIZE);

	p = mbr->partitions;
	for (i = 0; i < SCE_MAX_PARTITIONS; i++, p++) {
		u32 offset = le32_to_cpu(p->offset);
		u32 size = le32_to_cpu(p->size);

		if (slot == state->limit)
			break;
		if (!size || !p->code)
			continue;

		put_partition(state, slot, offset, size);
		slot++;
	}

	strlcat(state->pp_buf, "\n", PAGE_SIZE);
	put_dev_sector(sect);
	return 1;
}
