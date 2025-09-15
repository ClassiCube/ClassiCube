!!VP1.1
# cgc version 3.1.0013, build date Apr 24 2012
# command line args: -profile vp20
# source file: misc/xbox/vs_coloured.cg
#vendor NVIDIA Corporation
#version 3.1.0.13
#profile vp20
#program main
#semantic main.mvp
#var float4 input.col : $vin.DIFFUSE : ATTR3 : 0 : 1
#var float4 input.pos : $vin.POSITION : ATTR0 : 0 : 1
#var float4x4 mvp :  : c[0], 4 : 1 : 1
#var float4 main.col : $vout.COLOR : COL0 : -1 : 1
#var float4 main.pos : $vout.POSITION : HPOS : -1 : 1
MUL   R0, v[0].y, c[1];
MAD   R0, v[0].x, c[0], R0;
MAD   R0, v[0].z, c[2], R0;
ADD   R0, R0, c[3];
RCP   R1.x, R0.w;
MUL   o[HPOS].xyz, R0, R1.x;
MOV   o[COL0], v[3];
MOV   o[HPOS].w, R0;
END
# 8 instructions, 0 R-regs
