#define BA_PAGE(block) ((block) >> 3)
#define BA_BIT(block)  (1 << ((block) & 0x07))
#define BLOCKS_PER_PAGE 8

#define SIZE_TO_BLOCKS(size, blockSize) (((size) + ((blockSize) - 1)) / (blockSize))

static CC_INLINE int blockalloc_can_alloc(cc_uint8* table, int beg, int blocks) {
	for (int i = beg; i < beg + blocks; i++)
	{
		cc_uint8 page = table[BA_PAGE(i)];
		if (page & BA_BIT(i)) return false;
	}
	return true;
}

static int blockalloc_alloc(cc_uint8* table, int maxBlocks, int blocks) {
	if (blocks > maxBlocks) return -1;

	for (int i = 0; i <= maxBlocks - blocks;) 
	{
		cc_uint8 page = table[BA_PAGE(i)];

		// If entire page is used, skip entirely over it
		if ((i & 0x07) == 0 && page == 0xFF) { i += 8; continue; }

		// If block is used, move onto trying next block
 		if (page & BA_BIT(i)) { i++; continue; }
		
		// If can't be allocated starting at block, try next
		if (!blockalloc_can_alloc(table, i, blocks)) { i++; continue; }

		for (int j = i; j < i + blocks; j++) 
		{
			table[BA_PAGE(j)] |= BA_BIT(j);
        }
        return i;
    }
	return -1;
}

static void blockalloc_dealloc(cc_uint8* table, int origin, int blocks) {
	// Mark the used blocks as free again
	for (int i = origin; i < origin + blocks; i++) 
	{
		table[BA_PAGE(i)] &= ~BA_BIT(i);
    }
}

static CC_INLINE int blockalloc_total_free(cc_uint8* table, int maxBlocks) {
	int free_blocks = 0, i, used;
	cc_uint8 page;
	// could be optimised to look at entire page in 0x00 and 0xFF case
	// but this method is called so infrequently, not really worth it

	for (i = 0; i < maxBlocks; i++) 
	{
		page = table[BA_PAGE(i)];
		used = page & BA_BIT(i);

 		if (!used) free_blocks++;
    }
	return free_blocks;
}
