/* See /usr/include/machine/ucontext.h */
/* -> src/sys/[ARCH]/include/ucontext.h */
#include <signal.h>
#include <sys/ucontext.h>

void CrashHandler_DumpRegisters(void* ctx, cc_string* str) {
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;
#if defined __i386__
	/* src/sys/x86/include/ucontext.h */
	#define RREG(reg) &r->mc_##reg

	String_Format3(str, "eax=%x ebx=%x ecx=%x" _NL, RREG(eax), RREG(ebx), RREG(ecx));
	String_Format3(str, "edx=%x esi=%x edi=%x" _NL, RREG(edx), RREG(esi), RREG(edi));
	String_Format3(str, "eip=%x ebp=%x esp=%x" _NL, RREG(eip), RREG(ebp), RREG(esp));
#elif defined __x86_64__
	/* src/sys/x86/include/ucontext.h */
	#define RREG(reg) &r->mc_##reg

	String_Format3(str, "rax=%x rbx=%x rcx=%x" _NL, RREG(rax), RREG(rbx), RREG(rcx));
	String_Format3(str, "rdx=%x rsi=%x rdi=%x" _NL, RREG(rdx), RREG(rsi), RREG(rdi));
	String_Format3(str, "rip=%x rbp=%x rsp=%x" _NL, RREG(rip), RREG(rbp), RREG(rsp));
	String_Format3(str, "r8 =%x r9 =%x r10=%x" _NL, RREG(r8),  RREG(r9),  RREG(r10));
	String_Format3(str, "r11=%x r12=%x r13=%x" _NL, RREG(r11), RREG(r12), RREG(r13));
	String_Format2(str, "r14=%x r15=%x" _NL,        RREG(r14), RREG(r15));
#elif defined __aarch64__
	/* src/sys/arm64/include/ucontext.h */
	#define RREG(reg) &r->mc_gpregs.gp_ ## reg
	#define RNUM(reg) &r->mc_gpregs.gp_x[reg]

	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, RNUM(0),  RNUM(1),  RNUM(2),  RNUM(3));
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, RNUM(4),  RNUM(5),  RNUM(6),  RNUM(7));
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, RNUM(8),  RNUM(9),  RNUM(10), RNUM(11));
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, RNUM(12), RNUM(13), RNUM(14), RNUM(15));
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, RNUM(16), RNUM(17), RNUM(18), RNUM(19));
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, RNUM(20), RNUM(21), RNUM(22), RNUM(23));
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, RNUM(24), RNUM(25), RNUM(26), RNUM(27));
	String_Format4(str, "r28=%x r29=%x lr =%x sp =%x" _NL, RNUM(28), RNUM(29), RREG(lr), RREG(sp));
	String_Format1(str, "pc =%x" _NL,                      RREG(elr));
#elif defined __arm__
	/* src/sys/arm/include/ucontext.h */
	#define RREG(reg) &r->__gregs[_REG_ ## reg]
	#define RNUM(reg) &r->__gregs[reg]

	String_Format3(str, "r0 =%x r1 =%x r2 =%x" _NL, RNUM(0),  RNUM(1),  RNUM(2));
	String_Format3(str, "r3 =%x r4 =%x r5 =%x" _NL, RNUM(3),  RNUM(4),  RNUM(5));
	String_Format3(str, "r6 =%x r7 =%x r8 =%x" _NL, RNUM(6),  RNUM(7),  RNUM(8));
	String_Format3(str, "r9 =%x r10=%x fp =%x" _NL, RNUM(9),  RNUM(10), RREG(FP));
	String_Format3(str, "r12=%x sp =%x lr =%x" _NL, RNUM(12), RREG(SP), RREG(LR));
	String_Format1(str, "pc =%x"               _NL, RREG(PC));
