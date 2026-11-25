/* See Kernel/Arch/[ARCH]/mcontext.h */

/* Register constants can be found from includes in <signal.h> */
#include <signal.h>

void CrashHandler_DumpRegisters(void* ctx, cc_string* str) {
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;

#if defined __x86_64__
	/* Kernel/Arch/x86_64/mcontext.h */
	String_Format4(str, "rax=%x rbx=%x rcx=%x rdx=%x" _NL, &r->rax, &r->rbx, &r->rcx, &r->rdx);
	String_Format4(str, "rsi=%x rdi=%x rbp=%x rsp=%x" _NL, &r->rsi, &r->rdi, &r->rbp, &r->rsp);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->r8,  &r->r9,  &r->r10, &r->r11);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->r12, &r->r13, &r->r14, &r->r15);
	String_Format1(str, "rip=%x"                      _NL, &r->rip);
#elif defined __aarch64__
	/* Kernel/Arch/aarch64/mcontext.h */
	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->x[0],  &r->x[1],  &r->x[2],  &r->x[3]);
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->x[4],  &r->x[5],  &r->x[6],  &r->x[7]);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->x[8],  &r->x[9],  &r->x[10], &r->x[11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->x[12], &r->x[13], &r->x[14], &r->x[15]);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->x[16], &r->x[17], &r->x[18], &r->x[19]);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->x[20], &r->x[21], &r->x[22], &r->x[23]);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->x[24], &r->x[25], &r->x[26], &r->x[27]);
	String_Format4(str, "r28=%x fp =%x lr =%x sp =%x" _NL, &r->x[28], &r->x[29], &r->x[30], &r->sp);
	String_Format1(str, "pc =%x" _NL,                      &r->pc);
#elif defined __riscv
	/* Kernel/Arch/riscv64/mcontext.h */
	String_Format4(str, "pc =%x ra =%x sp =%x r3 =%x" _NL, &r->pc,    &r->x[0],  &r->x[1],  &r->x[2]);
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->x[3],  &r->x[4],  &r->x[5],  &r->x[6]);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->x[7],  &r->x[8],  &r->x[9],  &r->x[10]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->x[11], &r->x[12], &r->x[13], &r->x[14]);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->x[15], &r->x[16], &r->x[17], &r->x[18]);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->x[19], &r->x[20], &r->x[21], &r->x[22]);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->x[23], &r->x[24], &r->x[25], &r->x[26]);
	String_Format4(str, "r28=%x r29=%x r30=%x r31=%x" _NL, &r->x[27], &r->x[28], &r->x[29], &r->x[30]);
#else
	#error "Unknown CPU architecture"
#endif
}
