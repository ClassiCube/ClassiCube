!!VP1.1
# cgc version 3.1.0013, build date Apr 24 2012
# command line args: -profile vp20
# source file: misc/xbox/vs_textured.cg
#vendor NVIDIA Corporation
#version 3.1.0.13
#profile vp20
#program main
#semantic main.mvp
#var float4 in_pos : $vin.POSITION : ATTR0 : 0 : 1
#var float4 in_col : $vin.DIFFUSE : ATTR3 : 1 : 1
#var float4 in_tex : $vin.TEXCOORD : TEXCOORD0 : 2 : 1
#var float4x4 mvp :  : c[0], 4 : 3 : 1
#var float4 out_pos : $vout.POSITION : HPOS : 4 : 1
#var float4 out_col : $vout.COLOR : COL0 : 5 : 1
#var float4 out_tex : $vout.TEXCOORD0 : TEX0 : 6 : 1
MUL   R0, v[0].y, c[1];
MAD   R0, v[0].x, c[0], R0;
MAD   R0, v[0].z, c[2], R0;
ADD   R0, R0, c[3];
RCP   R1.x, R0.w;
MUL   o[HPOS].xyz, R0, R1.x;
MOV   o[HPOS].w, R0;
MOV   o[COL0], v[3];
MOV   o[TEX0], v[8];
END
# 9 instructions, 0 R-regs
