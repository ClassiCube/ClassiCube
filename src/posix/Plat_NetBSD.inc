/* See /usr/include/[ARCH]/mcontext.h */
/* -> src/sys/arch/[ARCH]/include/mcontext.h */
#include <signal.h>
#include <sys/ucontext.h>

#define RREG(reg) &r->__gregs[_REG_ ## reg]
#define RNUM(reg) &r->__gregs[reg]

void CrashHandler_DumpRegisters(void* ctx, cc_string* str) {
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;

#if defined __i386__
	/* src/sys/arch/i386/include/mcontext.h */
	String_Format3(str, "eax=%x ebx=%x ecx=%x" _NL, RREG(EAX), RREG(EBX), RREG(ECX));
	String_Format3(str, "edx=%x esi=%x edi=%x" _NL, RREG(EDX), RREG(ESI), RREG(EDI));
	String_Format3(str, "eip=%x ebp=%x esp=%x" _NL, RREG(EIP), RREG(EBP), RREG(ESP));
#elif defined __x86_64__
	/* src/sys/arch/amd64/include/mcontext.h */
	String_Format3(str, "rax=%x rbx=%x rcx=%x" _NL, RREG(RAX), RREG(RBX), RREG(RCX));
	String_Format3(str, "rdx=%x rsi=%x rdi=%x" _NL, RREG(RAX), RREG(RSI), RREG(RDI));
	String_Format3(str, "rip=%x rbp=%x rsp=%x" _NL, RREG(RIP), RREG(RBP), RREG(RSP));
	String_Format3(str, "r8 =%x r9 =%x r10=%x" _NL, RREG(R8),  RREG(R9),  RREG(R10));
	String_Format3(str, "r11=%x r12=%x r13=%x" _NL, RREG(R11), RREG(R12), RREG(R13));
	String_Format2(str, "r14=%x r15=%x" _NL,        RREG(R14), RREG(R15));
#elif defined __aarch64__
	/* src/sys/arch/arm/include/mcontext.h */
	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, RNUM(0),  RNUM(1),  RNUM(2),  RNUM(3));
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, RNUM(4),  RNUM(5),  RNUM(6),  RNUM(7));
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, RNUM(8),  RNUM(9),  RNUM(10), RNUM(11));
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, RNUM(12), RNUM(13), RNUM(14), RNUM(15));
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, RNUM(16), RNUM(17), RNUM(18), RNUM(19));
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, RNUM(20), RNUM(21), RNUM(22), RNUM(23));
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, RNUM(24), RNUM(25), RNUM(26), RNUM(27));
	String_Format4(str, "r28=%x fp =%x lr =%x sp =%x" _NL, RNUM(28), RREG(FP), RREG(LR), RREG(SP));
	String_Format1(str, "pc =%x" _NL,                      RREG(PC));
#elif defined __arm__
	/* src/sys/arch/arm/include/mcontext.h */
	String_Format3(str, "r0 =%x r1 =%x r2 =%x" _NL, RNUM(0),  RNUM(1),  RNUM(2));
	String_Format3(str, "r3 =%x r4 =%x r5 =%x" _NL, RNUM(3),  RNUM(4),  RNUM(5));
	String_Format3(str, "r6 =%x r7 =%x r8 =%x" _NL, RNUM(6),  RNUM(7),  RNUM(8));
	String_Format3(str, "r9 =%x r10=%x fp =%x" _NL, RNUM(9),  RNUM(10), RREG(FP));
	String_Format3(str, "r12=%x sp =%x lr =%x" _NL, RNUM(12), RREG(SP), RREG(LR));
	String_Format1(str, "pc =%x"               _NL, RREG(PC));
#elif defined __powerpc__
	/* src/sys/arch/powerpc/include/mcontext.h */
	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, RNUM(0),  RNUM(1),  RNUM(2),   RNUM(3));
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, RNUM(4),  RNUM(5),  RNUM(6),   RNUM(7));
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, RNUM(8),  RNUM(9),  RNUM(10),  RNUM(11));
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, RNUM(12), RNUM(13), RNUM(14),  RNUM(15));
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, RNUM(16), RNUM(17), RNUM(18),  RNUM(19));
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, RNUM(20), RNUM(21), RNUM(22),  RNUM(23));
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, RNUM(24), RNUM(25), RNUM(26),  RNUM(27));
	String_Format4(str, "r28=%x r29=%x r30=%x r31=%x" _NL, RNUM(28), RNUM(29), RNUM(30),  RNUM(31));
	String_Format4(str, "lr =%x cr =%x xer=%x ctr=%x" _NL, RREG(LR), RREG(CR), RREG(XER), RREG(CTR));
	String_Format1(str, "pc =%x"                      _NL, RREG(PC));
#elif defined __sparc__
	/* src/sys/arch/sparc/include/mcontext.h */
	String_Format4(str, "o0=%x o1=%x o2=%x o3=%x" _NL, RREG(O0), RREG(O1), RREG(O2), RREG(O3));
	String_Format4(str, "o4=%x o5=%x o6=%x o7=%x" _NL, RREG(O4), RREG(O5), RREG(O6), RREG(O7));
	String_Format4(str, "g1=%x g2=%x g3=%x g4=%x" _NL, RREG(G1), RREG(G2), RREG(G3), RREG(G4));
	String_Format4(str, "g5=%x g6=%x g7=%x y =%x" _NL, RREG(G5), RREG(G6), RREG(G7), RREG(Y));
	String_Format2(str, "pc=%x nc=%x" _NL,             RREG(PC), RREG(nPC));
