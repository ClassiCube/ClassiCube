/* See /usr/include/[ARCH]/mcontext.h */
/* -> src/sys/arch/[ARCH]/include/mcontext.h */
#include <signal.h>
#include <sys/ucontext.h>

void CrashHandler_DumpRegisters(void* ctx, cc_string* str) {
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;

#if defined __i386__
	/* src/sys/arch/i386/include/mcontext.h */
	String_Format4(str, "eax=%x ebx=%x ecx=%x edx=%x" _NL, &r->__gregs[_REG_EAX], &r->__gregs[_REG_EBX], &r->__gregs[_REG_ECX], &r->__gregs[_REG_EDX]);
	String_Format4(str, "esi=%x edi=%x ebp=%x esp=%x" _NL, &r->__gregs[_REG_ESI], &r->__gregs[_REG_EDI], &r->__gregs[_REG_EBP], &r->__gregs[_REG_ESP]);
	String_Format1(str, "eip=%x"                      _NL, &r->__gregs[_REG_EIP]);
#elif defined __x86_64__
	/* usr/src/uts/intel/sys/regset.h */
	String_Format4(str, "rax=%x rbx=%x rcx=%x rdx=%x" _NL, &r->__gregs[_REG_RAX], &r->__gregs[_REG_RBX], &r->__gregs[_REG_RCX], &r->__gregs[_REG_RDX]);
	String_Format4(str, "rsi=%x rdi=%x rbp=%x rsp=%x" _NL, &r->__gregs[_REG_RSI], &r->__gregs[_REG_RDI], &r->__gregs[_REG_RBP], &r->__gregs[_REG_RSP]);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->__gregs[_REG_R8],  &r->__gregs[_REG_R9],  &r->__gregs[_REG_R10], &r->__gregs[_REG_R11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->__gregs[_REG_R12], &r->__gregs[_REG_R13], &r->__gregs[_REG_R14], &r->__gregs[_REG_R15]);
	String_Format1(str, "rip=%x"                      _NL, &r->__gregs[_REG_RIP]);
#elif defined __aarch64__
	/* src/sys/arch/arm/include/mcontext.h */
	/* FP = R29, LR = R30, SP = R31 */
	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->__gregs[0],  &r->__gregs[1],  &r->__gregs[2],  &r->__gregs[3]);
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->__gregs[4],  &r->__gregs[5],  &r->__gregs[6],  &r->__gregs[7]);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->__gregs[8],  &r->__gregs[9],  &r->__gregs[10], &r->__gregs[11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->__gregs[12], &r->__gregs[13], &r->__gregs[14], &r->__gregs[15]);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->__gregs[16], &r->__gregs[17], &r->__gregs[18], &r->__gregs[19]);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->__gregs[20], &r->__gregs[21], &r->__gregs[22], &r->__gregs[23]);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->__gregs[24], &r->__gregs[25], &r->__gregs[26], &r->__gregs[27]);
	String_Format4(str, "r28=%x fp =%x lr =%x sp =%x" _NL, &r->__gregs[28], &r->__gregs[29], &r->__gregs[30], &r->__gregs[31]);
	String_Format1(str, "pc =%x"                      _NL, &r->__gregs[_REG_PC]);
#elif defined __arm__
	/* src/sys/arch/arm/include/mcontext.h */
	/* FP = R11, SP = R13, LR = R14, PC = R15 */
	String_Format3(str, "r0 =%x r1 =%x r2 =%x" _NL, &r->__gregs[0],  &r->__gregs[1],  &r->__gregs[2]);
	String_Format3(str, "r3 =%x r4 =%x r5 =%x" _NL, &r->__gregs[3],  &r->__gregs[4],  &r->__gregs[5]);
	String_Format3(str, "r6 =%x r7 =%x r8 =%x" _NL, &r->__gregs[6],  &r->__gregs[7],  &r->__gregs[8]);
	String_Format3(str, "r9 =%x r10=%x fp =%x" _NL, &r->__gregs[9],  &r->__gregs[10], &r->__gregs[11]);
	String_Format3(str, "r12=%x sp =%x lr =%x" _NL, &r->__gregs[12], &r->__gregs[13], &r->__gregs[14]);
	String_Format1(str, "pc =%x"               _NL, &r->__gregs[_REG_PC]);
