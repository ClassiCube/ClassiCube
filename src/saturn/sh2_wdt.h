static cc_uint32 wdt_overflows;

// Interrupt Priority Request A
#define SH2_REG_IPRA ((volatile uint16_t *)0xFFFFFEE2)
#define SH2_IPRA_WDT_NUM 4

// Vector Number Setting Register WDT
#define SH2_REG_VCRWDT ((volatile uint16_t *)0xFFFFFEE4)
#define SH2_VCRWDT_NUM 8

// Watchdog Timer Counter
#define SH2_REG_WTCNT_R	   ((volatile uint8_t  *)0xFFFFFE81)
#define SH2_REG_WTCNT_W    ((volatile uint16_t *)0xFFFFFE80)
#define SH2_CMD_WTCNT(val) (0x5A00 | (uint8_t)(val))

// Watchdog Timer Control/Status Register
#define SH2_REG_WTCSR_R	   ((volatile uint8_t  *)0xFFFFFE80)
#define SH2_REG_WTCSR_W    ((volatile uint16_t *)0xFFFFFE80)
#define SH2_CMD_WTCSR(val) (0xA500 | (uint8_t)(val))

#define SH2_WTCSR_CLOCK_DIV_2    0
#define SH2_WTCSR_CLOCK_DIV_64   1
#define SH2_WTCSR_CLOCK_DIV_128  2
#define SH2_WTCSR_CLOCK_DIV_256  3
#define SH2_WTCSR_CLOCK_DIV_512  4
#define SH2_WTCSR_CLOCK_DIV_1024 5
#define SH2_WTCSR_CLOCK_DIV_4096 6
#define SH2_WTCSR_CLOCK_DIV_8192 7

#define SH2_WTCSR_ENA_FLG  0x20
#define SH2_WTCSR_OVF_FLG  0x80


static CC_INLINE void wdt_set_irq_number(int irq_num) {
	uint16_t vcrwdt = *SH2_REG_VCRWDT;
	vcrwdt &= ~(   0x7F << SH2_VCRWDT_NUM);
	vcrwdt |=  (irq_num << SH2_VCRWDT_NUM);

	*SH2_REG_VCRWDT = vcrwdt;
}

static CC_INLINE void wdt_set_irq_priority(uint8_t pri) {
	uint16_t ipra = *SH2_REG_IPRA;
	ipra &= ~(0xF << SH2_IPRA_WDT_NUM);
	ipra |=  (pri << SH2_IPRA_WDT_NUM);

	*SH2_REG_IPRA = ipra;
}


void __attribute__((interrupt_handler)) wdt_handler(void) {
    uint8_t wtcr = *SH2_REG_WTCSR_R & ~SH2_WTCSR_OVF_FLG;

    *SH2_REG_WTCSR_W = SH2_CMD_WTCSR(wtcr);
    wdt_overflows++;
}

static CC_INLINE void wdt_stop(void) {
	*SH2_REG_WTCSR_W = SH2_CMD_WTCSR(SH2_WTCSR_CLOCK_DIV_1024);
}

static CC_INLINE void wdt_enable(void) {
	uint8_t wtcr = *SH2_REG_WTCSR_R & ~SH2_WTCSR_OVF_FLG;
    wtcr |= SH2_WTCSR_ENA_FLG;
	// interval mode is 0 anyways

    *SH2_REG_WTCNT_W = SH2_CMD_WTCNT(0);
	*SH2_REG_WTCSR_W = SH2_CMD_WTCSR(wtcr);
}

static CC_INLINE uint32_t wdt_total_ticks(void) {
	return (*SH2_REG_WTCNT_R) | (wdt_overflows << 8);
}
