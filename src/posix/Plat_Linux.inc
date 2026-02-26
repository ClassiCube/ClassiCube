/* See /usr/include/sys/ucontext.h */

/* Need to define this to get REG_ constants */
#define _GNU_SOURCE
#include <sys/ucontext.h>
#include <signal.h>

void CrashHandler_DumpRegisters(void* ctx, cc_string* str) {
#if defined __i386__
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;
	#define RREG(reg) &r->gregs[REG_ ## reg]

	String_Format3(str, "eax=%x ebx=%x ecx=%x" _NL, RREG(EAX), RREG(EBX), RREG(ECX));
	String_Format3(str, "edx=%x esi=%x edi=%x" _NL, RREG(EDX), RREG(ESI), RREG(EDI));
	String_Format3(str, "eip=%x ebp=%x esp=%x" _NL, RREG(EIP), RREG(EBP), RREG(ESP));
#elif defined __x86_64__
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;
	#define RREG(reg) &r->gregs[REG_ ## reg]

	String_Format3(str, "rax=%x rbx=%x rcx=%x" _NL, RREG(RAX), RREG(RBX), RREG(RCX));
	String_Format3(str, "rdx=%x rsi=%x rdi=%x" _NL, RREG(RAX), RREG(RSI), RREG(RDI));
	String_Format3(str, "rip=%x rbp=%x rsp=%x" _NL, RREG(RIP), RREG(RBP), RREG(RSP));
	String_Format3(str, "r8 =%x r9 =%x r10=%x" _NL, RREG(R8),  RREG(R9),  RREG(R10));
	String_Format3(str, "r11=%x r12=%x r13=%x" _NL, RREG(R11), RREG(R12), RREG(R13));
	String_Format2(str, "r14=%x r15=%x" _NL,        RREG(R14), RREG(R15));
#elif defined __aarch64__
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;
	#define RREG(reg) &r->reg
	#define RNUM(reg) &r->regs[reg]

	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, RNUM(0),  RNUM(1),  RNUM(2),  RNUM(3));
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, RNUM(4),  RNUM(5),  RNUM(6),  RNUM(7));
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, RNUM(8),  RNUM(9),  RNUM(10), RNUM(11));
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, RNUM(12), RNUM(13), RNUM(14), RNUM(15));
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, RNUM(16), RNUM(17), RNUM(18), RNUM(19));
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, RNUM(20), RNUM(21), RNUM(22), RNUM(23));
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, RNUM(24), RNUM(25), RNUM(26), RNUM(27));
	String_Format4(str, "r28=%x r29=%x lr =%x sp =%x" _NL, RNUM(28), RNUM(29), RNUM(30), RREG(sp));
	String_Format1(str, "pc =%x" _NL,                      RREG(pc));
#elif defined __arm__
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;
	#define RREG(reg) &r->arm_##reg
	#define RNUM(reg) &r->arm_r##reg

	String_Format3(str, "r0 =%x r1 =%x r2 =%x" _NL, RNUM(0),  RNUM(1),  RNUM(2));
	String_Format3(str, "r3 =%x r4 =%x r5 =%x" _NL, RNUM(3),  RNUM(4),  RNUM(5));
	String_Format3(str, "r6 =%x r7 =%x r8 =%x" _NL, RNUM(6),  RNUM(7),  RNUM(8));
	String_Format3(str, "r9 =%x r10=%x fp =%x" _NL, RNUM(9),  RNUM(10), RREG(fp));
	String_Format3(str, "ip =%x sp =%x lr =%x" _NL, RREG(ip), RREG(sp), RREG(lr));
	String_Format1(str, "pc =%x"               _NL, RREG(pc));
#elif defined __alpha__
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;	
	#define RREG(reg) &r->sc_##reg
	#define RNUM(reg) &r->sc_regs[reg]

	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, RNUM(0),  RNUM(1),  RNUM(2),  RNUM(3));
	String_Format4(str, "r4 =%x r5 =%x t5 =%x r7 =%x" _NL, RNUM(4),  RNUM(5),  RNUM(6),  RNUM(7));
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, RNUM(8),  RNUM(9),  RNUM(10), RNUM(11));
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, RNUM(12), RNUM(13), RNUM(14), RNUM(15));
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, RNUM(16), RNUM(17), RNUM(18), RNUM(19));
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, RNUM(20), RNUM(21), RNUM(22), RNUM(23));
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, RNUM(24), RNUM(25), RNUM(26), RNUM(27));
	String_Format4(str, "r28=%x r29=%x r30=%x pc =%x" _NL, RNUM(28), RNUM(29), RNUM(30), RREG(pc));
#elif defined __sparc__
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;
	#define RREG(reg) &r->gregs[REG_ ## reg]

	String_Format4(str, "o0=%x o1=%x o2=%x o3=%x" _NL, RREG(O0), RREG(O1), RREG(O2), RREG(O3));
	String_Format4(str, "o4=%x o5=%x o6=%x o7=%x" _NL, RREG(O4), RREG(O5), RREG(O6), RREG(O7));
	String_Format4(str, "g1=%x g2=%x g3=%x g4=%x" _NL, RREG(G1), RREG(G2), RREG(G3), RREG(G4));
	String_Format4(str, "g5=%x g6=%x g7=%x y =%x" _NL, RREG(G5), RREG(G6), RREG(G7), RREG(Y));
	String_Format2(str, "pc=%x nc=%x"             _NL, RREG(PC), RREG(nPC));
