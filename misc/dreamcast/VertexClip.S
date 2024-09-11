! r1 = v1,    CLOBBERS
! r2 = v2,    CLOBBERS
! r3 = OUT,   CLOBBERS
! r4 = TMP 1, preserved
! r5 = TMP 2, preserved
! r0 = CL0,   CLOBBERS

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

#define TM1 r0 // CLOBBERED, temp register 1
#define TM2 r4 // PRESERVED, temp register 2
#define CLO r5 // PRESERVED, output colour

#define IN1 r1 // CLOBBERED, input vertex 1
#define IN2 r2 // CLOBBERED, input vertex 2
#define OUT r3 // CLOBBERED, output vertex

#define CL1 r1 // CLOBBERED, input colour 1
#define CL2 r2 // CLOBBERED, input colour 2

! Calculates the near plane intersection point between two points:
!    float t  = fabsf(v1->z) / fabsf(v2->z - v1->z)
!    float invt = 1.0f - t;
!    
!    out->x = invt * v1->x + t * v2->x;
!    out->y = invt * v1->y + t * v2->y;
!    out->z = 0.0f; // clipped against near plane anyways (I.e Z/W = 0 --> Z = 0)
!    
!    out->u = invt * v1->u + t * v2->u;
!    out->v = invt * v1->v + t * v2->v;
!    out->w = invt * v1->w + t * v2->w;
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

.global _ClipLine
.align 4
_ClipLine:
	mov.l    r4,@-r15 ! LS, push(r4)
	mov.l    r5,@-r15 ! LS, push(r5)
	mov      IN1, TM1 ! MT, tmp  = &v1
	fldi0    fr4      ! LS, fr4  = 0
	add      #12, TM1 ! EX, tmp  = &v1->z
	fmov.s  @TM1, fr2 ! LS, fr2  = v1->z	
	mov      IN2, TM1 ! MT, tmp  = &v2
	fldi0    fr5      ! LS, fr5  = 0
	add      #12, TM1 ! EX, tmp   = &v2->z
	fmov.s  @TM1,fr11 ! LS, fr11 = v2->z
	fsub     fr2,fr11 ! FE, fr11 = v2->z - v1->z
	fldi0    fr8      ! LS, fr8  = 0
	fmul    fr11,fr11 ! FE, fr11 = (v2->z - v1->z) * (v2->z * v1->z)
	fldi0    fr9      ! LS, fr9  = 0
	fldi0    fr0      ! LS, fr0  = 0
	fldi0    fr1      ! LS, fr1  = 0
	fsrra   fr11      ! FE, fr11 = 1 / abs(v2->z - v1->z)
	fabs     fr2      ! LS, fr2  = abs(v1->z)
	fmul     fr2,fr11 ! FE, fr11 = abs(v1->Z) / abs(v2->z - v1->z)  --> t
	add       #4, IN1 ! EX, v1   += 4
	fldi1   fr10      ! LS, fr10 = 1
	add       #4, IN2 ! EX, v2   += 4
	add       #4, OUT ! EX, OUT  += 4
	fsub    fr11,fr10 ! FE, fr10 = 1.0 - t  --> invT
	
	fmov.s @IN1+, fr2 ! LS, A1 = v1->x, v1 += 4
	fmov.s @IN2+, fr3 ! LS, B1 = v2->x, v2 += 4
	fipr     fv8, fv0 ! FE, LERP(A1, B1)
	fmov.s @IN1+, fr6 ! LS, A2 = v1->y, v1 += 4
	fmov.s @IN2+, fr7 ! LS, B2 = v2->y, v2 += 4

	fmov.s   fr3,@OUT ! LS, OUT->x = LERP
	add       #4, OUT ! EX, OUT += 4
	fipr     fv8, fv4 ! FE, LERP(A2, B2)
	add       #4, IN1 ! EX, v1 += 4
	add       #4, IN2 ! EX, v2 += 4

	fmov.s   fr7,@OUT ! LS, OUT->y = LERP
	add       #4, OUT ! EX, OUT += 4
	fmov.s   fr1,@OUT ! LS, OUT->z = 0
	add       #4, OUT ! EX, OUT += 4
	
	fmov.s @IN1+, fr2 ! LS, A1 = v1->u, v1 += 4
	fmov.s @IN2+, fr3 ! LS, B1 = v2->u, v2 += 4
	fipr     fv8, fv0 ! FE, LERP(A1, B1)
	fmov.s @IN1+, fr6 ! LS, A2 = v1->v, v1 += 4
	fmov.s @IN2+, fr7 ! LS, B2 = v2->v, v2 += 4

	fmov.s   fr3,@OUT ! LS, OUT->u = LERP
	add       #4, OUT ! EX, OUT += 4
	fipr     fv8, fv4 ! FE, LERP(A2, B2)
	add       #4, IN1 ! EX, v1 += 4
	add       #4, IN2 ! EX, v2 += 4
	fmov.s  @IN1,fr2  ! LS, A1 = v1->w
	fmov.s  @IN2,fr3  ! LS, B1 = v2->w
	fmov.s   fr7,@OUT ! LS, OUT->v = LERP
	add       #8, OUT ! EX, OUT += 8
	
	fipr     fv8, fv0 ! FE, LERP(A1, B1)
	add      #-4, IN1 ! EX, v1 -= 4
	add      #-4, IN2 ! EX, v2 -= 4
	fmov.s   fr3,@OUT ! LS, OUT->w = lerp
	add      #-4, OUT ! EX, OUT -= 4
	
	mov.l  @IN1,CL1   ! LS, ACOLOR = v1->bgra
	mov.l  @IN2,CL2   ! LS, BCOLOR = v2->bgra
