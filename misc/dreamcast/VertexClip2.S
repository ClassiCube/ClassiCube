! FR0  = 0
! FR1  = 0
! FR2  = A.1
! FR3  = B.1
! FR4  = 0
! FR5  = 0
! FR6  = A.2
! FR7  = B.2
! FR8  = 0
! FR9  = 0
! FR10 = invT
! FR11 = t

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

! Writes output vertex as the near plane intersection point between two points:
!    float t  = fabsf(v1->z) / fabsf(v2->z - v1->z)
!    float invt = 1.0f - t;
!    // note: w = invt * v1->w + t * v2->w;, always ends up being zNear
!    
!    out->c = type << 24
!    out->x = (invt * v1->x + t * v2->x) * 1/zNear
!    out->y = (invt * v1->y + t * v2->y) * 1/zNear
!    out->w = 1/zNear  
!    
!    out->u = invt * v1->u + t * v2->u;
!    out->v = invt * v1->v + t * v2->v;
!    
!    out->b = invt * v1->b + t * v2->b;
!    out->g = invt * v1->g + t * v2->g;
!    out->r = invt * v1->r + t * v2->r;
!    out->a = invt * v1->a + t * v2->a;
! To optimise these calculations, FIPR is used:
!   FIPR = FVm.x*FVn.x + FVm.y*FVn.x + FVm.z*FVn.z + FVm.w*FVn.w --> FVn.w
! FIPR can be used to accomplish "vout->Q = invt * v1->Q + t * v2->Q" by:
!   - assigning x/y components to 0 for both vectors
!   - assigning t and invT to z/w of FVm vector
!   - assigning v1 and v2  to z/w  of FVn vector
!   FIPR = 0*0 + 0*0 + t*v1->Q + invT*v2->Q --> FVn.w
!   FIPR = t*v1->Q + invT*v2->Q --> FVn.w

.global _ClipEdge
.align 4
_ClipEdge:
	fschg             ! FE (swap to 32 bit FPU loads/stores)
	add      #28, IN1 ! EX, IN1  = &v1->z
	fldi0    fr4      ! LS, fr4  = 0
	fmov.s  @IN1, fr2 ! LS, fr2  = v1->z
	add      #28, IN2 ! EX, IN   = &v2->z
	fldi0    fr5      ! LS, fr5  = 0
	fmov.s  @IN2,fr11 ! LS, fr11 = v2->z
	fsub     fr2,fr11 ! FE, fr11 = v2->z - v1->z
	fldi0    fr8      ! LS, fr8  = 0
	shll16   TYP      ! EX, TYP <<= 16
	fmul    fr11,fr11 ! FE, fr11 = (v2->z - v1->z) * (v2->z * v1->z)
	fldi0    fr9      ! LS, fr9  = 0
	fldi0    fr0      ! LS, fr0  = 0
	fldi0    fr1      ! LS, fr1  = 0
	fsrra   fr11      ! FE, fr11 = 1 / abs(v2->z - v1->z)
	shll8    TYP      ! EX, TYP <<= 8
	fabs     fr2      ! LS, fr2  = abs(v1->z)
	mov.l  TYP,@OUT   ! LS, dst->cmd = TYPE
	fmul     fr2,fr11 ! FE, fr11 = abs(v1->Z) / abs(v2->z - v1->z)  --> t
	add     #-24, IN1 ! EX, IN1  = &v1->x
	fldi1   fr10      ! LS, fr10 = 1
	add     #-24, IN2 ! EX, IN2  = &v2->x
	add       #4, OUT ! EX, OUT  = &dst->x
	fsub    fr11,fr10 ! FE, invT = 1.0 - t  --> invT

! Load X components	
	fmov.s @IN1+, fr2 ! LS, A1 = v1->x, IN1 = &v1->y
	fmov.s @IN2+, fr3 ! LS, B1 = v2->x, IN2 = &v2->y

! Start interpolating X
	fipr     fv8, fv0 ! FE, LERP(A1, B1)
! Load Y components
	fmov.s  @IN1, fr6 ! LS, A2 = v1->y
	fmov.s  @IN2, fr7 ! LS, B2 = v2->y
! Load W
	mov.l _NEAR_CLIP_W,TM1 ! tmp = zNear
	lds 	 TM1,fpul ! LS, FPUL = zNear
	fsts     fpul,fr2 ! LS, fr2  = FPUL
! Store interpolated X
	fmul	 fr2,fr3  ! EX, fr7 = LERP * invW
	fmov.s   fr3,@OUT ! LS, dst->x = LERP * invW
	add       #4, OUT ! EX, OUT = &dst->y

! Start interpolating Y
	fipr     fv8, fv4 ! FE, LERP(A2, B2)
! Skip Z of input vertices
	add       #8, IN1 ! EX, IN1 = &v1->u
	add       #8, IN2 ! EX, IN2 = &v2->u

! Store interpolated Y
	fmul	 fr2,fr7  ! EX, fr7 = LERP * invW
	fmov.s   fr7,@OUT ! LS, OUT->y = LERP * invW
	add       #4, OUT ! EX, OUT = &dst->w
! Store W
	fmov.s   fr2,@OUT ! LS, OUT->w = 1/zNear
	add       #4, OUT ! EX, OUT = &dst->u
	
! Load U components
	fmov.s @IN1+, fr2 ! LS, A1 = v1->u, IN1 = &v1->v
	fmov.s @IN2+, fr3 ! LS, B1 = v2->u, IN2 = &v1->v

! Start interpolating U
	fipr     fv8, fv0 ! FE, LERP(A1, B1)
