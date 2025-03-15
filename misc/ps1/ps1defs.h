// Scratchpad memory is essentially data cache of CPU,
//  and so takes less cycles to access
#define SCRATCHPAD_MEM ((cc_uint8*)0x1F800000)

// === GTE COMMANDS ===
#define GTE_Set_TransX(value) __asm__ volatile("ctc2 %0, $5;" :: "r" (value) : )
#define GTE_Set_TransY(value) __asm__ volatile("ctc2 %0, $6;" :: "r" (value) : )
#define GTE_Set_TransZ(value) __asm__ volatile("ctc2 %0, $7;" :: "r" (value) : )

#define GTE_Get_OTZ(dst)  __asm__ volatile("mfc2 %0, $7;  nop;" : "=r" (dst) :: )
#define GTE_Get_MAC0(dst) __asm__ volatile("mfc2 %0, $24; nop;" : "=r" (dst) :: )

#define GTE_Exec_RTPT()  __asm__ volatile ("nop; nop; cop2 0x00280030;")
#define GTE_Exec_NCLIP() __asm__ volatile ("nop; nop; cop2 0x01400006;")
#define GTE_Exec_AVSZ3() __asm__ volatile ("nop; nop; cop2 0x0158002D;")
#define GTE_Exec_RTPS()  __asm__ volatile ("nop; nop; cop2 0x00180001;")

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

// === GPU REGISTERS ===
enum gpu_status_flags {
	GPU_STATUS_CMD_READY      = (1 << 26),
	GPU_STATUS_DMA_RECV_READY = (1 << 28),
};


// === GP0 COMMANDS ===
enum gp0_cmd_type {
	GP0_CMD_CLEAR_VRAM_CACHE = 0x01000000,
	GP0_CMD_MEM_FILL         = 0x02000000,
	GP0_CMD_TRANSFER_TO_VRAM = 0xA0000000,
	GP0_CMD_POLYGON          = 0x20000000,
	GP0_CMD_RECTANGLE        = 0x60000000,
	GP0_CMD_TEXTURE_PAGE     = 0xE1000000,
	GP0_CMD_TEXTURE_WINDOW   = 0xE2000000,
	GP0_CMD_DRAW_AREA_MIN    = 0xE3000000,
	GP0_CMD_DRAW_AREA_MAX    = 0xE4000000,
	GP0_CMD_DRAW_AREA_OFFSET = 0xE5000000,
};

enum gp0_polycmd_flags {
	POLY_CMD_QUAD     = 1u << 27,
	POLY_CMD_TEXTURED = 1u << 26,
	POLY_CMD_SEMITRNS = 1u << 25,
};

enum gp0_rectcmd_flags {
	RECT_CMD_1x1 = 1u << 27,
};

#define PACK_RGBC(r, g, b, c) ( (r) | ((g) << 8) | ((b) << 16) | (c) )

#define GP0_CMD_DRAW_MIN_PACK(x, y) ( GP0_CMD_DRAW_AREA_MIN | ((x) & 0x3FF) | (((y) & 0x3FF) << 10) )
#define GP0_CMD_DRAW_MAX_PACK(x, y) ( GP0_CMD_DRAW_AREA_MAX | ((x) & 0x3FF) | (((y) & 0x3FF) << 10) )

#define GP0_CMD_DRAW_OFFSET_PACK(x, y) ( GP0_CMD_DRAW_AREA_OFFSET | ((x) & 0x7FF) | (((y) & 0x7FF) << 11) )

#define GP0_CMD_FILL_XY(x, y) ( ((x) & 0xFFFF) | (((y) & 0xFFFF) << 16) )
#define GP0_CMD_FILL_WH(w, h) ( ((w) & 0xFFFF) | (((h) & 0xFFFF) << 16) )

#define GP0_CMD_CLUT_XY(x, y) ( ((x) & 0x3F) | (((y) & 0x1FF) << 6) )


// === GP1 COMMANDS ===
enum gp1_cmd_type {
	GP1_CMD_RESET_GPU        = 0x00000000,
	GP1_CMD_DISPLAY_ACTIVE   = 0x03000000,
	GP1_CMD_DMA_MODE         = 0x04000000,
	GP1_CMD_DISPLAY_ADDRESS  = 0x05000000,
	GP1_CMD_HORIZONTAL_RANGE = 0x06000000,
	GP1_CMD_VERTICAL_RANGE   = 0x07000000,
	GP1_CMD_VIDEO_MODE       = 0x08000000,
};

enum gp1_cmd_display {
	GP1_DISPLAY_DISABLED = 1,
	GP1_DISPLAY_ENABLED  = 0,
};

enum gp1_cmd_dma_mode {
	GP1_DMA_NONE       = 0,
	GP1_DMA_CPU_TO_GP0 = 2,
};

enum gp1_cmd_display_mode {
	GP1_HOR_RES_320 = 1 << 0,
	GP1_VER_RES_240 = 0 << 2,
};

#define GP1_CMD_DISPLAY_ADDRESS_XY(x, y) ( ((x) & 0x3FF) | (((y) & 0x1FF) << 10) )


// === GP0 POLYGON COMMANDS ===
#define POLY_CODE_F4 (GP0_CMD_POLYGON | POLY_CMD_QUAD)
#define POLY_LEN_F4  5
struct PSX_POLY_F4 {
	uint32_t tag;
	uint32_t rgbc; // r0, g0, b0, code;
	int16_t	 x0, y0;
	int16_t	 x1, y1;
	int16_t	 x2, y2;
	int16_t	 x3, y3;
};

#define POLY_CODE_FT4 (GP0_CMD_POLYGON | POLY_CMD_QUAD | POLY_CMD_TEXTURED)
#define POLY_LEN_FT4  9
struct PSX_POLY_FT4 {
	uint32_t tag;
	uint32_t rgbc; // r0, g0, b0, code;
	uint16_t x0, y0;
	uint8_t	 u0, v0;
	uint16_t clut;
	int16_t  x1, y1;
	uint8_t	 u1, v1;
	uint16_t tpage;
	int16_t	 x2, y2;
	uint8_t	 u2, v2;
	uint16_t pad0;
	int16_t	 x3, y3;
	uint8_t	 u3, v3;
	uint16_t pad1;
};
