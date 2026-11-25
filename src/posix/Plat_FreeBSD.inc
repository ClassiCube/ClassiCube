/* See /usr/include/machine/ucontext.h */
/* -> src/sys/[ARCH]/include/ucontext.h */
#include <signal.h>
#include <sys/ucontext.h>

void CrashHandler_DumpRegisters(void* ctx, cc_string* str) {
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;
#if defined __i386__
	/* src/sys/x86/include/ucontext.h */
	String_Format4(str, "eax=%x ebx=%x ecx=%x edx=%x" _NL, &r->mc_eax, &r->mc_ebx, &r->mc_ecx, &r->mc_edx);
	String_Format4(str, "esi=%x edi=%x ebp=%x esp=%x" _NL, &r->mc_esi, &r->mc_edi, &r->mc_ebp, &r->mc_esp);
	String_Format1(str, "eip=%x"                      _NL, &r->mc_eip);
#elif defined __x86_64__
	/* src/sys/x86/include/ucontext.h */
	String_Format4(str, "rax=%x rbx=%x rcx=%x rdx=%x" _NL, &r->mc_rax, &r->mc_rbx, &r->mc_rcx, &r->mc_rdx);
	String_Format4(str, "rsi=%x rdi=%x rbp=%x rsp=%x" _NL, &r->mc_rsi, &r->mc_rdi, &r->mc_rbp, &r->mc_rsp);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->mc_r8,  &r->mc_r9,  &r->mc_r10, &r->mc_r11);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->mc_r12, &r->mc_r13, &r->mc_r14, &r->mc_r15);
	String_Format1(str, "rip=%x"                      _NL, &r->mc_rip);
#elif defined __aarch64__
	/* src/sys/arm64/include/ucontext.h */
	/* LR = R30, SP = R31 */
	struct gpregs* g = &r->mc_gpregs;

	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &g->gp_x[0],  &g->gp_x[1],  &g->gp_x[2],  &g->gp_x[3]);
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &g->gp_x[4],  &g->gp_x[5],  &g->gp_x[6],  &g->gp_x[7]);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &g->gp_x[8],  &g->gp_x[9],  &g->gp_x[10], &g->gp_x[11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &g->gp_x[12], &g->gp_x[13], &g->gp_x[14], &g->gp_x[15]);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &g->gp_x[16], &g->gp_x[17], &g->gp_x[18], &g->gp_x[19]);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &g->gp_x[20], &g->gp_x[21], &g->gp_x[22], &g->gp_x[23]);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &g->gp_x[24], &g->gp_x[25], &g->gp_x[26], &g->gp_x[27]);
	String_Format4(str, "r28=%x r29=%x lr =%x sp =%x" _NL, &g->gp_x[28], &g->gp_x[29], &g->gp_lr,    &g->gp_sp);
	String_Format1(str, "pc =%x"                      _NL, &g->gp_elr);
#elif defined __arm__
	/* src/sys/arm/include/ucontext.h */
	/* FP = R11, SP = R13, LR = R14, PC = R15 */
	String_Format3(str, "r0 =%x r1 =%x r2 =%x" _NL, &r->__gregs[0],  &r->__gregs[1],  &r->__gregs[2]);
	String_Format3(str, "r3 =%x r4 =%x r5 =%x" _NL, &r->__gregs[3],  &r->__gregs[4],  &r->__gregs[5]);
	String_Format3(str, "r6 =%x r7 =%x r8 =%x" _NL, &r->__gregs[6],  &r->__gregs[7],  &r->__gregs[8]);
	String_Format3(str, "r9 =%x r10=%x fp =%x" _NL, &r->__gregs[9],  &r->__gregs[10], &r->__gregs[11]);
	String_Format3(str, "r12=%x sp =%x lr =%x" _NL, &r->__gregs[12], &r->__gregs[13], &r->__gregs[14]);
	String_Format1(str, "pc =%x"               _NL, &r->__gregs[_REG_PC]);
