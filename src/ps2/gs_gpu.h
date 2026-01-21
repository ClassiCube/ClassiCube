typedef volatile u32 vu32;
typedef volatile u64 vu64;


// GS registers
#define GS_CSR    *(vu64*)(0x12001000)

// GS CSR register components
#define GS_CSR_FINISH 0x0002
#define GS_CSR_VSYNC  0x0008


static CC_INLINE void GS_wait_draw_finish(void) {
	while (!(GS_CSR & GS_CSR_FINISH)) { }

	// NOTE: Resets 'finish signalled' flag to 0
	GS_CSR = GS_CSR_FINISH;
}

static CC_INLINE void GS_wait_vsync(void) {
	// If 'vsync signalled' flag is set, reset it to 0
	// If it wasn't set, then leaves it alone
	GS_CSR = GS_CSR & GS_CSR_VSYNC;

	while (!(GS_CSR & GS_CSR_VSYNC)) { }
}