#elif defined __powerpc__
	/* src/sys/arch/powerpc/include/mcontext.h */
	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->__gregs[0],  &r->__gregs[1],  &r->__gregs[2],   &r->__gregs[3]);
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->__gregs[4],  &r->__gregs[5],  &r->__gregs[6],   &r->__gregs[7]);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->__gregs[8],  &r->__gregs[9],  &r->__gregs[10],  &r->__gregs[11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->__gregs[12], &r->__gregs[13], &r->__gregs[14],  &r->__gregs[15]);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->__gregs[16], &r->__gregs[17], &r->__gregs[18],  &r->__gregs[19]);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->__gregs[20], &r->__gregs[21], &r->__gregs[22],  &r->__gregs[23]);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->__gregs[24], &r->__gregs[25], &r->__gregs[26],  &r->__gregs[27]);
	String_Format4(str, "r28=%x r29=%x r30=%x r31=%x" _NL, &r->__gregs[28], &r->__gregs[29], &r->__gregs[30],  &r->__gregs[31]);
	String_Format4(str, "lr =%x cr =%x xer=%x ctr=%x" _NL, &r->__gregs[_REG_LR], &r->__gregs[_REG_CR], &r->__gregs[_REG_XER], &r->__gregs[_REG_CTR]);
	String_Format1(str, "pc =%x"                      _NL, &r->__gregs[_REG_PC]);
#elif defined __sparc__
	/* src/sys/arch/sparc/include/mcontext.h */
	String_Format4(str, "o0=%x o1=%x o2=%x o3=%x" _NL, &r->__gregs[_REG_O0], &r->__gregs[_REG_O1], &r->__gregs[_REG_O2], &r->__gregs[_REG_O3]);
	String_Format4(str, "o4=%x o5=%x o6=%x o7=%x" _NL, &r->__gregs[_REG_O4], &r->__gregs[_REG_O5], &r->__gregs[_REG_O6], &r->__gregs[_REG_O7]);
	String_Format4(str, "g1=%x g2=%x g3=%x g4=%x" _NL, &r->__gregs[_REG_G1], &r->__gregs[_REG_G2], &r->__gregs[_REG_G3], &r->__gregs[_REG_G4]);
	String_Format4(str, "g5=%x g6=%x g7=%x y =%x" _NL, &r->__gregs[_REG_G5], &r->__gregs[_REG_G6], &r->__gregs[_REG_G7], &r->__gregs[_REG_Y]);
	String_Format2(str, "pc=%x nc=%x" _NL,             &r->__gregs[_REG_PC], &r->__gregs[_REG_nPC]);
#elif defined __mips__
	/* src/sys/arch/mips/include/mcontext.h */
	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->__gregs[0],  &r->__gregs[1],  &r->__gregs[2],  &r->__gregs[3] );
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->__gregs[4],  &r->__gregs[5],  &r->__gregs[6],  &r->__gregs[7] );
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->__gregs[8],  &r->__gregs[9],  &r->__gregs[10], &r->__gregs[11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->__gregs[12], &r->__gregs[13], &r->__gregs[14], &r->__gregs[15]);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->__gregs[16], &r->__gregs[17], &r->__gregs[18], &r->__gregs[19]);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->__gregs[20], &r->__gregs[21], &r->__gregs[22], &r->__gregs[23]);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->__gregs[24], &r->__gregs[25], &r->__gregs[26], &r->__gregs[27]);
	String_Format4(str, "r28=%x sp =%x fp =%x ra =%x" _NL, &r->__gregs[28], &r->__gregs[29], &r->__gregs[30], &r->__gregs[31]);
	String_Format3(str, "lo =%x hi =%x pc =%x"        _NL, &r->__gregs[_REG_MDLO], &r->__gregs[_REG_MDHI], &r->__gregs[_REG_EPC]);
