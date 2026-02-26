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
	#define RREG(reg) &r->gregs[reg]

	String_Format3(str, "eax=%x ebx=%x ecx=%x" _NL, RREG(EAX), RREG(EBX), RREG(ECX));
	String_Format3(str, "edx=%x esi=%x edi=%x" _NL, RREG(EDX), RREG(ESI), RREG(EDI));
	String_Format3(str, "eip=%x ebp=%x esp=%x" _NL, RREG(EIP), RREG(EBP), RREG(ESP));
#elif defined __x86_64__
	/* usr/src/uts/intel/sys/regset.h */
	#define RREG(reg) &r->gregs[REG_ ## reg]

	String_Format3(str, "rax=%x rbx=%x rcx=%x" _NL, RREG(RAX), RREG(RBX), RREG(RCX));
	String_Format3(str, "rdx=%x rsi=%x rdi=%x" _NL, RREG(RDX), RREG(RSI), RREG(RDI));
	String_Format3(str, "rip=%x rbp=%x rsp=%x" _NL, RREG(RIP), RREG(RBP), RREG(RSP));
	String_Format3(str, "r8 =%x r9 =%x r10=%x" _NL, RREG(R8),  RREG(R9),  RREG(R10));
	String_Format3(str, "r11=%x r12=%x r13=%x" _NL, RREG(R11), RREG(R12), RREG(R13));
	String_Format2(str, "r14=%x r15=%x" _NL,        RREG(R14), RREG(R15));
#elif defined __sparc__
	/* usr/src/uts/sparc/sys/regset.h */
	#define RREG(reg) &r->gregs[REG_ ## reg]

	String_Format4(str, "o0=%x o1=%x o2=%x o3=%x" _NL, RREG(O0), RREG(O1), RREG(O2), RREG(O3));
	String_Format4(str, "o4=%x o5=%x o6=%x o7=%x" _NL, RREG(O4), RREG(O5), RREG(O6), RREG(O7));
	String_Format4(str, "g1=%x g2=%x g3=%x g4=%x" _NL, RREG(G1), RREG(G2), RREG(G3), RREG(G4));
	String_Format4(str, "g5=%x g6=%x g7=%x y =%x" _NL, RREG(G5), RREG(G6), RREG(G7), RREG(Y));
	String_Format2(str, "pc=%x nc=%x" _NL,             RREG(PC), RREG(nPC));
#else
	#error "Unknown CPU architecture"
#endif
}
