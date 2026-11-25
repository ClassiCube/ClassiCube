/* See /usr/include/sys/ucontext.h */

/* Need to define this to get REG_ constants */
#define _GNU_SOURCE
#include <sys/ucontext.h>
#include <signal.h>

void CrashHandler_DumpRegisters(void* ctx, cc_string* str) {
#if defined __i386__
	/* See sysdeps/unix/sysv/linux/x86/sys/ucontext.h in glibc */
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;

	String_Format4(str, "eax=%x ebx=%x ecx=%x edx=%x" _NL, &r->gregs[REG_EAX], &r->gregs[REG_EBX], &r->gregs[REG_ECX], &r->gregs[REG_EDX]);
	String_Format4(str, "esi=%x edi=%x ebp=%x esp=%x" _NL, &r->gregs[REG_ESI], &r->gregs[REG_EDI], &r->gregs[REG_EBP], &r->gregs[REG_ESP]);
	String_Format1(str, "eip=%x"                      _NL, &r->gregs[REG_EIP]);
#elif defined __x86_64__
	/* See sysdeps/unix/sysv/linux/arm/sys/ucontext.h in glibc */
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;

	String_Format4(str, "rax=%x rbx=%x rcx=%x rdx=%x" _NL, &r->gregs[REG_RAX], &r->gregs[REG_RBX], &r->gregs[REG_RCX], &r->gregs[REG_RDX]);
	String_Format4(str, "rsi=%x rdi=%x rbp=%x rsp=%x" _NL, &r->gregs[REG_RSI], &r->gregs[REG_RDI], &r->gregs[REG_RBP], &r->gregs[REG_RSP]);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->gregs[REG_R8],  &r->gregs[REG_R9],  &r->gregs[REG_R10], &r->gregs[REG_R11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->gregs[REG_R12], &r->gregs[REG_R13], &r->gregs[REG_R14], &r->gregs[REG_R15]);
	String_Format1(str, "rip=%x"                      _NL, &r->gregs[REG_RIP]);
#elif defined __aarch64__
	/* See sysdeps/unix/sysv/linux/aarch64/sys/ucontext.h in glibc */
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;

	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->regs[0],  &r->regs[1],  &r->regs[2],  &r->regs[3]);
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->regs[4],  &r->regs[5],  &r->regs[6],  &r->regs[7]);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->regs[8],  &r->regs[9],  &r->regs[10], &r->regs[11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->regs[12], &r->regs[13], &r->regs[14], &r->regs[15]);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->regs[16], &r->regs[17], &r->regs[18], &r->regs[19]);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->regs[20], &r->regs[21], &r->regs[22], &r->regs[23]);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->regs[24], &r->regs[25], &r->regs[26], &r->regs[27]);
	String_Format4(str, "r28=%x r29=%x lr =%x sp =%x" _NL, &r->regs[28], &r->regs[29], &r->regs[30], &r->sp);
	String_Format1(str, "pc =%x" _NL,                      &r->pc);
#elif defined __arm__
	/* See sysdeps/unix/sysv/linux/arm/sys/ucontext.h in glibc */
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;

	String_Format3(str, "r0 =%x r1 =%x r2 =%x" _NL, &r->arm_r0,  &r->arm_r1,  &r->arm_r2);
	String_Format3(str, "r3 =%x r4 =%x r5 =%x" _NL, &r->arm_r3,  &r->arm_r4,  &r->arm_r5);
	String_Format3(str, "r6 =%x r7 =%x r8 =%x" _NL, &r->arm_r6,  &r->arm_r7,  &r->arm_r8);
	String_Format3(str, "r9 =%x r10=%x fp =%x" _NL, &r->arm_r9,  &r->arm_r10, &r->arm_fp);
	String_Format3(str, "ip =%x sp =%x lr =%x" _NL, &r->arm_ip, &r->arm_sp, &r->arm_lr);
	String_Format1(str, "pc =%x"               _NL, &r->arm_pc);
