/* See /usr/include/sys/ucontext.h */

/* Need to define this to get REG_ constants */
#define _GNU_SOURCE
#include <sys/ucontext.h>
#include <signal.h>

void CrashHandler_DumpRegisters(void* ctx, cc_string* str) {
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;

#if defined __i386__
	String_Format4(str, "eax=%x ebx=%x ecx=%x edx=%x" _NL, &r->gregs[REG_EAX], &r->gregs[REG_EBX], &r->gregs[REG_ECX], &r->gregs[REG_EDX]);
	String_Format4(str, "esi=%x edi=%x ebp=%x esp=%x" _NL, &r->gregs[REG_ESI], &r->gregs[REG_EDI], &r->gregs[REG_EBP], &r->gregs[REG_ESP]);
	String_Format1(str, "eip=%x"                      _NL, &r->gregs[REG_EIP]);
#elif defined __x86_64__
	String_Format4(str, "rax=%x rbx=%x rcx=%x rdx=%x" _NL, &r->gregs[REG_RAX], &r->gregs[REG_RBX], &r->gregs[REG_RCX], &r->gregs[REG_RDX]);
	String_Format4(str, "rsi=%x rdi=%x rbp=%x rsp=%x" _NL, &r->gregs[REG_RSI], &r->gregs[REG_RDI], &r->gregs[REG_RBP], &r->gregs[REG_RSP]);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->gregs[REG_R8],  &r->gregs[REG_R9],  &r->gregs[REG_R10], &r->gregs[REG_R11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->gregs[REG_R12], &r->gregs[REG_R13], &r->gregs[REG_R14], &r->gregs[REG_R15]);
	String_Format1(str, "rip=%x"                      _NL, &r->gregs[REG_RIP]);
#elif defined __aarch64__
	/* libc/kernel/uapi/asm-arm64/asm/sigcontext.h */
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
	/* libc/kernel/uapi/asm-arm/asm/sigcontext.h */
	String_Format3(str, "r0 =%x r1 =%x r2 =%x" _NL, &r->arm_r0,  &r->arm_r1,  &r->arm_r2);
	String_Format3(str, "r3 =%x r4 =%x r5 =%x" _NL, &r->arm_r3,  &r->arm_r4,  &r->arm_r5);
	String_Format3(str, "r6 =%x r7 =%x r8 =%x" _NL, &r->arm_r6,  &r->arm_r7,  &r->arm_r8);
	String_Format3(str, "r9 =%x r10=%x fp =%x" _NL, &r->arm_r9,  &r->arm_r10, &r->arm_fp);
	String_Format3(str, "ip =%x sp =%x lr =%x" _NL, &r->arm_ip, &r->arm_sp, &r->arm_lr);
	String_Format1(str, "pc =%x"               _NL, &r->arm_pc);
#elif defined __riscv
	/* libc/include/sys/ucontext.h */
	/* ZERO = R0, RA = R1, SP = R2 */
	String_Format3(str, "ra =%x sp =%x r3 =%x"        _NL,                  &r->__gregs[1],  &r->__gregs[2],  &r->__gregs[3]);
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->__gregs[4],  &r->__gregs[5],  &r->__gregs[6],  &r->__gregs[7]);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->__gregs[8],  &r->__gregs[9],  &r->__gregs[10], &r->__gregs[11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->__gregs[12], &r->__gregs[13], &r->__gregs[14], &r->__gregs[15]);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->__gregs[16], &r->__gregs[17], &r->__gregs[18], &r->__gregs[19]);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->__gregs[20], &r->__gregs[21], &r->__gregs[22], &r->__gregs[23]);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->__gregs[24], &r->__gregs[25], &r->__gregs[26], &r->__gregs[27]);
	String_Format4(str, "r28=%x r29=%x r30=%x r31=%x" _NL, &r->__gregs[28], &r->__gregs[29], &r->__gregs[30], &r->__gregs[31]);
	String_Format1(str, "pc =%x"                      _NL, &r->__gregs[REG_PC]);
#else
	#error "Unknown CPU architecture"
#endif
}
