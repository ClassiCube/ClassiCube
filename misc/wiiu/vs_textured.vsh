; $MODE = "UniformRegister"

; $SPI_VS_OUT_CONFIG.VS_EXPORT_COUNT = 1
; $NUM_SPI_VS_OUT_ID = 1
; out_colour
; $SPI_VS_OUT_ID[0].SEMANTIC_0 = 0
; out_uv
; $SPI_VS_OUT_ID[0].SEMANTIC_1 = 1

; C0
; $UNIFORM_VARS[0].name = "mvp"
; $UNIFORM_VARS[0].type = "mat4"
; $UNIFORM_VARS[0].count = 1
; $UNIFORM_VARS[0].block = -1
; $UNIFORM_VARS[0].offset = 0

; R1
; $ATTRIB_VARS[0].name = "in_pos"
; $ATTRIB_VARS[0].type = "vec3"
; $ATTRIB_VARS[0].location = 0
; R2
; $ATTRIB_VARS[1].name = "in_col"
; $ATTRIB_VARS[1].type = "vec4"
; $ATTRIB_VARS[1].location = 1
; R3
; $ATTRIB_VARS[2].name = "in_uv"
; $ATTRIB_VARS[2].type = "vec2"
; $ATTRIB_VARS[2].location = 2

; --------  Disassembly --------------------
00 CALL_FS NO_BARRIER 
01 ALU: ADDR(32) CNT(12)
      0  x: MULADD      R4.x,  R1.z,  C2.x,  C3.x
         y: MULADD      R4.y,  R1.z,  C2.y,  C3.y
         z: MULADD      R4.z,  R1.z,  C2.z,  C3.z
         w: MULADD      R4.w,  R1.z,  C2.w,  C3.w
      1  x: MULADD      R4.x,  R1.y,  C1.x,  PV0.x
         y: MULADD      R4.y,  R1.y,  C1.y,  PV0.y
         z: MULADD      R4.z,  R1.y,  C1.z,  PV0.z
         w: MULADD      R4.w,  R1.y,  C1.w,  PV0.w
      2  x: MULADD      R1.x,  R1.x,  C0.x,  PV1.x
         y: MULADD      R1.y,  R1.x,  C0.y,  PV1.y
         z: MULADD      R1.z,  R1.x,  C0.z,  PV1.z
         w: MULADD      R1.w,  R1.x,  C0.w,  PV1.w
02 EXP_DONE: POS0, R1
03 EXP: PARAM0, R2  NO_BARRIER 
04 EXP_DONE: PARAM1, R3.xyzz  NO_BARRIER 
END_OF_PROGRAM

