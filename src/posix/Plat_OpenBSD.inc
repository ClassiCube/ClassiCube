/* See /usr/include/machine/signal.h */
/* -> src/sys/arch/[ARCH]/include/signal.h */

/* Register constants can be found from includes in <signal.h> */
#include <signal.h>

void CrashHandler_DumpRegisters(void* ctx, cc_string* str) {
	ucontext_t* r = (ucontext_t*)ctx;

#if defined __i386__
	/* src/sys/arch/i386/include/signal.h */
	String_Format4(str, "eax=%x ebx=%x ecx=%x edx=%x" _NL, &r->sc_eax, &r->sc_ebx, &r->sc_ecx, &r->sc_edx);
	String_Format4(str, "esi=%x edi=%x ebp=%x esp=%x" _NL, &r->sc_esi, &r->sc_edi, &r->sc_ebp, &r->sc_esp);
	String_Format1(str, "eip=%x"                      _NL, &r->sc_eip);
#elif defined __x86_64__
	/* src/sys/arch/amd64/include/signal.h */
	String_Format4(str, "rax=%x rbx=%x rcx=%x rdx=%x" _NL, &r->sc_rax, &r->sc_rbx, &r->sc_rcx, &r->sc_rdx);
	String_Format4(str, "rsi=%x rdi=%x rbp=%x rsp=%x" _NL, &r->sc_rsi, &r->sc_rdi, &r->sc_rbp, &r->sc_rsp);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->sc_r8,  &r->sc_r9,  &r->sc_r10, &r->sc_r11);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->sc_r12, &r->sc_r13, &r->sc_r14, &r->sc_r15);
	String_Format1(str, "rip=%x"                      _NL, &r->sc_rip);
#elif defined __aarch64__
	/* src/sys/arch/arm64/include/signal.h */
	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->sc_x[0],  &r->sc_x[1],  &r->sc_x[2],  &r->sc_x[3]);
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->sc_x[4],  &r->sc_x[5],  &r->sc_x[6],  &r->sc_x[7]);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->sc_x[8],  &r->sc_x[9],  &r->sc_x[10], &r->sc_x[11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->sc_x[12], &r->sc_x[13], &r->sc_x[14], &r->sc_x[15]);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->sc_x[16], &r->sc_x[17], &r->sc_x[18], &r->sc_x[19]);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->sc_x[20], &r->sc_x[21], &r->sc_x[22], &r->sc_x[23]);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->sc_x[24], &r->sc_x[25], &r->sc_x[26], &r->sc_x[27]);
	String_Format4(str, "r28=%x r29=%x lr =%x sp =%x" _NL, &r->sc_x[28], &r->sc_x[29], &r->sc_lr,    &r->sc_sp);
	String_Format1(str, "pc =%x"                      _NL, &r->sc_elr);
#elif defined __arm__
	/* src/sys/arch/arm/include/signal.h */
	String_Format3(str, "r0 =%x r1 =%x r2 =%x" _NL, &r->sc_r[0],  &r->sc_r[1],   &r->sc_r[2]);
	String_Format3(str, "r3 =%x r4 =%x r5 =%x" _NL, &r->sc_r[3],  &r->sc_r[4],   &r->sc_r[5]);
	String_Format3(str, "r6 =%x r7 =%x r8 =%x" _NL, &r->sc_r[6],  &r->sc_r[7],   &r->sc_r[8]);
	String_Format3(str, "r9 =%x r10=%x r11=%x" _NL, &r->sc_r[9],  &r->sc_r[10],  &r->sc_r[11]);
	String_Format3(str, "r12=%x sp =%x lr =%x" _NL, &r->sc_r[12], &r->sc_usr_sp, &r->sc_usr_lr);
	String_Format1(str, "pc =%x"               _NL, &r->sc_pc);
#elif defined __powerpc__
	/* src/sys/arch/powerpc/include/signal.h */
	struct trapframe* f = &r->sc_frame;

	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &f->fixreg[0],  &f->fixreg[1],  &f->fixreg[2],  &f->fixreg[3]);
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &f->fixreg[4],  &f->fixreg[5],  &f->fixreg[6],  &f->fixreg[7]);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &f->fixreg[8],  &f->fixreg[9],  &f->fixreg[10], &f->fixreg[11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &f->fixreg[12], &f->fixreg[13], &f->fixreg[14], &f->fixreg[15]);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &f->fixreg[16], &f->fixreg[17], &f->fixreg[18], &f->fixreg[19]);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &f->fixreg[20], &f->fixreg[21], &f->fixreg[22], &f->fixreg[23]);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &f->fixreg[24], &f->fixreg[25], &f->fixreg[26], &f->fixreg[27]);
	String_Format4(str, "r28=%x r29=%x r30=%x r31=%x" _NL, &f->fixreg[28], &f->fixreg[29], &f->fixreg[30], &f->fixreg[31]);
	String_Format4(str, "lr =%x cr =%x xer=%x ctr=%x" _NL, &f->lr, &f->cr, &f->xer, &f->ctr);
	String_Format1(str, "pc =%x"                      _NL, &f->srr0);
