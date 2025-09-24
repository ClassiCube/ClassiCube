// cgc version 3.1.0013, build date Apr 24 2012
// command line args: -profile vp20
// source file: misc/xbox/vs_offset.cg
//vendor NVIDIA Corporation
//version 3.1.0.13
//profile vp20
//program main
//semantic main.mvp
//semantic main.tex_offset
//var float4 input.tex : $vin.TEXCOORD : TEXCOORD0 : 0 : 1
//var float4 input.col : $vin.DIFFUSE : ATTR3 : 0 : 1
//var float4 input.pos : $vin.POSITION : ATTR0 : 0 : 1
//var float4x4 mvp :  : c[0], 4 : 1 : 1
//var float4 tex_offset :  : c[4] : 2 : 1
//var float4 main.pos : $vout.POSITION : HPOS : -1 : 1
//var float4 main.col : $vout.COLOR : COL0 : -1 : 1
//var float4 main.tex : $vout.TEXCOORD0 : TEX0 : -1 : 1
// 9 instructions, 0 R-regs
0x00000000, 0x004c2055, 0x0836186c, 0x2f0007f8,
0x00000000, 0x008c0000, 0x0836186c, 0x1f0007f8,
0x00000000, 0x008c40aa, 0x0836186c, 0x1f0007f8,
0x00000000, 0x006c601b, 0x0436106c, 0x3f0007f8,
0x00000000, 0x0400001b, 0x08361300, 0x101807f8,
0x00000000, 0x0040001b, 0x0400286c, 0x2070e800,
0x00000000, 0x0020001b, 0x0436106c, 0x20701800,
0x00000000, 0x0020061b, 0x0836106c, 0x2070f818,
0x00000000, 0x006c921b, 0x0836106c, 0x3070f849,
