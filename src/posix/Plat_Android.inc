/* See /usr/include/sys/ucontext.h */

/* Need to define this to get REG_ constants */
#define _GNU_SOURCE
#include <sys/ucontext.h>
#include <signal.h>

void CrashHandler_DumpRegisters(void* ctx, cc_string* str) {
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;

#if defined __i386__
	#define RREG(reg) &r->gregs[REG_ ## reg]

	String_Format3(str, "eax=%x ebx=%x ecx=%x" _NL, RREG(EAX), RREG(EBX), RREG(ECX));
	String_Format3(str, "edx=%x esi=%x edi=%x" _NL, RREG(EDX), RREG(ESI), RREG(EDI));
	String_Format3(str, "eip=%x ebp=%x esp=%x" _NL, RREG(EIP), RREG(EBP), RREG(ESP));
#elif defined __x86_64__
	#define RREG(reg) &r->gregs[REG_ ## reg]

	String_Format3(str, "rax=%x rbx=%x rcx=%x" _NL, RREG(RAX), RREG(RBX), RREG(RCX));
	String_Format3(str, "rdx=%x rsi=%x rdi=%x" _NL, RREG(RAX), RREG(RSI), RREG(RDI));
	String_Format3(str, "rip=%x rbp=%x rsp=%x" _NL, RREG(RIP), RREG(RBP), RREG(RSP));
	String_Format3(str, "r8 =%x r9 =%x r10=%x" _NL, RREG(R8),  RREG(R9),  RREG(R10));
	String_Format3(str, "r11=%x r12=%x r13=%x" _NL, RREG(R11), RREG(R12), RREG(R13));
	String_Format2(str, "r14=%x r15=%x" _NL,        RREG(R14), RREG(R15));
#elif defined __aarch64__
	/* libc/kernel/uapi/asm-arm64/asm/sigcontext.h */
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
	/* libc/kernel/uapi/asm-arm/asm/sigcontext.h */
	#define RREG(reg) &r->arm_##reg
	#define RNUM(reg) &r->arm_r##reg

	String_Format3(str, "r0 =%x r1 =%x r2 =%x" _NL, RNUM(0),  RNUM(1),  RNUM(2));
	String_Format3(str, "r3 =%x r4 =%x r5 =%x" _NL, RNUM(3),  RNUM(4),  RNUM(5));
	String_Format3(str, "r6 =%x r7 =%x r8 =%x" _NL, RNUM(6),  RNUM(7),  RNUM(8));
	String_Format3(str, "r9 =%x r10=%x fp =%x" _NL, RNUM(9),  RNUM(10), RREG(fp));
	String_Format3(str, "ip =%x sp =%x lr =%x" _NL, RREG(ip), RREG(sp), RREG(lr));
	String_Format1(str, "pc =%x"               _NL, RREG(pc));
#elif defined __riscv
	/* libc/include/sys/ucontext.h */
	#define RREG(reg) &r->__gregs[REG_##reg]
	#define RNUM(reg) &r->__gregs[reg]

	String_Format4(str, "pc =%x ra =%x sp =%x r3 =%x" _NL, RREG(PC), RREG(RA), RREG(SP), RNUM(3));
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, RNUM(4),  RNUM(5),  RNUM(6),  RNUM(7));
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, RNUM(8),  RNUM(9),  RNUM(10), RNUM(11));
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, RNUM(12), RNUM(13), RNUM(14), RNUM(15));
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, RNUM(16), RNUM(17), RNUM(18), RNUM(19));
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, RNUM(20), RNUM(21), RNUM(22), RNUM(23));
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, RNUM(24), RNUM(25), RNUM(26), RNUM(27));
	String_Format4(str, "r28=%x r29=%x r30=%x r31=%x" _NL, RNUM(28), RNUM(29), RNUM(30), RNUM(31));
#else
	#error "Unknown CPU architecture"
#endif
}
