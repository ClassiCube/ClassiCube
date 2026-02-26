/* See CONTEXT in WinNT.h */
void CrashHandler_DumpRegisters(void* ctx, cc_string* str) {
	CONTEXT* r = (CONTEXT*)ctx;
#if defined _M_IX86
	#define RREG(reg) &r->reg

	String_Format3(str, "eax=%x ebx=%x ecx=%x" _NL, RREG(Eax), RREG(Ebx), RREG(Ecx));
	String_Format3(str, "edx=%x esi=%x edi=%x" _NL, RREG(Edx), RREG(Esi), RREG(Edi));
	String_Format3(str, "eip=%x ebp=%x esp=%x" _NL, RREG(Eip), RREG(Ebp), RREG(Esp));
#elif defined _M_X64
	#define RREG(reg) &r->reg

	String_Format3(str, "rax=%x rbx=%x rcx=%x" _NL, RREG(Rax), RREG(Rbx), RREG(Rcx));
	String_Format3(str, "rdx=%x rsi=%x rdi=%x" _NL, RREG(Rdx), RREG(Rsi), RREG(Rdi));
	String_Format3(str, "rip=%x rbp=%x rsp=%x" _NL, RREG(Rip), RREG(Rbp), RREG(Rsp));
	String_Format3(str, "r8 =%x r9 =%x r10=%x" _NL, RREG(R8),  RREG(R9),  RREG(R10));
	String_Format3(str, "r11=%x r12=%x r13=%x" _NL, RREG(R11), RREG(R12), RREG(R13));
	String_Format2(str, "r14=%x r15=%x" _NL,        RREG(R14), RREG(R15));
#elif defined _M_ARM64
	#define RREG(reg) &r->reg
	#define RNUM(reg) &r->X[reg]

	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, RNUM(0),  RNUM(1),  RNUM(2),  RNUM(3));
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, RNUM(4),  RNUM(5),  RNUM(6),  RNUM(7));
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, RNUM(8),  RNUM(9),  RNUM(10), RNUM(11));
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, RNUM(12), RNUM(13), RNUM(14), RNUM(15));
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, RNUM(16), RNUM(17), RNUM(18), RNUM(19));
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, RNUM(20), RNUM(21), RNUM(22), RNUM(23));
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, RNUM(24), RNUM(25), RNUM(26), RNUM(27));
	String_Format4(str, "r28=%x fp =%x lr =%x sp =%x" _NL, RNUM(28), RREG(Fp), RREG(Lr), RREG(Sp));
	String_Format1(str, "pc =%x" _NL,                      RREG(Pc));
#elif defined _M_ARM
	#define RREG(reg) &r->reg
	#define RNUM(reg) &r->R##reg

	String_Format3(str, "r0 =%x r1 =%x r2 =%x" _NL, RNUM(0),  RNUM(1),  RNUM(2));
	String_Format3(str, "r3 =%x r4 =%x r5 =%x" _NL, RNUM(3),  RNUM(4),  RNUM(5));
	String_Format3(str, "r6 =%x r7 =%x r8 =%x" _NL, RNUM(6),  RNUM(7),  RNUM(8));
	String_Format3(str, "r9 =%x r10=%x r11=%x" _NL, RNUM(9),  RNUM(10), RNUM(11));
	String_Format3(str, "r12=%x sp =%x lr =%x" _NL, RNUM(12), RREG(Sp), RREG(Lr));
	String_Format1(str, "pc =%x"               _NL, RREG(Pc));
#elif defined _M_PPC
	#define RREG(reg) &r->reg
	#define RNUM(reg) &r->Gpr##reg

	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, RNUM(0),   RNUM(1),  RNUM(2),  RNUM(3));
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, RNUM(4),   RNUM(5),  RNUM(6),  RNUM(7));
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, RNUM(8),   RNUM(9),  RNUM(10), RNUM(11));
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, RNUM(12),  RNUM(13), RNUM(14), RNUM(15));
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, RNUM(16),  RNUM(17), RNUM(18), RNUM(19));
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, RNUM(20),  RNUM(21), RNUM(22), RNUM(23));
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, RNUM(24),  RNUM(25), RNUM(26), RNUM(27));
	String_Format4(str, "r28=%x r29=%x r30=%x r31=%x" _NL, RNUM(28),  RNUM(29), RNUM(30), RNUM(31));
	String_Format3(str, "pc =%x lr =%x ctr=%x" _NL,        RREG(Iar), RREG(Lr), RREG(Ctr));
