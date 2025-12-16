! Calculates vertex as the near plane intersection point between two points:
!    float t = fabsf(v1->z) / fabsf(v2->z - v1->z)
!    float w = (1 - t) * v1->w + t * v2->w;
!    
!    out->c = type << 24
!    out->x = ((1 - t) * v1->x + t * v2->x) * 1/w
!    out->y = ((1 - t) * v1->y + t * v2->y) * 1/w
!    out->w = 1/w  
!    
!    out->u = (1 - t) * v1->u + t * v2->u;
!    out->v = (1 - t) * v1->v + t * v2->v;
!    
!    out->b = (1 - t) * v1->b + t * v2->b;
!    out->g = (1 - t) * v1->g + t * v2->g;
!    out->r = (1 - t) * v1->r + t * v2->r;
!    out->a = (1 - t) * v1->a + t * v2->a;

! Optimisations:
! - w always ends up being zNear
! - Calculations of (1 - t) * v1 + t * v2 can be rearranged to t * (v2 - v1) + v1
! - These rearranges calculations can then take advantage of FMAC 

! Final calculation:
!    out->c = type << 24
!    out->x = (t * (v2->x - v1->x) + v1->x) * 1/zNear
!    out->y = (t * (v2->y - v1->y) + v1->y) * 1/zNear
!    out->w = 1/zNear  
!    
!    out->u = t * (v2->u - v1->u) + v1->u;
!    out->v = t * (v2->v - v1->v) + v1->v;
!    
!    out->b = t * (v2->b - v1->b) + v1->b;
!    out->g = t * (v2->g - v1->g) + v1->g;
!    out->r = t * (v2->r - v1->r) + v1->r;
!    out->a = t * (v2->a - v1->a) + v1->a;

! INPUT ARGUMENTS
#define IN1 r4 // input vertex 1
#define IN2 r5 // input vertex 2
#define OUT r6 // output vertex
#define TYP r7 // type/flags for output vertex

#define TM1 r1 // temp register 1
#define TM2 r3 // temp register 2
#define CL1 r4 // input colour 1
#define CL2 r5 // input colour 2
#define CLO r7 // output colour

#define F_T  fr0
#define F_W  fr1
#define F_X1 fr2
#define F_X2 fr3
#define F_Y1 fr4
#define F_Y2 fr5
#define F_U1 fr6
#define F_U2 fr7
#define F_V1 fr8
#define F_V2 fr9
#define F_Z1 fr10
#define F_Z2 fr11
#define Ftmp fr11

.global _ClipEdge
.align 4
_ClipEdge:
	fschg              ! FE (swap to 32 bit FPU loads/stores)
! Start calculating interpolation factor
	add      #28, IN1  ! EX, IN1 = &v1->z
	mov.l _NEAR_CLIP_W,TM1 ! LS, tmp = invW (1/zNear)
	fmov.s  @IN1, F_Z1 ! LS, Z1  = v1->z
	add      #28, IN2  ! EX, IN  = &v2->z
	fmov.s  @IN2, F_Z2 ! LS, Z2  = v2->z
	shll16   TYP       ! EX, TYP <<= 16
	fsub    F_Z1, Ftmp ! FE, tmp = v2->z - v1->z
! Load W
	lds 	 TM1,fpul  ! LS, FPUL = invW (1/zNear)
	add     #-24, IN1  ! EX, IN1  = &v1->x
	fsts     fpul,F_W  ! LS, invW = FPUL
	add     #-24, IN2  ! EX, IN2  = &v2->x
! Finish calculating interpolation factor
	shll8    TYP       ! EX, TYP <<= 8
	fmul    Ftmp,Ftmp  ! FE, tmp = (v2->z - v1->z) * (v2->z * v1->z)
	mov.l  TYP,@OUT    ! LS, dst->cmd = TYPE

