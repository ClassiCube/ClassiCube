/* See /usr/include/sys/regset.h */
/* -> usr/src/uts/[ARCH)/sys/mcontext.h */
/* -> usr/src/uts/[ARCH)/sys/regset.h */
#include <signal.h>
#include <sys/ucontext.h>
#include <sys/regset.h>

void CrashHandler_DumpRegisters(void* ctx, cc_string* str) {
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;

#if defined __i386__
	/* usr/src/uts/intel/sys/regset.h */
	String_Format3(str, "eax=%x ebx=%x ecx=%x" _NL, &r->gregs[EAX], &r->gregs[EBX], &r->gregs[ECX]);
	String_Format3(str, "edx=%x esi=%x edi=%x" _NL, &r->gregs[EDX], &r->gregs[ESI], &r->gregs[EDI]);
	String_Format3(str, "eip=%x ebp=%x esp=%x" _NL, &r->gregs[EIP], &r->gregs[EBP], &r->gregs[ESP]);
#elif defined __x86_64__
	/* usr/src/uts/intel/sys/regset.h */
	String_Format3(str, "rax=%x rbx=%x rcx=%x" _NL, &r->gregs[REG_RAX], &r->gregs[REG_RBX], &r->gregs[REG_RCX]);
	String_Format3(str, "rdx=%x rsi=%x rdi=%x" _NL, &r->gregs[REG_RDX], &r->gregs[REG_RSI], &r->gregs[REG_RDI]);
	String_Format3(str, "rip=%x rbp=%x rsp=%x" _NL, &r->gregs[REG_RIP], &r->gregs[REG_RBP], &r->gregs[REG_RSP]);
	String_Format3(str, "r8 =%x r9 =%x r10=%x" _NL, &r->gregs[REG_R8],  &r->gregs[REG_R9],  &r->gregs[REG_R10]);
	String_Format3(str, "r11=%x r12=%x r13=%x" _NL, &r->gregs[REG_R11], &r->gregs[REG_R12], &r->gregs[REG_R13]);
	String_Format2(str, "r14=%x r15=%x"        _NL, &r->gregs[REG_R14], &r->gregs[REG_R15]);
#elif defined __sparc__
	/* usr/src/uts/sparc/sys/regset.h */
	String_Format4(str, "o0=%x o1=%x o2=%x o3=%x" _NL, &r->gregs[REG_O0], &r->gregs[REG_O1], &r->gregs[REG_O2], &r->gregs[REG_O3]);
	String_Format4(str, "o4=%x o5=%x o6=%x o7=%x" _NL, &r->gregs[REG_O4], &r->gregs[REG_O5], &r->gregs[REG_O6], &r->gregs[REG_O7]);
	String_Format4(str, "g1=%x g2=%x g3=%x g4=%x" _NL, &r->gregs[REG_G1], &r->gregs[REG_G2], &r->gregs[REG_G3], &r->gregs[REG_G4]);
	String_Format4(str, "g5=%x g6=%x g7=%x y =%x" _NL, &r->gregs[REG_G5], &r->gregs[REG_G6], &r->gregs[REG_G7], &r->gregs[REG_Y]);
	String_Format2(str, "pc=%x nc=%x"             _NL, &r->gregs[REG_PC], &r->gregs[REG_nPC]);
#else
	#error "Unknown CPU architecture"
#endif
}
