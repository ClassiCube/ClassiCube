/* See /headers/posix/arch/[ARCH]/signal.h */

/* Register constants can be found from includes in <signal.h> */
#include <signal.h>

#define RREG(reg) &r->reg
#define RNUM(reg) &r->x[reg]

void CrashHandler_DumpRegisters(void* ctx, cc_string* str) {
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;
#if defined __i386__
	/* /headers/posix/arch/x86/signal.h */
	String_Format3(str, "eax=%x ebx=%x ecx=%x" _NL, RREG(eax), RREG(ebx), RREG(ecx));
	String_Format3(str, "edx=%x esi=%x edi=%x" _NL, RREG(edx), RREG(esi), RREG(edi));
	String_Format3(str, "eip=%x ebp=%x esp=%x" _NL, RREG(eip), RREG(ebp), RREG(esp));
#elif defined __x86_64__
	/* /headers/posix/arch/x86_64/signal.h */
	String_Format3(str, "rax=%x rbx=%x rcx=%x" _NL, RREG(rax), RREG(rbx), RREG(rcx));
	String_Format3(str, "rdx=%x rsi=%x rdi=%x" _NL, RREG(rdx), RREG(rsi), RREG(rdi));
	String_Format3(str, "rip=%x rbp=%x rsp=%x" _NL, RREG(rip), RREG(rbp), RREG(rsp));
	String_Format3(str, "r8 =%x r9 =%x r10=%x" _NL, RREG(r8),  RREG(r9),  RREG(r10));
	String_Format3(str, "r11=%x r12=%x r13=%x" _NL, RREG(r11), RREG(r12), RREG(r13));
	String_Format2(str, "r14=%x r15=%x" _NL,        RREG(r14), RREG(r15));
#elif defined __aarch64__
	/* /headers/posix/arch/arm64/signal.h */
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
	/* /headers/posix/arch/arm/signal.h */
	String_Format3(str, "r0 =%x r1 =%x r2 =%x" _NL, RREG(r0),  RREG(r1),  RREG(r2));
	String_Format3(str, "r3 =%x r4 =%x r5 =%x" _NL, RREG(r3),  RREG(r4),  RREG(r5));
	String_Format3(str, "r6 =%x r7 =%x r8 =%x" _NL, RREG(r6),  RREG(r7),  RREG(r8));
	String_Format3(str, "r9 =%x r10=%x r11=%x" _NL, RREG(r9),  RREG(r10), RREG(r11));
	String_Format3(str, "r12=%x sp =%x lr =%x" _NL, RREG(r12), RREG(r13), RREG(r14));
	String_Format1(str, "pc =%x"               _NL, RREG(r15));
#elif defined __riscv
	/* /headers/posix/arch/riscv64/signal.h */
	String_Format4(str, "pc =%x ra =%x sp =%x r3 =%x" _NL, RREG(pc), RNUM(0),  RNUM(1),  RNUM(2));
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, RNUM(3),  RNUM(4),  RNUM(5),  RNUM(6));
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, RNUM(7),  RNUM(8),  RNUM(9),  RNUM(10));
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, RNUM(11), RNUM(12), RNUM(13), RNUM(14));
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, RNUM(15), RNUM(16), RNUM(17), RNUM(18));
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, RNUM(19), RNUM(20), RNUM(21), RNUM(22));
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, RNUM(23), RNUM(24), RNUM(25), RNUM(26));
	String_Format4(str, "r28=%x r29=%x r30=%x r31=%x" _NL, RNUM(27), RNUM(28), RNUM(29), RNUM(30));
#else
	#error "Unknown CPU architecture"
#endif
}