#elif defined __alpha__
	/* See sysdeps/unix/sysv/linux/alpha/sys/ucontext.h in glibc */
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;	

	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->sc_regs[0],  &r->sc_regs[1],  &r->sc_regs[2],  &r->sc_regs[3]);
	String_Format4(str, "r4 =%x r5 =%x t5 =%x r7 =%x" _NL, &r->sc_regs[4],  &r->sc_regs[5],  &r->sc_regs[6],  &r->sc_regs[7]);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->sc_regs[8],  &r->sc_regs[9],  &r->sc_regs[10], &r->sc_regs[11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->sc_regs[12], &r->sc_regs[13], &r->sc_regs[14], &r->sc_regs[15]);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->sc_regs[16], &r->sc_regs[17], &r->sc_regs[18], &r->sc_regs[19]);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->sc_regs[20], &r->sc_regs[21], &r->sc_regs[22], &r->sc_regs[23]);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->sc_regs[24], &r->sc_regs[25], &r->sc_regs[26], &r->sc_regs[27]);
	String_Format3(str, "r28=%x r29=%x r30=%x r31=%x" _NL, &r->sc_regs[28], &r->sc_regs[29], &r->sc_regs[30], &r->sc_regs[31]);
	String_Format2(str, "pc =%x ps =%x"               _NL, &r->sc_pc,       &r->sc_ps);
#elif defined __sparc__
	/* See sysdeps/unix/sysv/linux/powerpc/sys/ucontext.h in glibc */
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;

	String_Format4(str, "o0=%x o1=%x o2=%x o3=%x" _NL, &r->gregs[REG_O0], &r->gregs[REG_O1], &r->gregs[REG_O2], &r->gregs[REG_O3]);
	String_Format4(str, "o4=%x o5=%x o6=%x o7=%x" _NL, &r->gregs[REG_O4], &r->gregs[REG_O5], &r->gregs[REG_O6], &r->gregs[REG_O7]);
	String_Format4(str, "g1=%x g2=%x g3=%x g4=%x" _NL, &r->gregs[REG_G1], &r->gregs[REG_G2], &r->gregs[REG_G3], &r->gregs[REG_G4]);
	String_Format4(str, "g5=%x g6=%x g7=%x y =%x" _NL, &r->gregs[REG_G5], &r->gregs[REG_G6], &r->gregs[REG_G7], &r->gregs[REG_Y]);
	String_Format2(str, "pc=%x nc=%x"             _NL, &r->gregs[REG_PC], &r->gregs[REG_nPC]);
#elif defined __PPC__ && __WORDSIZE == 32
	/* See sysdeps/unix/sysv/linux/powerpc/sys/ucontext.h in glibc */
	/* https://stackoverflow.com/questions/48526785/extract-program-counter-instruction-pointer-in-signal-handler-ppc64 */
	/*	https://elixir.bootlin.com/linux/v6.11.7/source/arch/powerpc/kernel/signal_64.c#L180 */
	/*	https://elixir.bootlin.com/linux/v6.11.7/source/arch/powerpc/include/uapi/asm/ptrace.h */
	/*	PT_NIP = 32, PT_CTR = 35, PT_LNK = 36, PT_XER = 37, PT_CCR = 38 */
	mcontext_t* r = ((ucontext_t*)ctx)->uc_mcontext.uc_regs;

	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->gregs[0],  &r->gregs[1],  &r->gregs[2],  &r->gregs[3]);
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->gregs[4],  &r->gregs[5],  &r->gregs[6],  &r->gregs[7]);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->gregs[8],  &r->gregs[9],  &r->gregs[10], &r->gregs[11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->gregs[12], &r->gregs[13], &r->gregs[14], &r->gregs[15]);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->gregs[16], &r->gregs[17], &r->gregs[18], &r->gregs[19]);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->gregs[20], &r->gregs[21], &r->gregs[22], &r->gregs[23]);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->gregs[24], &r->gregs[25], &r->gregs[26], &r->gregs[27]);
	String_Format4(str, "r28=%x r29=%x r30=%x r31=%x" _NL, &r->gregs[28], &r->gregs[29], &r->gregs[30], &r->gregs[31]);
	String_Format4(str, "lr =%x cr =%x xer=%x ctr=%x" _NL, &r->gregs[36], &r->gregs[38], &r->gregs[37], &r->gregs[35]);
	String_Format1(str, "pc =%x"                      _NL, &r->gregs[32]);