#elif defined __mips__
	/* src/sys/arch/mips/include/mcontext.h */
	String_Format4(str, "at =%x v0 =%x v1 =%x a0 =%x" _NL, RREG(At),   RREG(V0), RREG(V1), RREG(A0));
	String_Format4(str, "a1 =%x a2 =%x a3 =%x t0 =%x" _NL, RREG(A1),   RREG(A2), RREG(A3), RREG(T0)); 
	String_Format4(str, "t1 =%x t2 =%x t3 =%x t4 =%x" _NL, RREG(T1),   RREG(T2), RREG(T3), RREG(T4));
	String_Format4(str, "t5 =%x t6 =%x t7 =%x s0 =%x" _NL, RREG(T5),   RREG(T6), RREG(T7), RREG(S0));
	String_Format4(str, "s1 =%x s2 =%x s3 =%x s4 =%x" _NL, RREG(S1),   RREG(S2), RREG(S3), RREG(S4));
	String_Format4(str, "s5 =%x s6 =%x s7 =%x t8 =%x" _NL, RREG(S5),   RREG(S6), RREG(S7), RREG(T8));
	String_Format4(str, "t9 =%x ko =%x k1 =%x gp =%x" _NL, RREG(T9),   RREG(K0), RREG(K1), RREG(GP));
	String_Format4(str, "sp =%x s8 =%x ra =%x pc =%x" _NL, RREG(SP),   RREG(S8), RREG(Ra), RREG(EPC));
	String_Format3(str, "lo =%x hi =%x"               _NL, RREG(MDLO), RREG(MDHI));
#elif defined __riscv
	/* src/sys/arch/riscv/include/ucontext.h */
	String_Format4(str, "pc =%x ra =%x sp =%x gp =%x" _NL, RREG(PC), RREG(RA), RREG(SP),  RREG(GP));
	String_Format4(str, "tp =%x t0 =%x t1 =%x t2 =%x" _NL, RREG(TP), RREG(T0), RREG(T1),  RREG(T2));
	String_Format4(str, "t3 =%x t4 =%x t5 =%x t6 =%x" _NL, RREG(T3), RREG(T4), RREG(T5),  RREG(T6));
	String_Format4(str, "s0 =%x s1 =%x s2 =%x s3 =%x" _NL, RREG(S0), RREG(S1), RREG(S2),  RREG(S3));
	String_Format4(str, "s4 =%x s5 =%x s6 =%x s7 =%x" _NL, RREG(S4), RREG(S5), RREG(S6),  RREG(S7));
	String_Format4(str, "s8 =%x s9 =%x s10=%x s11=%x" _NL, RREG(S8), RREG(S9), RREG(S10), RREG(S11));
	String_Format4(str, "a0 =%x a1 =%x a2 =%x a3 =%x" _NL, RREG(S0), RREG(A1), RREG(A2),  RREG(A3));
	String_Format4(str, "a4 =%x a5 =%x a6 =%x a7 =%x" _NL, RREG(A4), RREG(A5), RREG(A6),  RREG(A7));
#elif defined __alpha__
	/* src/sys/arch/alpha/include/ucontext.h */
	String_Format4(str, "v0 =%x t0 =%x t1 =%x t2 =%x" _NL, RREG(V0),  RREG(T0),  RREG(T1), RREG(T2));
	String_Format4(str, "t3 =%x t4 =%x t5 =%x t6 =%x" _NL, RREG(T3),  RREG(T4),  RREG(T5), RREG(T6));
	String_Format4(str, "t7 =%x s0 =%x s1 =%x s2 =%x" _NL, RREG(T7),  RREG(S0),  RREG(S1), RREG(S2));
	String_Format4(str, "s3 =%x s4 =%x s5 =%x s6 =%x" _NL, RREG(S3),  RREG(S4),  RREG(S5), RREG(S6));
	String_Format4(str, "a0 =%x a1 =%x a2 =%x a3 =%x" _NL, RREG(A0),  RREG(A1),  RREG(A2), RREG(A3));
	String_Format4(str, "a4 =%x a5 =%x t8 =%x t9 =%x" _NL, RREG(A4),  RREG(A5),  RREG(T8), RREG(T9));
	String_Format4(str, "t10=%x t11=%x ra =%x t12=%x" _NL, RREG(T10), RREG(T11), RREG(RA), RREG(T12));
	String_Format4(str, "at =%x gp =%x sp =%x pc =%x" _NL, RREG(At),  RREG(GP),  RREG(SP), RREG(PC));
#elif defined __sh__
	/* src/sys/arch/sh3/include/ucontext.h */
	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, RREG(R0),   RREG(R1),   RREG(R2),  RREG(R3));
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, RREG(R4),   RREG(R5),   RREG(R6),  RREG(R7));
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, RREG(R8),   RREG(R9),   RREG(R10), RREG(R11));
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, RREG(R12),  RREG(R13),  RREG(R14), RREG(R15));
	String_Format3(str, "mal=%x mah=%x gbr=%x"        _NL, RREG(MACL), RREG(MACH), RREG(GBR));
	String_Format2(str, "pc =%x ra =%x sr =%x"        _NL, RREG(PC),   RREG(PR),   RREG(SR));
#else
	#error "Unknown CPU architecture"
#endif
}