#elif defined __PPC__ && __WORDSIZE == 32
	/* See sysdeps/unix/sysv/linux/powerpc/sys/ucontext.h in glibc */
	mcontext_t* r = ((ucontext_t*)ctx)->uc_mcontext.uc_regs;
	#define RNUM(reg) &r->gregs[reg]

	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, RNUM(0),  RNUM(1),  RNUM(2),  RNUM(3));
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, RNUM(4),  RNUM(5),  RNUM(6),  RNUM(7));
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, RNUM(8),  RNUM(9),  RNUM(10), RNUM(11));
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, RNUM(12), RNUM(13), RNUM(14), RNUM(15));
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, RNUM(16), RNUM(17), RNUM(18), RNUM(19));
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, RNUM(20), RNUM(21), RNUM(22), RNUM(23));
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, RNUM(24), RNUM(25), RNUM(26), RNUM(27));
	String_Format4(str, "r28=%x r29=%x r30=%x r31=%x" _NL, RNUM(28), RNUM(29), RNUM(30), RNUM(31));
	String_Format3(str, "pc =%x lr =%x ctr=%x"        _NL, RNUM(32), RNUM(35), RNUM(34));
#elif defined __PPC__
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;
	/* TODO LR,CTR might be wrong, compare with PT_LNK in */
	/* https://elixir.bootlin.com/linux/v4.19.122/source/arch/powerpc/include/uapi/asm/ptrace.h#L102 */
	#define RNUM(reg) &r->gp_regs[reg]

	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, RNUM(0),  RNUM(1),  RNUM(2),  RNUM(3));
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, RNUM(4),  RNUM(5),  RNUM(6),  RNUM(7));
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, RNUM(8),  RNUM(9),  RNUM(10), RNUM(11));
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, RNUM(12), RNUM(13), RNUM(14), RNUM(15));
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, RNUM(16), RNUM(17), RNUM(18), RNUM(19));
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, RNUM(20), RNUM(21), RNUM(22), RNUM(23));
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, RNUM(24), RNUM(25), RNUM(26), RNUM(27));
	String_Format4(str, "r28=%x r29=%x r30=%x r31=%x" _NL, RNUM(28), RNUM(29), RNUM(30), RNUM(31));
	String_Format3(str, "pc =%x lr =%x ctr=%x"        _NL, RNUM(32), RNUM(35), RNUM(34));
#elif defined __mips__
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;
	#define RREG(reg) &r->reg
	#define RNUM(reg) &r->greps[reg]

	String_Format4(str, "r1 =%x r2 =%x r3 =%x r4 =%x" _NL, RNUM(1),  RNUM(2),  RNUM(3),  RNUM(4));
	String_Format4(str, "r5 =%x r6 =%x r7 =%x r8 =%x" _NL, RNUM(5),  RNUM(6),  RNUM(7),  RNUM(8));
	String_Format4(str, "r9 =%x r10=%x r11=%x r12=%x" _NL, RNUM(9),  RNUM(10), RNUM(11), RNUM(12));
	String_Format4(str, "r13=%x r14=%x r15=%x r16=%x" _NL, RNUM(13), RNUM(14), RNUM(15), RNUM(16));
	String_Format4(str, "r17=%x r18=%x r19=%x r20=%x" _NL, RNUM(17), RNUM(18), RNUM(19), RNUM(20));
	String_Format4(str, "r21=%x r22=%x r23=%x r24=%x" _NL, RNUM(21), RNUM(22), RNUM(23), RNUM(24));
	String_Format4(str, "r25=%x r26=%x r27=%x r28=%x" _NL, RNUM(25), RNUM(26), RNUM(27), RNUM(28));
	String_Format4(str, "sp =%x fp =%x ra =%x pc =%x" _NL, RNUM(29), RNUM(30), RNUM(31), RREG(pc));
	String_Format3(str, "lo =%x hi =%x" _NL,               &r->mdlo, &r->mdhi);
#elif defined __riscv
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;
	#define RREG(reg) &r->__gregs[REG_##reg]
	#define RNUM(reg) &r->__gregs[reg]

	String_Format4(str, "pc =%x ra =%x sp =%x r3 =%x" _NL, RREG(PC), RNUM(1),  RNUM(2),  RNUM(3));
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, RNUM(4),  RNUM(5),  RNUM(6),  RNUM(7));
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, RNUM(8),  RNUM(9),  RNUM(10), RNUM(11));
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, RNUM(12), RNUM(13), RNUM(14), RNUM(15));
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, RNUM(16), RNUM(17), RNUM(18), RNUM(19));
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, RNUM(20), RNUM(21), RNUM(22), RNUM(23));
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, RNUM(24), RNUM(25), RNUM(26), RNUM(27));
	String_Format4(str, "r28=%x r29=%x r30=%x r31=%x" _NL, RNUM(28), RNUM(29), RNUM(30), RNUM(31));
#elif defined __e2k__
	struct sigcontext* r = &((ucontext_t*)ctx)->uc_mcontext;
	#define RREG(reg) &r->reg

	/* Procedure Stack Pointer, Procedure Chain Stack Pointer, User Stack Base Pointer, User Stack Pointer */
	String_Format4(str, "ps =%x pcs=%x sbr=%x usd=%x" _NL, RREG(psp_hi), RREG(pcsp_hi), RREG(sbr), RREG(usd_hi));
	/* CR0 contains Instruction Pointer */
	String_Format1(str, "ip =%x"                      _NL, RREG(cr0_hi));
#else
	#error "Unknown CPU architecture"
#endif
}