#elif defined __mips__
	/* src/sys/arch/mips64/include/signal.h */
	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->sc_regs[0],  &r->sc_regs[1],  &r->sc_regs[2],  &r->sc_regs[3] );
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->sc_regs[4],  &r->sc_regs[5],  &r->sc_regs[6],  &r->sc_regs[7] );
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->sc_regs[8],  &r->sc_regs[9],  &r->sc_regs[10], &r->sc_regs[11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->sc_regs[12], &r->sc_regs[13], &r->sc_regs[14], &r->sc_regs[15]);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->sc_regs[16], &r->sc_regs[17], &r->sc_regs[18], &r->sc_regs[19]);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->sc_regs[20], &r->sc_regs[21], &r->sc_regs[22], &r->sc_regs[23]);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->sc_regs[24], &r->sc_regs[25], &r->sc_regs[26], &r->sc_regs[27]);
	String_Format4(str, "r28=%x sp =%x fp =%x ra =%x" _NL, &r->sc_regs[28], &r->sc_regs[29], &r->sc_regs[30], &r->sc_regs[31]);
	String_Format3(str, "lo =%x hi =%x pc =%x"        _NL, &r->mullo, &r->mullhi, &r->sc_pc);
#elif defined __alpha__
	/* src/sys/arch/alpha/include/signal.h */
	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->sc_regs[0],  &r->sc_regs[1],  &r->sc_regs[2],  &r->sc_regs[3]);
	String_Format4(str, "r4 =%x r5 =%x t5 =%x r7 =%x" _NL, &r->sc_regs[4],  &r->sc_regs[5],  &r->sc_regs[6],  &r->sc_regs[7]);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->sc_regs[8],  &r->sc_regs[9],  &r->sc_regs[10], &r->sc_regs[11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->sc_regs[12], &r->sc_regs[13], &r->sc_regs[14], &r->sc_regs[15]);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->sc_regs[16], &r->sc_regs[17], &r->sc_regs[18], &r->sc_regs[19]);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->sc_regs[20], &r->sc_regs[21], &r->sc_regs[22], &r->sc_regs[23]);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->sc_regs[24], &r->sc_regs[25], &r->sc_regs[26], &r->sc_regs[27]);
	String_Format4(str, "r28=%x r29=%x r30=%x r31=%x"  NL, &r->sc_regs[28], &r->sc_regs[29], &r->sc_regs[30], &r->sc_regs[31]);
	String_Format2(str, "pc =%x ps =%x"               _NL, &r->sc_pc, &r->sc_ps);
#elif defined __riscv
	/* src/sys/arch/riscv64/include/signal.h */
	/* note that R0 = ZERO and is not in registers struct */
	String_Format3(str, "ra =%x sp =%x gp =%x"        _NL,              &r->sc_ra,   &r->sc_sp,    &r->sc_gp);
	String_Format4(str, "tp =%x t0 =%x t1 =%x t2 =%x" _NL, &r->sc_tp,   &r->sc_t[0], &r->sc_t[1],  &r->sc_t[2]);
	String_Format4(str, "t3 =%x t4 =%x t5 =%x t6 =%x" _NL, &r->sc_t[3], &r->sc_t[4], &r->sc_t[5],  &r->sc_t[6]);
	String_Format4(str, "s0 =%x s1 =%x s2 =%x s3 =%x" _NL, &r->sc_s[0], &r->sc_s[1], &r->sc_s[2],  &r->sc_s[3]);
	String_Format4(str, "s4 =%x s5 =%x s6 =%x s7 =%x" _NL, &r->sc_s[4], &r->sc_s[5], &r->sc_s[6],  &r->sc_s[7]);
	String_Format4(str, "s8 =%x s9 =%x s10=%x s11=%x" _NL, &r->sc_s[8], &r->sc_s[9], &r->sc_s[10], &r->sc_s[11]);
	String_Format4(str, "a0 =%x a1 =%x a2 =%x a3 =%x" _NL, &r->sc_s[0], &r->sc_a[1], &r->sc_a[2],  &r->sc_a[3]);
	String_Format4(str, "a4 =%x a5 =%x a6 =%x a7 =%x" _NL, &r->sc_a[4], &r->sc_a[5], &r->sc_a[6],  &r->sc_a[7]);
	String_Format1(str, "pc =%x"                      _NL, &r->sc_sepc);
#elif defined __sparc__
	/* src/sys/arch/sparc64/include/signal.h */
	String_Format3(str, "sp=%x g1=%x o0=%x"           _NL, &r->sc_sp, &r->sc_g1, &r->sc_o0);
	String_Format2(str, "pc=%x nc=%x"                 _NL, &r->sc_pc, &r->sc_npc);
#else
	#error "Unknown CPU architecture"
#endif
}
