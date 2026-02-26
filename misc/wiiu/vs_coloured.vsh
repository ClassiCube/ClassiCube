; $MODE = "UniformRegister"

; $SPI_VS_OUT_CONFIG.VS_EXPORT_COUNT = 0
; $NUM_SPI_VS_OUT_ID = 1
; out_colour
; $SPI_VS_OUT_ID[0].SEMANTIC_0 = 0

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

; --------  Disassembly --------------------
00 CALL_FS NO_BARRIER 
01 ALU: ADDR(32) CNT(16)
      0  x: MUL         R0.x,  R1.z,  C2.x
         y: MUL         R0.y,  R1.z,  C2.y
         z: MUL         R0.z,  R1.z,  C2.z
         w: MUL         R0.w,  R1.z,  C2.w
      1  x: MULADD      R0.x,  R1.y,  C1.x,  PV0.x
         y: MULADD      R0.y,  R1.y,  C1.y,  PV0.y
         z: MULADD      R0.z,  R1.y,  C1.z,  PV0.z
         w: MULADD      R0.w,  R1.y,  C1.w,  PV0.w
      2  x: MULADD      R0.x,  R1.x,  C0.x,  PV1.x
         y: MULADD      R0.y,  R1.x,  C0.y,  PV1.y
         z: MULADD      R0.z,  R1.x,  C0.z,  PV1.z
         w: MULADD      R0.w,  R1.x,  C0.w,  PV1.w
      3  x: ADD         R0.x,         C3.x,  PV2.x
         y: ADD         R0.y,         C3.y,  PV2.y
         z: ADD         R0.z,         C3.z,  PV2.z
         w: ADD         R0.w,         C3.w,  PV2.w
02 EXP_DONE: POS0,   R0
03 EXP_DONE: PARAM0, R2  NO_BARRIER 
END_OF_PROGRAM

