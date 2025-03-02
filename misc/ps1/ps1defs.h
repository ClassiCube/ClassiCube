enum dma_chrc_flags {
	CHRC_STATUS_BUSY = (1 << 24),
};

enum dma_chrc_CMD {
	CHRC_FROM_RAM   = (1 << 0),
	CHRC_BEGIN      = (1 << 24),
	CHRC_MODE_SLICE = (1 << 9)
};

enum gpu_status_flags {
	GPU_STATUS_CMD_READY      = (1 << 26),
	GPU_STATUS_DMA_RECV_READY = (1 << 28),
};


enum gp0_cmd_type {
	GP0_CMD_CLEAR_VRAM_CACHE = 0x01000000,
	GP0_CMD_TRANSFER_TO_VRAM = 0xA0000000,
	GP0_CMD_POLYGON          = 0x20000000,
};

enum gp0_polycmd_flags {
	POLY_CMD_QUAD     = 1u << 27,
	POLY_CMD_TEXTURED = 1u << 26,
	POLY_CMD_SEMITRNS = 1u << 25,
};

enum gp1_cmd_type {
	GP1_CMD_DMA_MODE = 0x04000000,
};

enum gp1_cmd_dma_mode {
	GP1_DMA_NONE       = 0,
	GP1_DMA_CPU_TO_GP0 = 2,
};