! Bypass interpolation if unnecessary
	cmp/eq  CL1,CL2   ! MT, T = ACOLOR == BCOLOR
	bt.s    1f        ! BR, if (T) goto 1;
	mov     CL1,CLO   ! MT, OUTCOLOR = ACOLOR (branch delay instruction)
! Interpolate B
	extu.b  CL1,TM1   ! EX, val = ACOLOR.b
	lds     TM1,fpul  ! CO, FPUL = val
	float   fpul,fr2  ! EX, fr2 = float(FPUL)
	extu.b  CL2,TM1   ! EX, val = BCOLOR.b
	lds     TM1,fpul  ! CO, FPUL = val
	float   fpul,fr3  ! EX, fr3 = float(FPUL)
	fipr    fv8, fv0  ! FE, LERP(A1, B1)
	shlr8   CL1       ! EX, ACOLOR >>= 8
	ftrc    fr3,fpul  ! FE, FPUL = int(lerp)
	shlr8   CL2       ! EX, BCOLOR >>= 8
	sts     fpul,TM2  ! CO, tmp = FPUL
! Interpolate G
	extu.b  CL1,TM1   ! EX, val = ACOLOR.g
	lds     TM1,fpul  ! CO, FPUL = val
	float   fpul,fr2  ! EX, fr2 = float(FPUL)
	extu.b  CL2,TM1   ! EX, val = BCOLOR.g
	lds     TM1,fpul  ! CO, FPUL = val
	float   fpul,fr3  ! EX, fr3 = float(FPUL)
	fipr    fv8, fv0  ! FE, LERP(A1, B1)
	shlr8   CL1       ! EX, ACOLOR >>= 8
	ftrc    fr3,fpul  ! FE, FPUL = int(lerp)
	extu.b  TM2,TM2   ! EX, tmp = (uint8)tmp
	mov     TM2,CLO   ! MT, OUTCOLOR.b = tmp
	shlr8   CL2       ! EX, BCOLOR >>= 8
	sts     fpul,TM2  ! CO, tmp = FPUL
! Interpolate R
	extu.b  CL1,TM1   ! EX, val = ACOLOR.r
	lds     TM1,fpul  ! CO, FPUL = val
	float   fpul,fr2  ! EX, fr2 = float(FPUL)
	extu.b  CL2,TM1   ! EX, val = BCOLOR.r
	lds     TM1,fpul  ! CO, FPUL = val
	float   fpul,fr3  ! EX, fr3 = float(FPUL)
	fipr    fv8, fv0  ! FE, LERP(A1, B1)
	shlr8   CL1       ! EX, ACOLOR >>= 8
	ftrc    fr3,fpul  ! FE, FPUL = int(lerp)
	extu.b  TM2,TM2   ! EX, tmp = (uint8)tmp
	shll8   TM2       ! EX, tmp <<= 8
	or      TM2,CLO   ! EX, OUTCOLOR.g |= tmp
	shlr8   CL2       ! EX, BCOLOR >>= 8
	sts     fpul,TM2  ! CO, tmp = FPUL
! Interpolate A
	extu.b  CL1,TM1   ! EX, val = ACOLOR.a
	lds     TM1,fpul  ! CO, FPUL = val
	float   fpul,fr2  ! EX, fr2 = float(FPUL)
	extu.b  CL2,TM1   ! EX, val = BCOLOR.a
	lds     TM1,fpul  ! CO, FPUL = val
	float   fpul,fr3  ! EX, fr3 = float(FPUL)
	fipr    fv8, fv0  ! FE, LERP(A1, B1)
	ftrc    fr3,fpul  ! FE, FPUL = int(lerp)
	extu.b  TM2,TM2   ! EX, tmp = (uint8)tmp
	shll16  TM2       ! EX, tmp <<= 16
	or      TM2,CLO   ! EX, OUTCOLOR.r |= tmp
	sts     fpul,TM2  ! CO, tmp = FPUL
	extu.b  TM2,TM2   ! EX, tmp = (uint8)tmp
	shll16  TM2       ! EX, tmp <<= 16
	shll8   TM2       ! EX, tmp <<= 8
	or      TM2,CLO   ! EX, OUTCOLOR.a |= tmp

1:
	mov.l  CLO,@OUT   ! LS, OUT->color = OUTCOLOR
	mov.l   @r15+,r5  ! LS, pop(r5)
	rts               ! CO, return after executing instruction in delay slot
	mov.l   @r15+,r4  ! LS, pop(r4)

.size _ClipLine, .-_ClipLine
.type _ClipLine,%function
