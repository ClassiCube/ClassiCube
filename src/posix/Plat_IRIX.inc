/* See /usr/include/sys/ucontext.h */
/* https://nixdoc.net/man-pages/IRIX/man5/UCONTEXT.5.html */
/* https://gnu.googlesource.com/binutils-gdb/+/dfa9f0d57b133318a0de825a53a8b8ced0e86914/gdb/irix5-nat.c */
#include <signal.h>
#include <sys/ucontext.h>

void CrashHandler_DumpRegisters(void* ctx, cc_string* str) {
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;

	/* TODO verify these offsets */
	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->__gregs[0],  &r->__gregs[1],  &r->__gregs[2],  &r->__gregs[3] );
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->__gregs[4],  &r->__gregs[5],  &r->__gregs[6],  &r->__gregs[7] );
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->__gregs[8],  &r->__gregs[9],  &r->__gregs[10], &r->__gregs[11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->__gregs[12], &r->__gregs[13], &r->__gregs[14], &r->__gregs[15]);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->__gregs[16], &r->__gregs[17], &r->__gregs[18], &r->__gregs[19]);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->__gregs[20], &r->__gregs[21], &r->__gregs[22], &r->__gregs[23]);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->__gregs[24], &r->__gregs[25], &r->__gregs[26], &r->__gregs[27]);
	String_Format4(str, "r28=%x sp =%x fp =%x ra =%x" _NL, &r->__gregs[28], &r->__gregs[29], &r->__gregs[30], &r->__gregs[31]);

	String_Format3(str, "lo =%x hi =%x pc =%x"        _NL, &r->__gregs[CTX_MDLO], &r->__gregs[CTX_MDHI], &r->__gregs[CTX_EPC]);
}
