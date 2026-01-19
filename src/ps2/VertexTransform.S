#include "AsmCommon.i"
// NOTE: vclipw.xyz takes 4 cycles to produce result, which must be accounted for

// mips ISA has explicit delay slots
// (i.e. instruction after branches/jumps are always unconditionally executed)
.set noreorder

# transform temp registers
	#define POSCL $vf10 // TRANSFORMED(POS_[1234]) * CLIP_PLANES_ADJUST
	#define POS_1 $vf11 // vertex 1 position
	#define POS_2 $vf12 // vertex 2 position
	#define POS_3 $vf13 // vertex 3 position
	#define POS_4 $vf14 // vertex 4 position


// Loads matrix into VU0 registers
//	$a0 = addresss of mvp
BEG_FUNC(LoadMvpMatrix)
	lqc2 	MVP1, 0x00($a0) # vf1 = mvp.row1
	lqc2 	MVP2, 0x10($a0) # vf2 = mvp.row2
	lqc2 	MVP3, 0x20($a0) # vf3 = mvp.row3
	jr		$ra
	lqc2 	MVP4, 0x30($a0) # vf4 = mvp.row4
END_FUNC(LoadMvpMatrix)

// Loads clipping scaling factors into VU0 registers
//	$a0 = addresss of factors
BEG_FUNC(LoadClipScaleFactors)
	jr		$ra
	lqc2 	CL_F, 0x00($a0)
END_FUNC(LoadClipScaleFactors)

// Loads viewport origin into VU0 registers
//	$a0 = addresss of origin
BEG_FUNC(LoadViewportOrigin)
	jr		$ra
	lqc2 	VP_O, 0x00($a0)
END_FUNC(LoadViewportOrigin)

// Loads viewport scale into VU0 registers
//	$a0 = addresss of scale
BEG_FUNC(LoadViewportScale)
	jr		$ra
	lqc2 	VP_S, 0x00($a0)
END_FUNC(LoadViewportScale)

