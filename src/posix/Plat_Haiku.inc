/* See /headers/posix/arch/[ARCH]/signal.h */

/* Register constants can be found from includes in <signal.h> */
#include <signal.h>

void CrashHandler_DumpRegisters(void* ctx, cc_string* str) {
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;

#if defined __i386__
	/* /headers/posix/arch/x86/signal.h */
	String_Format3(str, "eax=%x ebx=%x ecx=%x" _NL, &r->eax, &r->ebx, &r->ecx);
	String_Format3(str, "edx=%x esi=%x edi=%x" _NL, &r->edx, &r->esi, &r->edi);
	String_Format3(str, "eip=%x ebp=%x esp=%x" _NL, &r->eip, &r->ebp, &r->esp);
#elif defined __x86_64__
	/* /headers/posix/arch/x86_64/signal.h */
	String_Format3(str, "rax=%x rbx=%x rcx=%x" _NL, &r->rax, &r->rbx, &r->rcx);
	String_Format3(str, "rdx=%x rsi=%x rdi=%x" _NL, &r->rdx, &r->rsi, &r->rdi);
	String_Format3(str, "rip=%x rbp=%x rsp=%x" _NL, &r->rip, &r->rbp, &r->rsp);
	String_Format3(str, "r8 =%x r9 =%x r10=%x" _NL, &r->r8,  &r->r9,  &r->r10);
	String_Format3(str, "r11=%x r12=%x r13=%x" _NL, &r->r11, &r->r12, &r->r13);
	String_Format2(str, "r14=%x r15=%x" _NL,        &r->r14, &r->r15);
#elif defined __aarch64__
	/* /headers/posix/arch/arm64/signal.h */
	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->x0,  &r->x1,  &r->x2,  &r->x3);
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->x4,  &r->x5,  &r->x6,  &r->x7);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->x8,  &r->x9,  &r->x10, &r->x11);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->x12, &r->x13, &r->x14, &r->x15);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->x16, &r->x17, &r->x18, &r->x19);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->x20, &r->x21, &r->x22, &r->x23);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->x24, &r->x25, &r->x26, &r->x27);
	String_Format4(str, "r28=%x r29=%x lr =%x sp =%x" _NL, &r->x28, &r->x29, &r->lr,  &r->sp);
	String_Format1(str, "pc =%x" _NL,                      &r->elr);
#elif defined __arm__
	/* /headers/posix/arch/arm/signal.h */
	String_Format3(str, "r0 =%x r1 =%x r2 =%x" _NL, &r->r0,  &r->r1,  &r->r2);
	String_Format3(str, "r3 =%x r4 =%x r5 =%x" _NL, &r->r3,  &r->r4,  &r->r5);
	String_Format3(str, "r6 =%x r7 =%x r8 =%x" _NL, &r->r6,  &r->r7,  &r->r8);
	String_Format3(str, "r9 =%x r10=%x r11=%x" _NL, &r->r9,  &r->r10, &r->r11);
	String_Format3(str, "r12=%x sp =%x lr =%x" _NL, &r->r12, &r->r13, &r->r14);
	String_Format1(str, "pc =%x"               _NL, &r->r15);
#elif defined __riscv
	/* /headers/posix/arch/riscv64/signal.h */
	String_Format4(str, "pc =%x ra =%x sp =%x r3 =%x" _NL, &r->pc,  &r->x0,  &r->x1,  &r->x2);
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->x3,  &r->x4,  &r->x5,  &r->x6);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->x7,  &r->x8,  &r->x9,  &r->x10);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->x11, &r->x12, &r->x13, &r->x14);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->x15, &r->x16, &r->x17, &r->x18);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->x19, &r->x20, &r->x21, &r->x22);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->x23, &r->x24, &r->x25, &r->x26);
	String_Format4(str, "r28=%x r29=%x r30=%x r31=%x" _NL, &r->x27, &r->x28, &r->x29, &r->x30);
#else
	#error "Unknown CPU architecture"
#endif
}
