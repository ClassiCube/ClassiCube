static volatile uint32_t vblanks_0;
static volatile uint32_t vblanks_1;

static void handle_vblank(void* ptr) {
	uint32_t* counter = (uint32_t*)ptr;
	(*counter)++;
}

static void GSP_setup(void) {
	// Start listening for VBLANK events
	gspSetEventCallback(GSPGPU_EVENT_VBlank0, handle_vblank, (void*)&vblanks_0, false);
	gspSetEventCallback(GSPGPU_EVENT_VBlank1, handle_vblank, (void*)&vblanks_1, false);
}

// Waits for VBLANK for both top and bottom screens
static void GSP_wait_for_full_vblank(void) {
	uint32_t init0 = vblanks_0;
	uint32_t init1 = vblanks_1;

	for (;;) {
		gspWaitForAnyEvent();
		if (vblanks_0 != init0 && vblanks_1 != init1) return;
	}
}

