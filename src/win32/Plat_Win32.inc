/* See CONTEXT in WinNT.h */
void CrashHandler_DumpRegisters(void* ctx, cc_string* str) {
	CONTEXT* r = (CONTEXT*)ctx;

#if defined _M_IX86
	String_Format4(str, "eax=%x ebx=%x ecx=%x edx=%x" _NL, &r->Eax, &r->Ebx, &r->Ecx, &r->Edx);
	String_Format4(str, "esi=%x edi=%x ebp=%x esp=%x" _NL, &r->Esi, &r->Edi, &r->Ebp, &r->Esp);
	String_Format1(str, "eip=%x"                      _NL, &r->Eip);
#elif defined _M_X64
	String_Format4(str, "rax=%x rbx=%x rcx=%x rdx=%x" _NL, &r->Rax, &r->Rbx, &r->Rcx, &r->Rdx);
	String_Format4(str, "rsi=%x rdi=%x rbp=%x rsp=%x" _NL, &r->Rsi, &r->Rdi, &r->Rbp, &r->Rsp);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->R8,  &r->R9,  &r->R10, &r->R11);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->R12, &r->R13, &r->R14, &r->R15);
	String_Format1(str, "rip=%x"                      _NL, &r->Rip);
#elif defined _M_ARM64
	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->X0,  &r->X1,  &r->X2,  &r->X3);
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->X4,  &r->X5,  &r->X6,  &r->X7);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->X8,  &r->X9,  &r->X10, &r->X11);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->X12, &r->X13, &r->X14, &r->X15);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->X16, &r->X17, &r->X18, &r->X19);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->X20, &r->X21, &r->X22, &r->X23);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->X24, &r->X25, &r->X26, &r->X27);
	String_Format4(str, "r28=%x fp =%x lr =%x sp =%x" _NL, &r->X28, &r->Fp,  &r->Lr,  &r->Sp);
	String_Format1(str, "pc =%x"                      _NL, &r->Pc);
#elif defined _M_ARM
	String_Format3(str, "r0 =%x r1 =%x r2 =%x" _NL, &r->R0,  &r->R1,  &r->R2);
	String_Format3(str, "r3 =%x r4 =%x r5 =%x" _NL, &r->R3,  &r->R4,  &r->R5);
	String_Format3(str, "r6 =%x r7 =%x r8 =%x" _NL, &r->R6,  &r->R7,  &r->R8);
	String_Format3(str, "r9 =%x r10=%x r11=%x" _NL, &r->R9,  &r->R10, &r->R11);
	String_Format3(str, "r12=%x sp =%x lr =%x" _NL, &r->R12, &r->Sp,  &r->Lr);
	String_Format1(str, "pc =%x"               _NL, &r->Pc);
#elif defined _M_PPC
	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->Gpr0,  &r->Gpr1,  &r->Gpr2,  &r->Gpr3);
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->Gpr4,  &r->Gpr5,  &r->Gpr6,  &r->Gpr7);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->Gpr8,  &r->Gpr9,  &r->Gpr10, &r->Gpr11);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->Gpr12, &r->Gpr13, &r->Gpr14, &r->Gpr15);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->Gpr16, &r->Gpr17, &r->Gpr18, &r->Gpr19);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->Gpr20, &r->Gpr21, &r->Gpr22, &r->Gpr23);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->Gpr24, &r->Gpr25, &r->Gpr26, &r->Gpr27);
	String_Format4(str, "r28=%x r29=%x r30=%x r31=%x" _NL, &r->Gpr28, &r->Gpr29, &r->Gpr30, &r->Gpr31);
	String_Format3(str, "pc =%x lr =%x ctr=%x"        _NL, &r->Iar,   &r->Lr,    &r->Ctr);
#elif defined _M_ALPHA
	String_Format4(str, "v0 =%x t0 =%x t1 =%x t2 =%x" _NL, &r->IntV0, &r->IntT0, &r->IntT1, &r->IntT2);
	String_Format4(str, "t3 =%x t4 =%x t5 =%x t6 =%x" _NL, &r->IntT3, &r->IntT4, &r->IntT5, &r->IntT6);
	String_Format4(str, "t7 =%x s0 =%x s1 =%x s2 =%x" _NL, &r->IntT7, &r->IntS0, &r->IntS1, &r->IntS2);
	String_Format4(str, "s3 =%x s4 =%x s5 =%x a0 =%x" _NL, &r->IntS3, &r->IntS4, &r->IntS5, &r->IntA0);
	String_Format4(str, "a1 =%x a2 =%x a3 =%x a4 =%x" _NL, &r->IntA1, &r->IntA2, &r->IntA3, &r->IntA4);
	String_Format4(str, "a5 =%x t8 =%x t9 =%x t10=%x" _NL, &r->IntA5, &r->IntT8, &r->IntT9, &r->IntT10);
	String_Format4(str, "t11=%x ra =%x t12=%x at =%x" _NL, &r->IntT11,&r->IntRa, &r->IntT12,&r->IntAt);
	String_Format4(str, "gp =%x fp =%x sp =%x pc =%x" _NL, &r->IntGp, &r->IntFp, &r->IntSp, &r->Fir);
#elif defined _MIPS_
	String_Format4(str, "at =%x v0 =%x v1 =%x a0 =%x" _NL, &r->IntAt, &r->IntV0, &r->IntV1, &r->IntA0);
	String_Format4(str, "a1 =%x a2 =%x a3 =%x t0 =%x" _NL, &r->IntA1, &r->IntA2, &r->IntA3, &r->IntT0); 
	String_Format4(str, "t1 =%x t2 =%x t3 =%x t4 =%x" _NL, &r->IntT1, &r->IntT2, &r->IntT3, &r->IntT4);
	String_Format4(str, "t5 =%x t6 =%x t7 =%x s0 =%x" _NL, &r->IntT5, &r->IntT6, &r->IntT7, &r->IntS0);
	String_Format4(str, "s1 =%x s2 =%x s3 =%x s4 =%x" _NL, &r->IntS1, &r->IntS2, &r->IntS3, &r->IntS4);
	String_Format4(str, "s5 =%x s6 =%x s7 =%x t8 =%x" _NL, &r->IntS5, &r->IntS6, &r->IntS7, &r->IntT8);
	String_Format4(str, "t9 =%x ko =%x k1 =%x gp =%x" _NL, &r->IntT9, &r->IntK0, &r->IntK1, &r->IntGp);
	String_Format4(str, "sp =%x s8 =%x ra =%x pc =%x" _NL, &r->IntSp, &r->IntS8, &r->IntRa, &r->Fir);
	String_Format2(str, "lo =%x hi =%x"               _NL, &r->IntLo, &r->IntHi);
#elif defined SHx
	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->R0,   &r->R1,   &r->R2,  &r->R3);
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->R4,   &r->R5,   &r->R6,  &r->R7);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->R8,   &r->R9,   &r->R10, &r->R11);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->R12,  &r->R13,  &r->R14, &r->R15);
	String_Format3(str, "mal=%x mah=%x gbr=%x"        _NL, &r->MACL, &r->MACH, &r->Gbr);
	String_Format2(str, "pc =%x ra =%x"               _NL, &r->Fir,  &r->PR);
#else
	#error "Unknown CPU architecture"
#endif
}