#elif defined __riscv
	/* src/sys/riscv/include/ucontext.h */
	/* note that R0 = ZERO and is not in registers struct */
	String_Format3(str, "ra =%x sp =%x gp =%x"        _NL,              &r->rg_ra,   &r->rg_sp,    &r->rg_gp);
	String_Format4(str, "tp =%x t0 =%x t1 =%x t2 =%x" _NL, &r->rg_tp,   &r->rg_t[0], &r->rg_t[1],  &r->rg_t[2]);
	String_Format4(str, "t3 =%x t4 =%x t5 =%x t6 =%x" _NL, &r->rg_t[3], &r->rg_t[4], &r->rg_t[5],  &r->rg_t[6]);
	String_Format4(str, "s0 =%x s1 =%x s2 =%x s3 =%x" _NL, &r->rg_s[0], &r->rg_s[1], &r->rg_s[2],  &r->rg_s[3]);
	String_Format4(str, "s4 =%x s5 =%x s6 =%x s7 =%x" _NL, &r->rg_s[4], &r->rg_s[5], &r->rg_s[6],  &r->rg_s[7]);
	String_Format4(str, "s8 =%x s9 =%x s10=%x s11=%x" _NL, &r->rg_s[8], &r->rg_s[9], &r->rg_s[10], &r->rg_s[11]);
	String_Format4(str, "a0 =%x a1 =%x a2 =%x a3 =%x" _NL, &r->rg_s[0], &r->rg_a[1], &r->rg_a[2],  &r->rg_a[3]);
	String_Format4(str, "a4 =%x a5 =%x a6 =%x a7 =%x" _NL, &r->rg_a[4], &r->rg_a[5], &r->rg_a[6],  &r->rg_a[7]);
	String_Format1(str, "pc =%x"                      _NL, &r->rg_sepc);
#elif defined __powerpc__
	/* src/sys/powerpc/include/ucontext.h */
	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->mc_frame[0],  &r->mc_frame[1],  &r->mc_frame[2],   &r->mc_frame[3]);
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->mc_frame[4],  &r->mc_frame[5],  &r->mc_frame[6],   &r->mc_frame[7]);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->mc_frame[8],  &r->mc_frame[9],  &r->mc_frame[10],  &r->mc_frame[11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->mc_frame[12], &r->mc_frame[13], &r->mc_frame[14],  &r->mc_frame[15]);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->mc_frame[16], &r->mc_frame[17], &r->mc_frame[18],  &r->mc_frame[19]);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->mc_frame[20], &r->mc_frame[21], &r->mc_frame[22],  &r->mc_frame[23]);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->mc_frame[24], &r->mc_frame[25], &r->mc_frame[26],  &r->mc_frame[27]);
	String_Format4(str, "r28=%x r29=%x r30=%x r31=%x" _NL, &r->mc_frame[28], &r->mc_frame[29], &r->mc_frame[30],  &r->mc_frame[31]);
	String_Format4(str, "lr =%x cr =%x xer=%x ctr=%x" _NL, &r->mc_lr, &r->mc_cr, &r->mc_xer, &r->mc_ctr);
	String_Format1(str, "pc =%x"                      _NL, &r->mc_srr0);
#elif defined __mips__
	/* src/sys/mips/include/ucontext.h */
	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->mc_regs[0],  &r->mc_regs[1],  &r->mc_regs[2],  &r->mc_regs[3] );
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->mc_regs[4],  &r->mc_regs[5],  &r->mc_regs[6],  &r->mc_regs[7] );
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->mc_regs[8],  &r->mc_regs[9],  &r->mc_regs[10], &r->mc_regs[11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->mc_regs[12], &r->mc_regs[13], &r->mc_regs[14], &r->mc_regs[15]);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->mc_regs[16], &r->mc_regs[17], &r->mc_regs[18], &r->mc_regs[19]);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->mc_regs[20], &r->mc_regs[21], &r->mc_regs[22], &r->mc_regs[23]);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->mc_regs[24], &r->mc_regs[25], &r->mc_regs[26], &r->mc_regs[27]);
	String_Format4(str, "r28=%x sp =%x fp =%x ra =%x" _NL, &r->mc_regs[28], &r->mc_regs[29], &r->mc_regs[30], &r->mc_regs[31]);
	String_Format3(str, "lo =%x hi =%x pc =%x"        _NL, &r->mullo, &r->mullhi, &r->mc_pc);
#else
	#error "Unknown CPU architecture"
#endif
}
