#define BA_PAGE(block) ((block) >> 3)
#define BA_BIT(block)  (1 << ((block) & 0x07))
#define BLOCKS_PER_PAGE 8

#define SIZE_TO_BLOCKS(size, blockSize) (((size) + ((blockSize) - 1)) / (blockSize))

#define BA_BLOCK_USED(table, block) (table[BA_PAGE(block)] & BA_BIT(block))

#define BA_BLOCK_MARK_FREE(table, block) table[BA_PAGE(block)] &= ~BA_BIT(block);
#define BA_BLOCK_MARK_USED(table, block) table[BA_PAGE(block)] |=  BA_BIT(block);

static CC_INLINE int blockalloc_can_alloc(cc_uint8* table, int beg, int blocks) {
	int i;
	for (i = beg; i < beg + blocks; i++)
	{
		if (BA_BLOCK_USED(table, i)) return false;
	}
	return true;
}

static int blockalloc_alloc(cc_uint8* table, int maxBlocks, int blocks) {
	int i, j;
	if (blocks > maxBlocks) return -1;

	for (i = 0; i <= maxBlocks - blocks;) 
	{
		cc_uint8 page = table[BA_PAGE(i)];

		// If entire page is used, skip entirely over it
		if ((i & 0x07) == 0 && page == 0xFF) { i += 8; continue; }

		// If block is used, move onto trying next block
 		if (page & BA_BIT(i)) { i++; continue; }
		
		// If can't be allocated starting at block, try next
		if (!blockalloc_can_alloc(table, i, blocks)) { i++; continue; }

		for (j = i; j < i + blocks; j++) 
		{
			BA_BLOCK_MARK_USED(table, j);
        }
        return i;
    }
	return -1;
}

static void blockalloc_dealloc(cc_uint8* table, int base, int blocks) {
	int i;
	for (i = base; i < base + blocks; i++) 
	{
		BA_BLOCK_MARK_FREE(table, i);
    }
}

static CC_INLINE void blockalloc_calc_usage(cc_uint8* table, int maxBlocks, int blockSize,
											int* freeSize, int* usedSize) {
	int i, total_free = 0;
	// Could be optimised to look at entire page in 0x00 and 0xFF case
	// But this method is called so infrequently, so not really worth it

	for (i = 0; i < maxBlocks; i++) 
	{
 		if (!BA_BLOCK_USED(table, i)) total_free++;
    }
	
	*freeSize = blockSize * total_free;
	*usedSize = blockSize * (maxBlocks - total_free);
}

static CC_INLINE int blockalloc_shift(cc_uint8* table, int base, int blocks) {
	int moved = 0;

	// Try to shift downwards towards prior allocated entry
	base--;
	while (base >= 0 && !BA_BLOCK_USED(table, base))
	{
		// Mark previously last allocated block as now unused
		BA_BLOCK_MARK_FREE(table, base + blocks);
		moved++;

		// Mark previously last empty block as now used
		BA_BLOCK_MARK_USED(table, base);
		base--;
	}
	return moved;
}