! Load components
	fmov.s @IN1+, F_X1 ! LS, X1 = v1->x
	fmov.s @IN2+, F_X2 ! LS, X2 = v2->x
	fmov.s @IN1+, F_Y1 ! LS, Y1 = y1->x
	fmov.s @IN2+, F_Y2 ! LS, Y2 = y2->x
	fsrra   Ftmp       ! FE, tmp = 1 / abs(v2->z - v1->z)
	add       #4, IN1  ! EX, skip over W
	fabs    F_Z1       ! LS, z1 = abs(v1->z)
	add       #4, IN2  ! EX, skip over W
	fmov.s @IN1+, F_U1 ! LS, U1 = v1->u
	fmov.s @IN2+, F_U2 ! LS, U2 = v2->u
	fmov.s @IN1+, F_V1 ! LS, V1 = v1->v
	fmul    F_Z1,Ftmp  ! FE, tmp = abs(v1->Z) / abs(v2->z - v1->z)
	fmov.s @IN2+, F_V2 ! LS, V2 = v2->v

! Interpolate vertices
	fsub    F_X1, F_X2 ! FE, X2 = X2 - X1
	fsub    F_Y1, F_Y2 ! FE, Y2 = Y2 - Y1
	fsub    F_U1, F_U2 ! FE, U2 = U2 - U1
	fmov   Ftmp, F_T   ! LS,   T  = tmp
	fsub    F_V1, F_V2 ! FE, V2 = V2 - V1

	fmac F_T,F_X2,F_X1 ! FE, X = T * (X2 - X1) + X1
	fmac F_T,F_Y2,F_Y1 ! FE, Y = T * (Y2 - Y1) + Y1
	fmac F_T,F_U2,F_U1 ! FE, U = T * (U2 - U1) + U1
	fmac F_T,F_V2,F_V1 ! FE, V = T * (V2 - V1) + V1
	
! Adjust by w
	fmul	 F_W, F_X1 ! FE, x = invW * x
	fmul	 F_W, F_Y1 ! FE, x = invY * x

! Load colours and check if equal
	mov.l  @IN1,CL1    ! LS, ACOLOR = v1->bgra
	mov.l  @IN2,CL2    ! LS, BCOLOR = v2->bgra
	cmp/eq  CL1,CL2    ! MT, T = ACOLOR == BCOLOR
	add		 #28,  OUT ! EX, dst = &dst->padding
	
! Bypass RGBA interpolation if unnecessary
	bt.s    1f         ! BR, if (T) goto 1;
	mov     CL1,CLO    ! MT, OUTCOLOR = ACOLOR (branch delay instruction)

! Interpolate B
	extu.b  CL1,TM1    ! EX, val  = ACOLOR.b
	lds     TM1,fpul   ! LS, FPUL = val
	float   fpul,F_Z1  ! FE,  C1  = float(val)
	extu.b  CL2,TM1    ! EX, val  = BCOLOR.b
	lds     TM1,fpul   ! LS, FPUL = val
	float   fpul,F_Z2  ! FE,  C2  = float(val)
	fsub    F_Z1, F_Z2 ! FE,  C2  = C2 - C1
	fmac F_T,F_Z2,F_Z1 ! FE,  C   = T * (C2 - C1) + C1
	shlr8   CL1        ! EX, ACOLOR >>= 8
	shlr8   CL2        ! EX, BCOLOR >>= 8
	ftrc    F_Z1,fpul  ! FE, FPUL = int(C)
	sts     fpul,TM2   ! LS, tmp  = FPUL