#elif defined __PPC__
	/* see above code block */
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;

	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->gp_regs[0],  &r->gp_regs[1],  &r->gp_regs[2],  &r->gp_regs[3]);
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->gp_regs[4],  &r->gp_regs[5],  &r->gp_regs[6],  &r->gp_regs[7]);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->gp_regs[8],  &r->gp_regs[9],  &r->gp_regs[10], &r->gp_regs[11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->gp_regs[12], &r->gp_regs[13], &r->gp_regs[14], &r->gp_regs[15]);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->gp_regs[16], &r->gp_regs[17], &r->gp_regs[18], &r->gp_regs[19]);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->gp_regs[20], &r->gp_regs[21], &r->gp_regs[22], &r->gp_regs[23]);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->gp_regs[24], &r->gp_regs[25], &r->gp_regs[26], &r->gp_regs[27]);
	String_Format4(str, "r28=%x r29=%x r30=%x r31=%x" _NL, &r->gp_regs[28], &r->gp_regs[29], &r->gp_regs[30], &r->gp_regs[31]);
	String_Format4(str, "lr =%x cr =%x xer=%x ctr=%x" _NL, &r->gp_regs[36], &r->gp_regs[38], &r->gp_regs[37], &r->gp_regs[35]);
	String_Format1(str, "pc =%x"                      _NL, &r->gp_regs[32]);
#elif defined __mips__
	/* See sysdeps/unix/sysv/linux/mips/sys/ucontext.h in glibc */
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;

	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->gregs[0],  &r->gregs[1],  &r->gregs[2],  &r->gregs[3] );
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->gregs[4],  &r->gregs[5],  &r->gregs[6],  &r->gregs[7] );
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->gregs[8],  &r->gregs[9],  &r->gregs[10], &r->gregs[11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->gregs[12], &r->gregs[13], &r->gregs[14], &r->gregs[15]);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->gregs[16], &r->gregs[17], &r->gregs[18], &r->gregs[19]);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->gregs[20], &r->gregs[21], &r->gregs[22], &r->gregs[23]);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->gregs[24], &r->gregs[25], &r->gregs[26], &r->gregs[27]);
	String_Format4(str, "r28=%x sp =%x fp =%x ra =%x" _NL, &r->gregs[28], &r->gregs[29], &r->gregs[30], &r->gregs[31]);
	String_Format3(str, "lo =%x hi =%x pc =%x"        _NL, &r->mdlo, &r->mdhi, &r->pc);
#elif defined __riscv
	/* See sysdeps/unix/sysv/linux/riscv/sys/ucontext.h in glibc */
	/* ZERO = R0, RA = R1, SP = R2 */
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;

	String_Format3(str, "ra =%x sp =%x r3 =%x"        _NL,                  &r->__gregs[1],  &r->__gregs[2],  &r->__gregs[3]);
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->__gregs[4],  &r->__gregs[5],  &r->__gregs[6],  &r->__gregs[7]);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->__gregs[8],  &r->__gregs[9],  &r->__gregs[10], &r->__gregs[11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->__gregs[12], &r->__gregs[13], &r->__gregs[14], &r->__gregs[15]);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->__gregs[16], &r->__gregs[17], &r->__gregs[18], &r->__gregs[19]);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->__gregs[20], &r->__gregs[21], &r->__gregs[22], &r->__gregs[23]);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->__gregs[24], &r->__gregs[25], &r->__gregs[26], &r->__gregs[27]);
	String_Format4(str, "r28=%x r29=%x r30=%x r31=%x" _NL, &r->__gregs[28], &r->__gregs[29], &r->__gregs[30], &r->__gregs[31]);
	String_Format1(str, "pc =%x"                      _NL, &r->__gregs[REG_PC]);
#elif defined __e2k__
	struct sigcontext* r = &((ucontext_t*)ctx)->uc_mcontext;

	/* Procedure Stack Pointer, Procedure Chain Stack Pointer, User Stack Base Pointer, User Stack Pointer */
	String_Format4(str, "ps =%x pcs=%x sbr=%x usd=%x" _NL, &r->psp_hi, &r->pcsp_hi, &r->sbr, &r->usd_hi);
	/* CR0 contains Instruction Pointer */
	String_Format1(str, "ip =%x"                      _NL, &r->cr0_hi);
#else
	#error "Unknown CPU architecture"
#endif
}