#elif defined __riscv
	/* src/sys/arch/riscv/include/ucontext.h */
	String_Format4(str, "pc =%x ra =%x sp =%x gp =%x" _NL, &r->__gregs[_REG_PC], &r->__gregs[_REG_RA], &r->__gregs[_REG_SP],  &r->__gregs[_REG_GP]);
	String_Format4(str, "tp =%x t0 =%x t1 =%x t2 =%x" _NL, &r->__gregs[_REG_TP], &r->__gregs[_REG_T0], &r->__gregs[_REG_T1],  &r->__gregs[_REG_T2]);
	String_Format4(str, "t3 =%x t4 =%x t5 =%x t6 =%x" _NL, &r->__gregs[_REG_T3], &r->__gregs[_REG_T4], &r->__gregs[_REG_T5],  &r->__gregs[_REG_T6]);
	String_Format4(str, "s0 =%x s1 =%x s2 =%x s3 =%x" _NL, &r->__gregs[_REG_S0], &r->__gregs[_REG_S1], &r->__gregs[_REG_S2],  &r->__gregs[_REG_S3]);
	String_Format4(str, "s4 =%x s5 =%x s6 =%x s7 =%x" _NL, &r->__gregs[_REG_S4], &r->__gregs[_REG_S5], &r->__gregs[_REG_S6],  &r->__gregs[_REG_S7]);
	String_Format4(str, "s8 =%x s9 =%x s10=%x s11=%x" _NL, &r->__gregs[_REG_S8], &r->__gregs[_REG_S9], &r->__gregs[_REG_S10], &r->__gregs[_REG_S11]);
	String_Format4(str, "a0 =%x a1 =%x a2 =%x a3 =%x" _NL, &r->__gregs[_REG_S0], &r->__gregs[_REG_A1], &r->__gregs[_REG_A2],  &r->__gregs[_REG_A3]);
	String_Format4(str, "a4 =%x a5 =%x a6 =%x a7 =%x" _NL, &r->__gregs[_REG_A4], &r->__gregs[_REG_A5], &r->__gregs[_REG_A6],  &r->__gregs[_REG_A7]);
#elif defined __alpha__
	/* src/sys/arch/alpha/include/ucontext.h */
	/* RA = R26, GP = R29, SP = R30 */
	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->__gregs[0],  &r->__gregs[1],  &r->__gregs[2],  &r->__gregs[3]);
	String_Format4(str, "r4 =%x r5 =%x t5 =%x r7 =%x" _NL, &r->__gregs[4],  &r->__gregs[5],  &r->__gregs[6],  &r->__gregs[7]);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->__gregs[8],  &r->__gregs[9],  &r->__gregs[10], &r->__gregs[11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->__gregs[12], &r->__gregs[13], &r->__gregs[14], &r->__gregs[15]);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->__gregs[16], &r->__gregs[17], &r->__gregs[18], &r->__gregs[19]);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->__gregs[20], &r->__gregs[21], &r->__gregs[22], &r->__gregs[23]);
	String_Format4(str, "r24=%x r25=%x ra =%x r27=%x" _NL, &r->__gregs[24], &r->__gregs[25], &r->__gregs[26], &r->__gregs[27]);
	String_Format4(str, "r28=%x gp =%x sp =%x r31=%x" _NL, &r->__gregs[28], &r->__gregs[29], &r->__gregs[30], &r->__gregs[31]);
	String_Format2(str, "pc =%x ps =%x"               _NL, &r->__gregs[_REG_PC], &r->__gregs[_REG_PS]);
#elif defined __sh__
	/* src/sys/arch/sh3/include/ucontext.h */
	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->__gregs[_REG_R0],   &r->__gregs[_REG_R1],   &r->__gregs[_REG_R2],  &r->__gregs[_REG_R3]);
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->__gregs[_REG_R4],   &r->__gregs[_REG_R5],   &r->__gregs[_REG_R6],  &r->__gregs[_REG_R7]);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->__gregs[_REG_R8],   &r->__gregs[_REG_R9],   &r->__gregs[_REG_R10], &r->__gregs[_REG_R11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->__gregs[_REG_R12],  &r->__gregs[_REG_R13],  &r->__gregs[_REG_R14], &r->__gregs[_REG_R15]);
	String_Format3(str, "mal=%x mah=%x gbr=%x"        _NL, &r->__gregs[_REG_MACL], &r->__gregs[_REG_MACH], &r->__gregs[_REG_GBR]);
	String_Format2(str, "pc =%x ra =%x sr =%x"        _NL, &r->__gregs[_REG_PC],   &r->__gregs[_REG_PR],   &r->__gregs[_REG_SR]);
#else
	#error "Unknown CPU architecture"
#endif
}