! Interpolate G
	extu.b  CL1,TM1    ! EX, val  = ACOLOR.g
	lds     TM1,fpul   ! LS, FPUL = val
	float   fpul,F_Z1  ! FE,  C1  = float(val)
	extu.b  CL2,TM1    ! EX, val  = BCOLOR.g
	lds     TM1,fpul   ! LS, FPUL = val
	float   fpul,F_Z2  ! FE,  C2  = float(val)
	fsub    F_Z1, F_Z2 ! FE,  C2  = C2 - C1
	fmac F_T,F_Z2,F_Z1 ! FE,  C   = T * (C2 - C1) + C1
	shlr8   CL1        ! EX, ACOLOR >>= 8
	extu.b  TM2,TM2    ! EX, tmp  = (uint8)tmp
	mov     TM2,CLO    ! MT, OUTCOLOR.b = tmp
	shlr8   CL2        ! EX, BCOLOR >>= 8
	ftrc    F_Z1,fpul  ! FE, FPUL = int(C)
	sts     fpul,TM2   ! LS, tmp  = FPUL

! Interpolate R
	extu.b  CL1,TM1    ! EX, val  = ACOLOR.r
	lds     TM1,fpul   ! LS, FPUL = val
	float   fpul,F_Z1  ! FE,  C1  = float(val)
	extu.b  CL2,TM1    ! EX, val  = BCOLOR.r
	lds     TM1,fpul   ! LS, FPUL = val
	float   fpul,F_Z2  ! FE,  C2  = float(val)
	fsub    F_Z1, F_Z2 ! FE,  C2  = C2 - C1
	fmac F_T,F_Z2,F_Z1 ! FE,  C   = T * (C2 - C1) + C1
	shlr8   CL1        ! EX, ACOLOR >>= 8
	extu.b  TM2,TM2    ! EX, tmp  = (uint8)tmp
	shll8   TM2        ! EX, tmp <<= 8
	or      TM2,CLO    ! EX, OUTCOLOR.g |= tmp
	shlr8   CL2        ! EX, BCOLOR >>= 8
	ftrc    F_Z1,fpul  ! FE, FPUL = int(C)
	sts     fpul,TM2   ! LS, tmp  = FPUL
	
! Interpolate A
	extu.b  CL1,TM1    ! EX, val  = ACOLOR.a
	lds     TM1,fpul   ! LS, FPUL = val
	float   fpul,F_Z1  ! FE,  C1  = float(val)
	extu.b  CL2,TM1    ! EX, val  = BCOLOR.a
	lds     TM1,fpul   ! LS, FPUL = val
	float   fpul,F_Z2  ! FE,  C2  = float(val)
	fsub    F_Z1, F_Z2 ! FE,  C2  = C2 - C1
	fmac F_T,F_Z2,F_Z1 ! FE,  C   = T * (C2 - C1) + C1
	extu.b  TM2,TM2    ! EX, tmp  = (uint8)tmp
	shll16  TM2        ! EX, tmp <<= 16
	or      TM2,CLO    ! EX, OUTCOLOR.r |= tmp
	ftrc    F_Z1,fpul  ! FE, FPUL = int(C)
	sts     fpul,TM2   ! LS, tmp  = FPUL
	extu.b  TM2,TM2    ! EX, tmp  = (uint8)tmp
	shll16  TM2        ! EX, tmp <<= 16
	shll8   TM2        ! EX, tmp <<= 8
	or      TM2,CLO    ! EX, OUTCOLOR.a |= tmp

1:
! Store output	
	mov.l    CLO,@-OUT ! LS, dst->color = OUTCOLOR
	fmov.s  F_V1,@-OUT ! LS, dst->v = v	
	fmov.s  F_U1,@-OUT ! LS, dst->u = u
	fmov.s  F_W ,@-OUT ! LS, dst->w = invW
	fmov.s  F_Y1,@-OUT ! LS, dst->y = y	
	fmov.s  F_X1,@-OUT ! LS, dst->x = x

	fschg              ! FE (swap to 64 bit FPU loads/stores)
	rts                ! CO, return after executing instruction in delay slot
	pref   @OUT        ! LS, trigger store queue flush
.size _ClipEdge, .-_ClipEdge
.type _ClipEdge, %function

.align 4
_NEAR_CLIP_W:
        .float 0
.global _NEAR_CLIP_W