#elif defined __riscv
	/* src/sys/riscv/include/ucontext.h */
	#define RREG(reg) &r->gp_##reg

	String_Format4(str, "pc =%x ra =%x sp =%x gp =%x" _NL, RREG(sepc), RREG(ra),   RREG(sp),    RREG(gp));
	String_Format4(str, "tp =%x t0 =%x t1 =%x t2 =%x" _NL, RREG(tp),   RREG(t[0]), RREG(t[1]),  RREG(t[2]));
	String_Format4(str, "t3 =%x t4 =%x t5 =%x t6 =%x" _NL, RREG(t[3]), RREG(t[4]), RREG(t[5]),  RREG(t[6]));
	String_Format4(str, "s0 =%x s1 =%x s2 =%x s3 =%x" _NL, RREG(s[0]), RREG(s[1]), RREG(s[2]),  RREG(s[3]));
	String_Format4(str, "s4 =%x s5 =%x s6 =%x s7 =%x" _NL, RREG(s[4]), RREG(s[5]), RREG(s[6]),  RREG(s[7]));
	String_Format4(str, "s8 =%x s9 =%x s10=%x s11=%x" _NL, RREG(s[8]), RREG(s[9]), RREG(s[10]), RREG(s[11]));
	String_Format4(str, "a0 =%x a1 =%x a2 =%x a3 =%x" _NL, RREG(s[0]), RREG(a[1]), RREG(a[2]),  RREG(a[3]));
	String_Format4(str, "a4 =%x a5 =%x a6 =%x a7 =%x" _NL, RREG(a[4]), RREG(a[5]), RREG(a[6]),  RREG(a[7]));
#elif defined __powerpc__
	/* src/sys/powerpc/include/ucontext.h */
	#define RREG(reg) &r->mc ## reg
	#define RNUM(reg) &r->mc_frame[reg]

	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, RNUM(0),  RNUM(1),  RNUM(2),   RNUM(3));
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, RNUM(4),  RNUM(5),  RNUM(6),   RNUM(7));
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, RNUM(8),  RNUM(9),  RNUM(10),  RNUM(11));
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, RNUM(12), RNUM(13), RNUM(14),  RNUM(15));
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, RNUM(16), RNUM(17), RNUM(18),  RNUM(19));
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, RNUM(20), RNUM(21), RNUM(22),  RNUM(23));
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, RNUM(24), RNUM(25), RNUM(26),  RNUM(27));
	String_Format4(str, "r28=%x r29=%x r30=%x r31=%x" _NL, RNUM(28), RNUM(29), RNUM(30),  RNUM(31));
	String_Format4(str, "lr =%x cr =%x xer=%x ctr=%x" _NL, RREG(lr), RREG(cr), RREG(xer), RREG(ctr));
	String_Format1(str, "pc =%x"                      _NL, RREG(srr0));
#elif defined __mips__
	#define RREG(reg) &r->mc ## reg
	#define RNUM(reg) &r->mc_regs[reg]

	String_Format4(str, "r1 =%x r2 =%x r3 =%x r4 =%x" _NL, RNUM(1),   RNUM(2),  RNUM(3),  RNUM(4));
	String_Format4(str, "r5 =%x r6 =%x r7 =%x r8 =%x" _NL, RNUM(5),   RNUM(6),  RNUM(7),  RNUM(8));
	String_Format4(str, "r9 =%x r10=%x r11=%x r12=%x" _NL, RNUM(9),   RNUM(10), RNUM(11), RNUM(12));
	String_Format4(str, "r13=%x r14=%x r15=%x r16=%x" _NL, RNUM(13),  RNUM(14), RNUM(15), RNUM(16));
	String_Format4(str, "r17=%x r18=%x r19=%x r20=%x" _NL, RNUM(17),  RNUM(18), RNUM(19), RNUM(20));
	String_Format4(str, "r21=%x r22=%x r23=%x r24=%x" _NL, RNUM(21),  RNUM(22), RNUM(23), RNUM(24));
	String_Format4(str, "r25=%x r26=%x r27=%x r28=%x" _NL, RNUM(25),  RNUM(26), RNUM(27), RNUM(28));
	String_Format4(str, "sp =%x fp =%x ra =%x pc =%x" _NL, RNUM(29),  RNUM(30), RNUM(31), RREG(pc));
	String_Format3(str, "lo =%x hi =%x" _NL,               &r->mullo, &r->mullhi);
#else
	#error "Unknown CPU architecture"
#endif
}
