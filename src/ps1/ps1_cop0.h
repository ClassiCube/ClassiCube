// === COP0 registers ===
#define COP0R_STATUS 12

#define COP0_SetReg(reg, val) __asm__ volatile("mtc0 %0, $%1; nop;" : : "r" (val), "i" (reg) : )
#define COP0_GetReg(reg, dst) __asm__ volatile("nop; mfc0 %0, $%1;" : "=r" (dst) : "i" (reg) : )


#define COP0_STATUS_COP2_EN (1u << 30)
