#define GTE_Set_TransX(val) __asm__ volatile("nop; ctc2 %0, $5;" :: "r" (val) : )
#define GTE_Set_TransY(val) __asm__ volatile("nop; ctc2 %0, $6;" :: "r" (val) : )
#define GTE_Set_TransZ(val) __asm__ volatile("nop; ctc2 %0, $7;" :: "r" (val) : )

// Sets C2_OFX register (screen offset X added after vertex transform)
#define GTE_Set_ScreenOffsetX(val)  __asm__ volatile ("nop; ctc2 %0, $24;" :: "r" (val) : )
// Sets C2_OFY register (screen offset Y added after vertex transform)
#define GTE_Set_ScreenOffsetY(val)  __asm__ volatile ("nop; ctc2 %0, $25;" :: "r" (val) : )
// Sets C2_H register (projection plane distance, i.e. field of view)
#define GTE_Set_ScreenDistance(val) __asm__ volatile ("nop; ctc2 %0, $26;" :: "r" (val) : )

#define GTE_Get_OTZ(dst)  __asm__ volatile("mfc2 %0, $7;  nop;" : "=r" (dst) :: )
#define GTE_Get_MAC0(dst) __asm__ volatile("mfc2 %0, $24; nop;" : "=r" (dst) :: )

#define GTE_Exec_RTPT()  __asm__ volatile ("nop; nop; cop2 0x00280030;")
#define GTE_Exec_NCLIP() __asm__ volatile ("nop; nop; cop2 0x01400006;")
#define GTE_Exec_AVSZ3() __asm__ volatile ("nop; nop; cop2 0x0158002D;")
#define GTE_Exec_RTPS()  __asm__ volatile ("nop; nop; cop2 0x00180001;")

// e.g. expands to "swc2 $14, 8(%0);"
#define GTE_Store_XY0(dst, off) __asm__ volatile ("swc2 $12, %1(%0);" :: "r" (dst), "i" (off) : "memory" )
#define GTE_Store_XY1(dst, off) __asm__ volatile ("swc2 $13, %1(%0);" :: "r" (dst), "i" (off) : "memory" )
#define GTE_Store_XY2(dst, off) __asm__ volatile ("swc2 $14, %1(%0);" :: "r" (dst), "i" (off) : "memory" )

#define GTE_Load_XY0(src,  off) __asm__ volatile ("lwc2 $0, 0 + %1(%0);" :: "r" (src), "i" (off) : )
#define GTE_Load__Z0(src,  off) __asm__ volatile ("lwc2 $1, 4 + %1(%0);" :: "r" (src), "i" (off) : )
#define GTE_Load_XY1(src,  off) __asm__ volatile ("lwc2 $2, 0 + %1(%0);" :: "r" (src), "i" (off) : )
#define GTE_Load__Z1(src,  off) __asm__ volatile ("lwc2 $3, 4 + %1(%0);" :: "r" (src), "i" (off) : )
#define GTE_Load_XY2(src,  off) __asm__ volatile ("lwc2 $4, 0 + %1(%0);" :: "r" (src), "i" (off) : )
#define GTE_Load__Z2(src,  off) __asm__ volatile ("lwc2 $5, 4 + %1(%0);" :: "r" (src), "i" (off) : )

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
