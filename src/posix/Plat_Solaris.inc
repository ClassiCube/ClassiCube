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
	String_Format4(str, "eax=%x ebx=%x ecx=%x edx=%x" _NL, &r->gregs[EAX], &r->gregs[EBX], &r->gregs[ECX], &r->gregs[EDX]);
	String_Format4(str, "esi=%x edi=%x ebp=%x esp=%x" _NL, &r->gregs[ESI], &r->gregs[EDI], &r->gregs[EBP], &r->gregs[ESP]);
	String_Format1(str, "eip=%x"                      _NL, &r->gregs[EIP]);
#elif defined __x86_64__
	/* usr/src/uts/intel/sys/regset.h */
	String_Format4(str, "rax=%x rbx=%x rcx=%x rdx=%x" _NL, &r->gregs[REG_RAX], &r->gregs[REG_RBX], &r->gregs[REG_RCX], &r->gregs[REG_RDX]);
	String_Format4(str, "rsi=%x rdi=%x rbp=%x rsp=%x" _NL, &r->gregs[REG_RSI], &r->gregs[REG_RDI], &r->gregs[REG_RBP], &r->gregs[REG_RSP]);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->gregs[REG_R8],  &r->gregs[REG_R9],  &r->gregs[REG_R10], &r->gregs[REG_R11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->gregs[REG_R12], &r->gregs[REG_R13], &r->gregs[REG_R14], &r->gregs[REG_R15]);
	String_Format1(str, "rip=%x"                      _NL, &r->gregs[REG_RIP]);
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
