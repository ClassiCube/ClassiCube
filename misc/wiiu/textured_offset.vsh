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
; C4 (MIGHT BE WRONG AND SHOULD BE [3] instead??? )
; $UNIFORM_VARS[1].name = "texOffset"
; $UNIFORM_VARS[1].type = "vec2"
; $UNIFORM_VARS[1].count = 1
; $UNIFORM_VARS[1].block = -1
; $UNIFORM_VARS[1].offset = 0

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
01 ALU: ADDR(32) CNT(18) 
      0  x: MUL         ____,  C3.y,  1.0f      
         y: MUL         ____,  C3.x,  1.0f      
         z: MUL         ____,  C3.w,  1.0f      
         w: MUL         ____,  C3.z,  1.0f      
      1  x: MULADD      R127.x,  R2.z,  C2.y,  PV0.x      
         y: MULADD      R127.y,  R2.z,  C2.x,  PV0.y      
         z: MULADD      R127.z,  R2.z,  C2.w,  PV0.z      
         w: MULADD      R127.w,  R2.z,  C2.z,  PV0.w      
      2  x: MULADD      R127.x,  R2.y,  C1.y,  PV1.x      
         y: MULADD      R127.y,  R2.y,  C1.x,  PV1.y      
         z: MULADD      R127.z,  R2.y,  C1.w,  PV1.z      
         w: MULADD      R127.w,  R2.y,  C1.z,  PV1.w      
      3  x: MULADD      R2.x,  R2.x,  C0.x,  PV2.y      
         y: MULADD      R2.y,  R2.x,  C0.y,  PV2.x      
         z: MULADD      R2.z,  R2.x,  C0.z,  PV2.w      
         w: MULADD      R2.w,  R2.x,  C0.w,  PV2.z      
      4  x: ADD         R3.x,  R3.x,  C4.x      
         y: ADD         R3.y,  R3.y,  C4.y      
02 EXP_DONE: POS0, R2
03 EXP: PARAM0, R1  NO_BARRIER 
04 EXP_DONE: PARAM1, R3.xyzz  NO_BARRIER 
05 ALU: ADDR(50) CNT(1) 
      5  x: NOP         ____      
06 NOP NO_BARRIER 
END_OF_PROGRAM

