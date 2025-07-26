typedef volatile uint8_t   vu8;
typedef volatile uint16_t vu16;
typedef volatile uint32_t vu32;

#define MEM_IO		    0x04000000
#define MEM_VRAM		0x06000000

// Display functionality
#define REG_DISP_CTRL	*(vu32*)(MEM_IO + 0x0000)

#define DCTRL_MODE3		0x0003
#define DCTRL_BG2		0x0400

// Keypad functionality
#define REG_KEYINPUT	*(vu16*)(MEM_IO + 0x0130)

#define KEY_A			0x0001
#define KEY_B			0x0002
#define KEY_SELECT		0x0004
#define KEY_START		0x0008
#define KEY_RIGHT		0x0010
#define KEY_LEFT		0x0020
#define KEY_UP			0x0040
#define KEY_DOWN		0x0080
#define KEY_R			0x0100
#define KEY_L			0x0200

// Timer functionality
#define REG_TMR0_DATA	*(vu16*)(MEM_IO + 0x0100) // Timer 0 data
#define REG_TMR0_CTRL	*(vu16*)(MEM_IO + 0x0102) // Timer 0 control
#define REG_TMR1_DATA	*(vu16*)(MEM_IO + 0x0104) // Timer 1 data
#define REG_TMR1_CTRL	*(vu16*)(MEM_IO + 0x0106) // Timer 1 control
#define REG_TMR2_DATA	*(vu16*)(MEM_IO + 0x0108) // Timer 2 data
#define REG_TMR2_CTRL	*(vu16*)(MEM_IO + 0x010A) // Timer 2 control
#define REG_TMR3_DATA	*(vu16*)(MEM_IO + 0x010C) // Timer 3 data
#define REG_TMR3_CTRL	*(vu16*)(MEM_IO + 0x010E) // Timer 3 control

#define TMR_CASCADE		0x0004
#define TMR_ENABLE		0x0080

#define SYS_CLOCK 16777216
