// Scratchpad memory is essentially data cache of CPU,
//  and so takes less cycles to access
#define SCRATCHPAD_MEM ((cc_uint8*)0x1F800000)

// === DMA REGISTERS ===
enum dma_chrc_flags {
	CHRC_STATUS_BUSY = (1 << 24),
};

enum dma_chrc_CMD {
	CHRC_FROM_RAM      = (1 << 0),
	CHRC_DIR_DECREMENT = (1 << 1),
	CHRC_MODE_SLICE    = (1 << 9),
	CHRC_MODE_CHAIN    = (1 << 10),
	CHRC_BEGIN_XFER    = (1 << 24),
	CHRC_NO_DREQ_WAIT  = (1 << 28),
};

