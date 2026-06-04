// === COP2/GTE control registers ===
#define C2CR_RT11_12  0
#define C2CR_RT13_21  1
#define C2CR_RT22_23  2
#define C2CR_RT31_32  3
#define C2CR_RT33     4
#define C2CR_TRANSX   5
#define C2CR_TRANSY   6
#define C2CR_TRANSZ   7
#define C2CR_SCR_OFX  24
#define C2CR_SCR_OFY  25
#define C2CR_SCR_DIST 26
#define C2CR_SCR_DIST 26
#define C2CR_AVE_ZSF3 29
#define C2CR_AVE_ZSF4 30

#define GTE_SetCtrlReg(reg, val) __asm__ volatile("nop; ctc2 %0, $%1;" : : "r" (val), "i" (reg) : )

// === COP2/GTE data registers ===
#define C2DR_VXY0     0
#define C2DR_VZ0      1
#define C2DR_VXY1     2
#define C2DR_VZ1      3
#define C2DR_VXY2     4
#define C2DR_VZ2      5
#define C2DR_OTZ      7
#define C2DR_XY0      12
#define C2DR_XY1      13
#define C2DR_XY2      14
#define C2DR_Z0       16
#define C2DR_Z1       17
#define C2DR_Z2       18
#define C2DR_Z3       19
#define C2DR_MAC0     24

#define GTE_GetDataReg(reg, dst)       __asm__ volatile ("nop; mfc2 %0, $%1;" : "=r" (dst) : "i" (reg) : )
#define GTE_LoadDataReg(reg, dst, off) __asm__ volatile ("lwc2 $%2, %1(%0);" : : "r" (dst), "i" (off), "i" (reg) : "memory" )
#define GTE_SaveDataReg(reg, dst, off) __asm__ volatile ("swc2 $%2, %1(%0);" : : "r" (dst), "i" (off), "i" (reg) : "memory" )
// e.g. expands to "swc2 $14, 8($5);"

// === GTE commands ===
#define GTE_Exec_RTPT()  __asm__ volatile ("nop; nop; cop2 0x00280030;")
#define GTE_Exec_NCLIP() __asm__ volatile ("nop; nop; cop2 0x01400006;")
#define GTE_Exec_AVSZ3() __asm__ volatile ("nop; nop; cop2 0x0158002D;")
#define GTE_Exec_RTPS()  __asm__ volatile ("nop; nop; cop2 0x00180001;")

#define GTE_Load_RotMatrix(mat) __asm__ volatile ( \
	"lw		$t0,  0(%0);\n" \
	"lw		$t1,  4(%0);\n" \
	"lw		$t2,  8(%0);\n" \
	"lw		$t3, 12(%0);\n" \
	"lhu	$t4, 16(%0);\n" \
	"ctc2	$t0, $0;\n"		\
	"ctc2	$t1, $1;\n"		\
	"ctc2	$t2, $2;\n"		\
	"ctc2	$t3, $3;\n"		\
	"ctc2	$t4, $4;\n"		\
	:: "r" (mat) : "$t0", "$t1", "$t2", "$t3", "$t4" )