#elif defined _M_ALPHA
	#define RREG(reg) &r->Int##reg

	String_Format4(str, "v0 =%x t0 =%x t1 =%x t2 =%x" _NL, RREG(V0),  RREG(T0), RREG(T1),  RREG(T2));
	String_Format4(str, "t3 =%x t4 =%x t5 =%x t6 =%x" _NL, RREG(T3),  RREG(T4), RREG(T5),  RREG(T6));
	String_Format4(str, "t7 =%x s0 =%x s1 =%x s2 =%x" _NL, RREG(T7),  RREG(S0), RREG(S1),  RREG(S2));
	String_Format4(str, "s3 =%x s4 =%x s5 =%x a0 =%x" _NL, RREG(S3),  RREG(S4), RREG(S5),  RREG(A0));
	String_Format4(str, "a1 =%x a2 =%x a3 =%x a4 =%x" _NL, RREG(A1),  RREG(A2), RREG(A3),  RREG(A4));
	String_Format4(str, "a5 =%x t8 =%x t9 =%x t10=%x" _NL, RREG(A5),  RREG(T8), RREG(T9),  RREG(T10));
	String_Format4(str, "t11=%x ra =%x t12=%x at =%x" _NL, RREG(T11), RREG(Ra), RREG(T12), RREG(At));
	String_Format4(str, "gp =%x fp =%x sp =%x pc =%x" _NL, RREG(Gp),  RREG(Fp), RREG(Sp),  &r->Fir);
#elif defined _MIPS_
	#define RREG(reg) &r->Int##reg

	String_Format4(str, "at =%x v0 =%x v1 =%x a0 =%x" _NL, RREG(At), RREG(V0), RREG(V1), RREG(A0));
	String_Format4(str, "a1 =%x a2 =%x a3 =%x t0 =%x" _NL, RREG(A1), RREG(A2), RREG(A3), RREG(T0)); 
	String_Format4(str, "t1 =%x t2 =%x t3 =%x t4 =%x" _NL, RREG(T1), RREG(T2), RREG(T3), RREG(T4));
	String_Format4(str, "t5 =%x t6 =%x t7 =%x s0 =%x" _NL, RREG(T5), RREG(T6), RREG(T7), RREG(S0));
	String_Format4(str, "s1 =%x s2 =%x s3 =%x s4 =%x" _NL, RREG(S1), RREG(S2), RREG(S3), RREG(S4));
	String_Format4(str, "s5 =%x s6 =%x s7 =%x t8 =%x" _NL, RREG(S5), RREG(S6), RREG(S7), RREG(T8));
	String_Format4(str, "t9 =%x ko =%x k1 =%x gp =%x" _NL, RREG(T9), RREG(K0), RREG(K1), RREG(Gp));
	String_Format4(str, "sp =%x s8 =%x ra =%x pc =%x" _NL, RREG(Sp), RREG(S8), RREG(Ra), &r->Fir));
	String_Format3(str, "lo =%x hi =%x"               _NL, RREG(Lo), RREG(Hi));
#elif defined SHx
	#define RREG(reg) &r->reg
	#define RNUM(reg) &r->R##reg

	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, RNUM(0),    RNUM(1),    RNUM(2),  RNUM(3));
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, RNUM(4),    RNUM(5),    RNUM(6),  RNUM(7));
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, RNUM(8),    RNUM(9),    RNUM(10), RNUM(11));
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, RNUM(12),   RNUM(13),   RNUM(14), RNUM(15));
	String_Format3(str, "mal=%x mah=%x gbr=%x"        _NL, RREG(MACL), RREG(MACH), RREG(Gbr));
	String_Format2(str, "pc =%x ra =%x"               _NL, RREG(Fir),  RREG(PR));
#else
	#error "Unknown CPU architecture"
#endif
}