! Load V components
	fmov.s @IN1+, fr6 ! LS, A2 = v1->v, IN1 = &v1->bgra
	fmov.s @IN2+, fr7 ! LS, B2 = v2->v, IN2 = &v2->bgra
! Store interpolated U
	fmov.s   fr3,@OUT ! LS, dst->u = LERP
	add       #4, OUT ! EX, OUT = &dst->v

! Start interpolating V
	fipr     fv8, fv4 ! FE, LERP(A2, B2)
! Load colours and check if equal
	mov.l  @IN1,CL1   ! LS, ACOLOR = v1->bgra
	mov.l  @IN2,CL2   ! LS, BCOLOR = v2->bgra
	cmp/eq  CL1,CL2   ! MT, T = ACOLOR == BCOLOR
! Store V
	fmov.s   fr7,@OUT ! LS, dst->v = LERP
	add       #4, OUT ! EX, OUT = &dst->bgra
	
! Bypass RGBA interpolation if unnecessary
	bt.s    1f        ! BR, if (T) goto 1;
	mov     CL1,CLO   ! MT, OUTCOLOR = ACOLOR (branch delay instruction)

! Interpolate B
	extu.b  CL1,TM1   ! EX, val = ACOLOR.b
	lds     TM1,fpul  ! LS, FPUL = val
	float   fpul,fr2  ! FE, fr2 = float(FPUL)
	extu.b  CL2,TM1   ! EX, val = BCOLOR.b
	lds     TM1,fpul  ! LS, FPUL = val
	float   fpul,fr3  ! FE, fr3 = float(FPUL)
	fipr    fv8, fv0  ! FE, LERP(A1, B1)
	shlr8   CL1       ! EX, ACOLOR >>= 8
	shlr8   CL2       ! EX, BCOLOR >>= 8
	ftrc    fr3,fpul  ! FE, FPUL = int(lerp)
	sts     fpul,TM2  ! LS, tmp = FPUL

! Interpolate G
	extu.b  CL1,TM1   ! EX, val = ACOLOR.g
	lds     TM1,fpul  ! LS, FPUL = val
	float   fpul,fr2  ! FE, fr2 = float(FPUL)
	extu.b  CL2,TM1   ! EX, val = BCOLOR.g
	lds     TM1,fpul  ! LS, FPUL = val
	float   fpul,fr3  ! FE, fr3 = float(FPUL)
	fipr    fv8, fv0  ! FE, LERP(A1, B1)
	shlr8   CL1       ! EX, ACOLOR >>= 8
	extu.b  TM2,TM2   ! EX, tmp = (uint8)tmp
	mov     TM2,CLO   ! MT, OUTCOLOR.b = tmp
	shlr8   CL2       ! EX, BCOLOR >>= 8
	ftrc    fr3,fpul  ! FE, FPUL = int(lerp)
	sts     fpul,TM2  ! LS, tmp = FPUL

! Interpolate R
	extu.b  CL1,TM1   ! EX, val = ACOLOR.r
	lds     TM1,fpul  ! LS, FPUL = val
	float   fpul,fr2  ! FE, fr2 = float(FPUL)
	extu.b  CL2,TM1   ! EX, val = BCOLOR.r
	lds     TM1,fpul  ! LS, FPUL = val
	float   fpul,fr3  ! FE, fr3 = float(FPUL)
	fipr    fv8, fv0  ! FE, LERP(A1, B1)
	shlr8   CL1       ! EX, ACOLOR >>= 8
	extu.b  TM2,TM2   ! EX, tmp = (uint8)tmp
	shll8   TM2       ! EX, tmp <<= 8
	or      TM2,CLO   ! EX, OUTCOLOR.g |= tmp
	shlr8   CL2       ! EX, BCOLOR >>= 8
	ftrc    fr3,fpul  ! FE, FPUL = int(lerp)
	sts     fpul,TM2  ! LS, tmp = FPUL
	
! Interpolate A
	extu.b  CL1,TM1   ! EX, val = ACOLOR.a
	lds     TM1,fpul  ! LS, FPUL = val
	float   fpul,fr2  ! FE, fr2 = float(FPUL)
	extu.b  CL2,TM1   ! EX, val = BCOLOR.a
	lds     TM1,fpul  ! LS, FPUL = val
	float   fpul,fr3  ! FE, fr3 = float(FPUL)
	fipr    fv8, fv0  ! FE, LERP(A1, B1)
	extu.b  TM2,TM2   ! EX, tmp = (uint8)tmp
	shll16  TM2       ! EX, tmp <<= 16
	or      TM2,CLO   ! EX, OUTCOLOR.r |= tmp
	ftrc    fr3,fpul  ! FE, FPUL = int(lerp)
	sts     fpul,TM2  ! LS, tmp = FPUL
	extu.b  TM2,TM2   ! EX, tmp = (uint8)tmp
	shll16  TM2       ! EX, tmp <<= 16
	shll8   TM2       ! EX, tmp <<= 8
	or      TM2,CLO   ! EX, OUTCOLOR.a |= tmp

1:
	mov.l  CLO,@OUT   ! LS, OUT->color = OUTCOLOR
	add   #-24, OUT   ! EX, OUT += 8
	fschg             ! FE (swap to 64 bit FPU loads/stores)
	rts               ! CO, return after executing instruction in delay slot
	pref   @OUT       ! LS, trigger store queue flush
.size _ClipEdge, .-_ClipEdge
.type _ClipEdge, %function

.align 4
_NEAR_CLIP_W:
        .float 0
.global _NEAR_CLIP_W
