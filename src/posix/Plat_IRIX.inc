/* See /usr/include/sys/ucontext.h */
/* https://nixdoc.net/man-pages/IRIX/man5/UCONTEXT.5.html */
#include <signal.h>
#include <sys/ucontext.h>

#define RREG(reg) &r->__gregs[CTX ## reg]

void CrashHandler_DumpRegisters(void* ctx, cc_string* str) {
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;

	/* TODO dump other CPU registers.. */
	String_Format3(str, "pc =%x lo =%x hi =%x" _NL, RREG(EPC), RREG(MDLO), RREG(MDHI));
}
