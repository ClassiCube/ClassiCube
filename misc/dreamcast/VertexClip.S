! r0 CLOBBERS
! r1 CLOBBERS
! r2 CLOBBERS
! r3 CLOBBERS
! r4 = v1
! r5 = v2
! r6 = OUT

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
! FIPR can be used to accomplish "vout->Q invt * v1->Q + t * v2->Q" by:
!   - assigning x/y components to 0 for both vectors
!   - assigning t and invT to z/w of FVm vector
!   - assigning v1 and v2  to z/w  of FVn vector
!   FIPR = 0*0 + 0*0 + t*v1->Q + invT*v2->Q --> FVn.w
!   FIPR = t*v1->Q + invT*v2->Q --> FVn.w

.global _ClipLine
.align 4
.type  _ClipLine,%function
_ClipLine:
	mov       r4,  r1 ! MT, r1   = &v1
	fldi0    fr4      ! LS, fr4  = 0
	add      #12,  r1 ! EX, r1   = &v1->z
	fmov.s   @r1, fr2 ! LS, fr2  = v1->z	
	mov       r5,  r1 ! MT, r1   = &v2
	fldi0    fr5      ! LS, fr5  = 0
	add      #12,  r1 ! EX, r1   = &v2->z
	fmov.s   @r1,fr11 ! LS, fr11 = v2->z
	fsub     fr2,fr11 ! FE, fr11 = v2->z - v1->z
	fldi0    fr8      ! LS, fr8  = 0
	fmul    fr11,fr11 ! FE, fr11 = (v2->z - v1->z) * (v2->z * v1->z)
	fldi0    fr9      ! LS, fr9  = 0
	fldi0    fr0      ! LS, fr0  = 0
	fldi0    fr1      ! LS, fr1  = 0
	fsrra   fr11      ! FE, fr11 = 1 / abs(v2->z - v1->z)
	fabs     fr2      ! LS, fr2  = abs(v1->z)
	fmul     fr2,fr11 ! FE, fr11 = abs(v1->Z) / abs(v2->z - v1->z)  --> t
	add       #4,  r4 ! EX, A    = &v1->x
	fldi1   fr10      ! LS, fr10 = 1
	add       #4,  r5 ! EX, B    = &v2->x
	add       #4,  r6 ! EX, OUT  = &OUT->x
	fsub    fr11,fr10 ! FE, fr10 = 1.0 - t  --> invT
	
	fmov.s  @r4+, fr2 ! LS, A1 = v1->x
	fmov.s  @r5+, fr3 ! LS, B1 = v2->x
	fipr     fv8, fv0 ! FE, LERP(A1, B1)
	fmov.s  @r4+, fr6 ! LS, A2 = v1->y
	fmov.s  @r5+, fr7 ! LS, B2 = v2->y

	fmov.s   fr3, @r6 ! LS, OUT->x = LERP
	add       #4,  r6 ! EX, OUT += 4
	fipr     fv8, fv4 ! FE, LERP(A2, B2)
	add       #4,  r4 ! EX, v1 += 4
	add       #4,  r5 ! EX, v2 += 4

	fmov.s   fr7, @r6 ! LS, OUT->y = LERP
	add       #4,  r6 ! EX, OUT += 4
	fmov.s   fr1, @r6 ! LS, OUT->z = 0
	add       #4,  r6 ! EX, OUT += 4
	
	fmov.s  @r4+, fr2 ! LS, A1 = v1->u
	fmov.s  @r5+, fr3 ! LS, B1 = v2->u
	fipr     fv8, fv0 ! FE, LERP(A1, B1)
	fmov.s  @r4+, fr6 ! LS, A2 = v1->v
	fmov.s  @r5+, fr7 ! LS, B2 = v2->v

	fmov.s   fr3, @r6 ! LS, OUT->u = LERP
	add       #4,  r6 ! EX, OUT += 4
	fipr     fv8, fv4 ! FE, LERP(A2, B2)
	fmov.s   fr7, @r6 ! LS, OUT->v = LERP
	add       #4,  r6 ! EX, OUT += 4
	
	mov.l   @r4+,r0   ! LS, ACOLOR = v1->bgra
	extu.b  r0,r1     ! EX, tmp = ACOLOR.b
	lds     r1,fpul   ! CO, FPUL = tmp
	float   fpul,fr2  ! EX, fr2 = float(FPUL)
	mov.l   @r5+,r2   ! LS, BCOLOR = v2->bgra
	extu.b  r2,r3     ! EX, tmp = BCOLOR.b
	lds     r3,fpul   ! CO, FPUL = tmp
	float   fpul,fr3  ! EX, fr3 = float(FPUL)
	fipr    fv8, fv0  ! FE, LERP(A1, B1)
	ftrc    fr3,fpul  ! FE, FPUL = int(lerp)
	sts     fpul,r3   ! CO, tmp = FPUL
	extu.b  r3,r3     ! EX, tmp = (uint8)tmp
	mov     r3, r7    ! MT, OUTCOLOR.b = tmp
	
	shlr8   r0        ! EX, ACOLOR >>= 8
	extu.b  r0,r1     ! EX, tmp = ACOLOR.g
	lds     r1,fpul   ! CO, FPUL = tmp
	float   fpul,fr2  ! EX, fr2 = float(FPUL)
	shlr8   r2        ! EX, BCOLOR >>= 8
	extu.b  r2,r3     ! EX, tmp = BCOLOR.g
	lds     r3,fpul   ! CO, FPUL = tmp
	float   fpul,fr3  ! EX, fr3 = float(FPUL)
	fipr    fv8, fv0  ! FE, LERP(A1, B1)
	ftrc    fr3,fpul  ! FE, FPUL = int(lerp)
	sts     fpul,r3   ! CO, tmp = FPUL
	extu.b  r3,r3     ! EX, tmp = (uint8)tmp
	shll8   r3        ! EX, tmp <<= 8
	or      r3,r7     ! EX, OUTCOLOR.r |= tmp
	
	shlr8   r0        ! EX, ACOLOR >>= 8
	extu.b  r0,r1     ! EX, tmp = ACOLOR.b
	lds     r1,fpul   ! CO, FPUL = tmp
	float   fpul,fr2  ! EX, fr2 = float(FPUL)
	shlr8   r2        ! EX, BCOLOR >>= 8
	extu.b  r2,r3     ! EX, tmp = BCOLOR.b
	lds     r3,fpul   ! CO, FPUL = tmp
	float   fpul,fr3  ! EX, fr3 = float(FPUL)
	fipr    fv8, fv0  ! FE, LERP(A1, B1)
	ftrc    fr3,fpul  ! FE, FPUL = int(lerp)
	sts     fpul,r3   ! CO, tmp = FPUL
	extu.b  r3,r3     ! EX, tmp = (uint8)tmp
	shll16  r3        ! EX, tmp <<= 16
	or      r3,r7     ! EX, OUTCOLOR.g |= tmp
	
	shlr8   r0        ! EX, ACOLOR >>= 8
	extu.b  r0,r1     ! EX, tmp = ACOLOR.r
	lds     r1,fpul   ! CO, FPUL = tmp
	float   fpul,fr2  ! EX, fr2 = float(FPUL)
	shlr8   r2        ! EX, BCOLOR >>= 8
	extu.b  r2,r3     ! EX, tmp = BCOLOR.r
	lds     r3,fpul   ! CO, FPUL = tmp
	float   fpul,fr3  ! EX, fr3 = float(FPUL)
	fipr    fv8, fv0  ! FE, LERP(A1, B1)
	ftrc    fr3,fpul  ! FE, FPUL = int(lerp)
	sts     fpul,r3   ! CO, tmp = FPUL
	extu.b  r3,r3     ! EX, tmp = (uint8)tmp
	shll16  r3        ! EX, tmp <<= 16
	shll8   r3        ! EX, tmp <<= 8
	or      r3,r7     ! EX, OUTCOLOR.a |= tmp
	mov.l   r7, @r6   ! LS, OUT->color = OUTCOLOR
	
	fmov.s  @r4+,fr2 ! LS, A1 = v1->w
	fmov.s  @r5+,fr3 ! LS, B1 = v2->w
	fipr    fv8, fv0 ! FE, LERP(A1, B1)
	add      #4,  r6 ! EX, OUT += 4
	rts              ! CO, return after executing instruction in delay slot
	fmov.s  fr3, @r6 ! LS, OUT->w = lerp
